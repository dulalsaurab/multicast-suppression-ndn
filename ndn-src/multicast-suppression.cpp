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

namespace nfd {
namespace face {
namespace ams {

NFD_LOG_INIT(MulticastSuppression);

float DISCOUNT_FACTOR = 0.6;
const time::milliseconds MEASUREMENT_LIFETIME = 300_s; // 5 minutes 

// in miliseconds ms
const double minSuppressionTime = 5.0; 
const double maxSuppressionTime= 100.0;
const int successfulTransmission = 1; // S, we are taking 1 for now

// EMA section
// objectName granularity is (-1) name component
bool EMAMeasurements::seeded = false;
EMAMeasurements::EMAMeasurements(Name name, double expMovingAverage = 1.0, int lastDuplicateCount = 0, double delayTime = 1.0)
: m_name(name)
, m_expMovingAveragePrev (expMovingAverage)
, m_expMovingAverageCurrent (expMovingAverage)
, m_averageDuplicateCount (lastDuplicateCount) // need to change: average for all the name, e.g. name (/a/b, = 2 /a/c = 2), /a = (2+2)/2 = 2 
, m_currentSuppressionTime(delayTime)
{
    if (!seeded) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        NFD_LOG_INFO ("seeded " << seed);
        std::srand(seed);
        seeded = true;
    }
}
/*
 we compute exponential moving average to give higher preference to the most recent interest/data
 EMA = duplicate count if t = 1
 EMA  = alpha*Dt + (1 - alpha) * EMA t-1
*/
void
EMAMeasurements::addUpdateEMA(int duplicateCount)
{
    
    NFD_LOG_INFO("moving average before: " << m_expMovingAverageCurrent);
    m_expMovingAveragePrev = m_expMovingAverageCurrent;
    m_expMovingAverageCurrent =  DISCOUNT_FACTOR*duplicateCount
                                                       + (1 - DISCOUNT_FACTOR)*m_expMovingAveragePrev;
     NFD_LOG_INFO("moving average after: " << m_expMovingAverageCurrent << "\n duplicate count "<<  duplicateCount);
    updateDelayTime();
}

/* 
  Computes an estimated wait time for a given prefix
  Algorithm: History Based Probabilistic Backoff Algorithm 
  (https://thescipub.com/pdf/ajeassp.2012.230.236.pdf)
  Contention Window (CW)
*/
void
EMAMeasurements::updateDelayTime()
{
    auto collionProbability = m_expMovingAverageCurrent/(1+m_expMovingAverageCurrent);   // this can be 0/0
    // auto discountedCollisionProb =  DISCOUNT_FACTOR*collionProbability;
    // try
    // {
    NFD_LOG_INFO("collionProbability: "<< collionProbability);
    auto temp = std::min(maxSuppressionTime, 
                                       m_currentSuppressionTime*pow(2, m_expMovingAverageCurrent));
    // source: https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/
    m_currentSuppressionTime = 0.3*temp +  std::rand() % (static_cast<int>(temp*0.7) + 1); //added +1 to avoid /0 case 
 
    //  need to think more about the second component and what to multiply with, now lets use collisionProb
    // float alpha;
    // auto previousSuppressionTime = this->m_currentSuppressionTime;
    // float collionProbability = this->m_expMovingAverageCurrent/(1+this->m_expMovingAverageCurrent);   // this can't be 0/0
    // if (this->m_expMovingAverageCurrent < 2)
    // {
    //     // number of duplicate is in acceptable range
    //     alpha = -1 + collionProbability*2;
    //     this->m_currentSuppressionTime = std::min(maxSuppressionTime-1, previousSuppressionTime*pow(2, alpha)); 
    // }
    // else
    // {
    //     // number of collision decreased
    //     alpha = -1 + (1 - collionProbability)*2;
    //     this->m_currentSuppressionTime = std::max(minSuppressionTime+1, previousSuppressionTime*pow(2, alpha));
    // }
    NFD_LOG_INFO("New suppression time set: " << this->m_currentSuppressionTime);
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
    }
    else { 
        NFD_LOG_INFO("Counter for interest " << name << " incremented");
         ++it->second;
    }
    time::milliseconds entryLifetime = interest.getInterestLifetime();
    if (entryLifetime < time::milliseconds::zero())
        entryLifetime = ndn::DEFAULT_INTEREST_LIFETIME;

    NFD_LOG_INFO("Erasing the interest from the map in : " << entryLifetime); 
    setUpdateExpiration(entryLifetime, name, 'i');
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
    time::milliseconds entryLifetime = data.getFreshnessPeriod();
    NFD_LOG_INFO("Erasing the data from the map in : " << entryLifetime); 
    setUpdateExpiration(entryLifetime, name, 'd');
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
    NFD_LOG_INFO("++++++++");
    for (auto& it: _map)
    {
        NFD_LOG_INFO ("Type: " << type 
                                                      << " , Object name: " << it.first 
                                                      << " , dup count: "  << it.second
                                                    );
    }
    NFD_LOG_INFO("++++++++");
}

void
MulticastSuppression::updateMeasurement(Name name, char type)
{
    auto vec = getEMARecorder(type);
    auto duplicateCount = getDuplicateCount(name, type);
    // granularity = name - last component e.g. /a/b --> /a
    name = name.getPrefix(-1);
    auto it = vec->find(name);
    if (it == vec->end())
    {
        NFD_LOG_INFO("Creating EMA record for name: " << name << " type: " << type);
        auto expirationId = getScheduler().schedule(MEASUREMENT_LIFETIME, [=]  {
                                        if (vec->count(name) > 0)
                                            vec->erase(name);
                                    });
        auto& emaEntry = vec->emplace(name, std::make_shared<EMAMeasurements>(name, duplicateCount, duplicateCount)).first->second;
        emaEntry->setEMAExpiration(expirationId);
        emaEntry->addUpdateEMA(duplicateCount);
    }
    else
    {
        NFD_LOG_INFO("Updating EMA record for name: " << name << " type: " << type);
        it->second->getEMAExpiration().cancel();
        auto expirationId = getScheduler().schedule(MEASUREMENT_LIFETIME, [=]  {
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
    return (it != vec->end()) ?  it->second->getCurrentSuppressionTime() : getRandomTime(); // if no measurement, forward immediately (observation phase)
}

} //namespace ams
} //namespace face
} //namespace nfd
