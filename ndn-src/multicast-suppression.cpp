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

/* This is 2*MAX_PROPOGATION_DELAY.  Basically, when a nodes (C1) forwards a packet, it will take 15ms to reach
its neighbors (C2). The packet will be recevied by the neighbors and they will suppress their forwarding. In case,
if the neighbor didnt received the packet from C1 in 15 ms, it will forward its own packet. Now, the actual duplicate count 
in the network is 2, both nodes C1 & C2 should record dc = 2. For this to happen, it takes about 15ms for the packet from C2 to 
reach C1. Thus, DEFAULT_INSTANT_LIFETIME = 30ms*/

const time::milliseconds DEFAULT_INSTANT_LIFETIME = 30_ms;
const double DUPLICATE_THRESHOLD = 1.5; // parameter to tune
const double ADATIVE_DECREASE = 10.0;
const double MULTIPLICATIVE_INCREASE = 1.3;

// in milliseconds ms
const double minSuppressionTime = 5.0f;
const double maxSuppressionTime= 15000.0f;
static bool seeded = false;

unsigned int UNSET = -1234;
int CHARACTER_SIZE = 126;
int maxIgnore = 3;

/* only for experimentation, will be removed later */
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

NameTree::NameTree()
: isLeaf(false)
, suppressionTime(UNSET)
{
}

void
NameTree::insert(std::string prefix, double value)
{
	auto node = this; // this should be root
	for (unsigned int i = 0; i < prefix.length(); i++)
	{
		auto index = prefix[i] - 'a';
		if (!node->children[index])
			node->children[index] = new NameTree();

        if (i == prefix.length() - 1)
		    node->suppressionTime = value;

		node = node->children[index];
	}
  // leaf node
	node->isLeaf = true;
}

double
NameTree::longestPrefixMatch(const std::string& prefix) // longest prefix match
{
  auto node = this;
  double lastValueFound = UNSET;
  for (unsigned int i=0; i < prefix.length(); i++)
  {
      auto index = prefix[i] - 'a';
      if (prefix[i+1] == '/') // key at i is the available prefix if the next item is /
        lastValueFound = node->suppressionTime;

      if (!node->children[index])
        return lastValueFound;
    
      if (i == prefix.length() - 1)
		    lastValueFound = node->suppressionTime;

      node = node->children[index];
  }
  return lastValueFound;
}

time::milliseconds
NameTree::getSuppressionTimer(const std::string& name)
{ 
    double val, suppressionTime;
    val = longestPrefixMatch(name);
    suppressionTime = (val == UNSET) ? minSuppressionTime : val;
    time::milliseconds suppressionTimer (getRandomNumber(static_cast<int> (suppressionTime))); // timer is randomized value
    NFD_LOG_INFO("Suppression time: " << suppressionTime << " Suppression timer: " << suppressionTimer);
    return suppressionTimer;
}

/* objectName granularity is (-1) name component */
EMAMeasurements::EMAMeasurements(double expMovingAverage = 0, int lastDuplicateCount = 0, double suppressionTime = MAX_PROPOGATION_DELAY)
: m_expMovingAveragePrev (expMovingAverage)
, m_expMovingAverageCurrent (expMovingAverage)
, m_currentSuppressionTime(suppressionTime)
, m_computedMaxSuppressionTime(suppressionTime)
, m_lastDuplicateCount(lastDuplicateCount)
, m_ssthress(suppressionTime/2.0)
, ignore(0)
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
    this->ignore  = (duplicateCount > m_lastDuplicateCount) ? (ignore +1) : 0;

    if(ignore > 0 && ignore < maxIgnore) {
        NDN_LOG_INFO("Duplicate count: " << duplicateCount << " m_lastdup: " << m_lastDuplicateCount << " ignore: " << ignore);
        return;
    }

    // reset duplicateCount and ignore
    m_lastDuplicateCount = duplicateCount;
    this->ignore = 0;

    m_expMovingAveragePrev = m_expMovingAverageCurrent;
    if (m_expMovingAverageCurrent == 0) {
        this->m_expMovingAverageCurrent = duplicateCount;
    }
    else {
        this->m_expMovingAverageCurrent =  round ((DISCOUNT_FACTOR*duplicateCount 
                                                                       + (1 - DISCOUNT_FACTOR)*this->m_expMovingAverageCurrent)*100.0)/100.0;
        updateDelayTime();
        }

    NFD_LOG_INFO("Moving average" << " before: " << m_expMovingAveragePrev
                                                                 << " after: " << m_expMovingAverageCurrent
                                                                 << " duplicate count: " << duplicateCount
                                                                 << " suppression time: "<< this->m_currentSuppressionTime
                                                                 << " computed max: " << this->m_computedMaxSuppressionTime
                                                                 << " ssthres: " << this->m_ssthress);
}

void
EMAMeasurements::updateDelayTime()
{
    double temp;
    // Implicit action: if you haven’t reached the goal, but your moving average is decreasing then do nothing.
    if (m_expMovingAverageCurrent > DUPLICATE_THRESHOLD && 
        m_expMovingAverageCurrent >=m_expMovingAveragePrev ) {
            temp = m_currentSuppressionTime*MULTIPLICATIVE_INCREASE; // ADATIVE_DECREASE;
        }
    else if (m_expMovingAverageCurrent <= DUPLICATE_THRESHOLD) {
        temp = (m_currentSuppressionTime > m_ssthress) ? m_currentSuppressionTime - ADATIVE_DECREASE
                                                                                           : m_currentSuppressionTime - 1.0;
        // temp = m_currentSuppressionTime - ADATIVE_DECREASE;
    }
    else
        temp = m_currentSuppressionTime;

    m_currentSuppressionTime =  std::min(std::max(minSuppressionTime, temp), maxSuppressionTime);

    // updated the max if its smaller than current suppression time
    if (m_currentSuppressionTime > m_computedMaxSuppressionTime) 
    {
        m_computedMaxSuppressionTime = m_currentSuppressionTime;
        setSSthress(m_computedMaxSuppressionTime);
    }
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
MulticastSuppression::updateMeasurement(Name name, char type)
{
    // if the measurment expires, can't the name stay with EMA = 0? so that we dont have to recreate it again later
    auto vec = getEMARecorder(type);
    auto nameTree = getNameTree(type);
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
                                            // dont delete the entry in the nametree, just unset the value
                                        nameTree->insert(name.toUri(), UNSET);
                                    });
        auto& emaEntry = vec->emplace(name, std::make_shared<EMAMeasurements>()).first->second;
        emaEntry->setEMAExpiration(expirationId);
        emaEntry->addUpdateEMA(duplicateCount);
        nameTree->insert(name.toUri(), emaEntry->getCurrentSuppressionTime());
    }
    else
    {
        NFD_LOG_INFO("Updating EMA record for name: " << name << " type: " << type);
        it->second->getEMAExpiration().cancel();
        auto expirationId = getScheduler().schedule(MAX_MEASURMENT_INACTIVE_PERIOD, [=]  {
                                            if (vec->count(name) > 0)
                                                vec->erase(name); 
                                                // set the value in the nametree = -1 
                                            nameTree->insert(name.toUri(), UNSET);
                                        });

        it->second->setEMAExpiration(expirationId);
        it->second->addUpdateEMA(duplicateCount);
        nameTree->insert(name.toUri(), it->second->getCurrentSuppressionTime());
    }
}

time::milliseconds
MulticastSuppression::getDelayTimer(Name name, char type)
{
    NFD_LOG_INFO("Getting supperssion timer for name: " << name);
    auto nameTree = getNameTree(type);
    auto suppressionTimer = nameTree->getSuppressionTimer(name.getPrefix(-1).toUri());
    NFD_LOG_INFO("Suppression timer for name: " << name << " and type: "<< type << " = " << suppressionTimer);
    return suppressionTimer;
}

} //namespace ams
} //namespace face
} //namespace nfd
