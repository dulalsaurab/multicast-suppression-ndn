// build a suppression strategy for each 
#include "multicast-suppression.hpp"
#include <tuple>
#include <functional>
#include <iostream>
#include "common/global.hpp"
#include "common/logger.hpp"
#include <algorithm>
#include <numeric>
#include<stdio.h>
#include <random>
#include <math.h>
#include <fstream>

namespace nfd {
namespace face {
namespace ams {

NFD_LOG_INIT(MulticastSuppression);

double DISCOUNT_FACTOR = 0.125;
double MAX_PROPOGATION_DELAY = 15; 
const time::milliseconds MAX_MEASURMENT_INACTIVE_PERIOD = 300_s; // 5 minutes
const time::milliseconds DEFAULT_INSTANT_LIFETIME = 30_ms;
const double DUPLICATE_THRESHOLD = 1.5; // parameter to tune 
const double ADATIVE_DECREASE = 10.0;
const double MULTIPLICATIVE_INCREASE = 1.4;

// in milliseconds ms
const double minSuppressionTime = 5.0f; 
const double maxSuppressionTime= 15000.0f;
static bool seeded = false;

// only for experiment purpose, will be removed later
int
getSeed()
{
    std::string line;
    std::ifstream f ("seed"); // read file and return the value
    getline(f, line);
    auto seed = std::stoi (line);
    NFD_LOG_INFO ("value of the seed: " << seed);
    return seed;
}

int
getRandomNumber(int upperBound)
{
    if (!seeded) {
        unsigned seed = getSeed();
        NFD_LOG_INFO ("seeded " << seed);
        std::srand(seed);
        seeded = true;
    }
    return std::rand() % upperBound;
}

// EMA section
// objectName granularity is (-1) name component
EMAMeasurements::EMAMeasurements(double expMovingAverage = 0, int lastDuplicateCount = 0, double suppressionTime = 5.0)
: m_expMovingAveragePrev (expMovingAverage)
, m_expMovingAverageCurrent (expMovingAverage)
, m_averageDuplicateCount (lastDuplicateCount) // need to change: average for all the name, e.g. name (/a/b, = 2 /a/c = 2), /a = (2+2)/2 = 2 
, m_currentSuppressionTime(suppressionTime)
, m_rng(ndn::random::getRandomNumberEngine())
, m_rangeUniformRandom(0, maxSuppressionTime)
{
}
/*
 we compute exponential moving average to give higher preference to the most recent interest/data
 EMA = duplicate count if t = 1
 EMA  = alpha*Dt + (1 - alpha) * EMA t-1
*/
void
EMAMeasurements::addUpdateEMA(int duplicateCount)
{
    m_expMovingAveragePrev = m_expMovingAverageCurrent;
    if (m_expMovingAverageCurrent == 0) {
        m_expMovingAverageCurrent = duplicateCount;
    }
    else {
        m_expMovingAverageCurrent =  round ((DISCOUNT_FACTOR*duplicateCount
-                                                                       + (1 - DISCOUNT_FACTOR)*this->m_expMovingAverageCurrent)*100.0)/100.0;
        updateDelayTime();
    }
    NFD_LOG_INFO("Moving average" << " before: " << m_expMovingAveragePrev 
                                                                 << " after: " << m_expMovingAverageCurrent
                                                                 << " duplicate count: " << duplicateCount
                                                                 << " suppression time: "<< this->m_currentSuppressionTime);
}

time::milliseconds
EMAMeasurements::getCurrentSuppressionTime()
{
    time::milliseconds suppressionTime (getRandomNumber(static_cast<int> (this->m_currentSuppressionTime)));
    NFD_LOG_INFO("Suppression time: " << this->m_currentSuppressionTime << " Suppression timer: " << suppressionTime);
    return suppressionTime;
}

void
EMAMeasurements::updateDelayTime()
{
    double temp;
    // Implicit action: if you haven’t reached the goal, but your moving average is decreasing then do nothing.
    if (m_expMovingAverageCurrent > DUPLICATE_THRESHOLD && m_expMovingAverageCurrent >=m_expMovingAveragePrev)
        temp = m_currentSuppressionTime*MULTIPLICATIVE_INCREASE; // ADATIVE_DECREASE;
    else if (m_expMovingAverageCurrent <= DUPLICATE_THRESHOLD)
        temp = m_currentSuppressionTime - ADATIVE_DECREASE;
    else
        temp = m_currentSuppressionTime;
    
    m_currentSuppressionTime =  std::min(std::max(minSuppressionTime, temp), maxSuppressionTime);
}

void
MulticastSuppression::recordInterest(const Interest interest)
{
    auto name = interest.getName();
    NFD_LOG_INFO("Interest to check/record" << name);
    auto it = m_interestHistory.find(name);
    if (it == m_interestHistory.end()) // check if interest is already in the map
    {
        m_interestHistory.emplace(name, 1);
        NFD_LOG_INFO ("Interest: " << name << " inserted into map");

        //  remove the entry after the lifetime expries
        time::milliseconds entryLifetime = DEFAULT_INSTANT_LIFETIME;
        NFD_LOG_INFO("Erasing the interest from the map in : " << entryLifetime); 
        setUpdateExpiration(entryLifetime, name, 'i');
    }
    else { 
        NFD_LOG_INFO("Counter for interest " << name << " incremented");
         ++it->second;
    }
}

void
MulticastSuppression::recordData(Data data)
{
    auto name = data.getName(); //.getPrefix(-1); //removing nounce
    NFD_LOG_INFO("Data to check/record " << name);
    auto it = m_dataHistory.find(name);
    if (it == m_dataHistory.end())
    {
        NFD_LOG_INFO("Inserting data " << name << " into the map");
        m_dataHistory.emplace(name, 1);

        time::milliseconds entryLifetime = DEFAULT_INSTANT_LIFETIME;
        NFD_LOG_INFO("Erasing the data from the map in : " << entryLifetime); 
        setUpdateExpiration(entryLifetime, name, 'd');
    }
    else
    {
       NFD_LOG_INFO("Counter for  data " << name << " incremented"); 
        ++it->second;
    }
    // need to check if we have the interest in the map
    // if present, need to remove it from the map
    ndn::Name name_cop = name;
    name_cop.appendNumber(0);
    auto itr_timer = m_objectExpirationTimer.find(name_cop);
    if (itr_timer != m_objectExpirationTimer.end()) {
        NFD_LOG_INFO("Data overheard, deleting interest " <<name << " from the map");
        itr_timer->second.cancel();
        // schedule deletion now
        if (m_interestHistory.count(name) > 0) {
            updateMeasurement(name, 'i');
            m_interestHistory.erase(name);
            NFD_LOG_INFO("Interest successfully deleted from the history " <<name);
            }
    }
}

void
MulticastSuppression::setUpdateExpiration(time::milliseconds entryLifetime, Name name, char type)
{
  auto vec = getRecorder(type);
  auto eventId = getScheduler().schedule(entryLifetime, [=]  {
      if (vec->count(name) > 0) {
          //  record interest into moving average
          updateMeasurement(name,  type);
          vec->erase(name);
           NFD_LOG_INFO("Name: " << name << " type: " << type << " expired, and deleted from the instant history");
          }
        });

    name =  (type == 'i') ? name.appendNumber(0) : name.appendNumber(1);
    auto itr_timer = m_objectExpirationTimer.find(name);
    if (itr_timer != m_objectExpirationTimer.end()) {
            NFD_LOG_INFO("Updating timer for name: " << name << "type: " << type);
            itr_timer->second.cancel();
            itr_timer->second = eventId;
        }
    else {
        m_objectExpirationTimer.emplace(name, eventId);
    }
}

void
MulticastSuppression::print_map(std::map <Name, int> _map, char type)
{
    for (auto& it: _map)
    {
        NFD_LOG_INFO ("Type: " << type  << " , Object name: " << it.first  << " , dup count: "  << it.second);
    }
}

void
MulticastSuppression::updateMeasurement(Name name, char type)
{
    auto vec = getEMARecorder(type);
    auto duplicateCount = getDuplicateCount(name, type);
    NDN_LOG_INFO("Name:  " << name << " Duplicate Count: " << duplicateCount << " type: " << type);
    // granularity = name - last component e.g. /a/b --> /a
    name = name.getPrefix(-1);
    auto it = vec->find(name);
    if (it == vec->end())
    {
        NFD_LOG_INFO("Creating EMA record for name: " << name << " type: " << type);
        auto expirationId = getScheduler().schedule(MAX_MEASURMENT_INACTIVE_PERIOD, [=]  {
                                        if (vec->count(name) > 0)
                                            vec->erase(name);
                                    });
        auto& emaEntry = vec->emplace(name, std::make_shared<EMAMeasurements>()).first->second;
        emaEntry->setEMAExpiration(expirationId);
        emaEntry->addUpdateEMA(duplicateCount);
    }
    else
    {
        NFD_LOG_INFO("Updating EMA record for name: " << name << " type: " << type);
        it->second->getEMAExpiration().cancel();
        auto expirationId = getScheduler().schedule(MAX_MEASURMENT_INACTIVE_PERIOD, [=]  {
                                            if (vec->count(name) > 0)
                                                vec->erase(name); 
                                        });

        it->second->setEMAExpiration(expirationId);
        it->second->setLastDuplicateCount(duplicateCount); // duplicate count doesnt make sense. EMA granularity is -1, 
        // duplicateCount is of whole perfix, so lastDuplicate count will also be of the last name rather not of the -1 granular prefix
        it->second->addUpdateEMA(duplicateCount);
    }
}

// sum the average for higher prefix granularity
// for example EMA for /a/001, /a/002 will be sum and returned
//  this function will get the EMA for all the name matching "the prefix granularity",  and returns the average
float
MulticastSuppression::getMovingAverage(Name name, char type)
{
    auto vec = getEMARecorder(type);
    auto it = vec->find(name.getPrefix(-1)); //granularity -1
    return (it != vec->end()) ?  it->second->getEMACurrent() : 0.0 ;
}

time::milliseconds
MulticastSuppression::getDelayTimer(Name name, char type)
{
    auto vec = getEMARecorder(type);
    auto it = vec->find(name.getPrefix(-1)); //granularity -1
    auto suppressionTimer = (it != vec->end()) ?  it->second->getCurrentSuppressionTime() : static_cast<time::milliseconds>(getRandomNumber(5)); // if no measurement,  get a random number between 0, 5
    NFD_LOG_INFO("Suppression timer for name: " << name << " and type: "<< type << " = " << suppressionTimer);
    return suppressionTimer;
}

} //namespace ams
} //namespace face
} //namespace nfd
