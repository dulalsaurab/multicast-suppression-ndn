// build a suppression strategy for each 
#include "multicast-suppression.hpp"
#include <tuple>
#include <functional>
#include <iostream>
#include "common/global.hpp"
#include "common/logger.hpp"
#include <algorithm>
#include <numeric>

namespace nfd {
namespace fw {
namespace ams {

NFD_LOG_INIT(MulticastSuppression);

float DISCOUNT_FACTOR = 0.8;
const time::milliseconds MEASUREMENT_LIFETIME = 300_s; // 5 minutes 


// EMA section
// objectName granularity is (-1) name component
EMAMeasurements::EMAMeasurements(Name name, double expMovingAverageDc = 0.0, int lastDuplicateCount = 0)
: name(name)
, expMovingAverageDc (expMovingAverageDc)
, lastDuplicateCount (lastDuplicateCount)
{
}

/*
// we compute exponential moving average to git higher preference to recent data
EMA = Dt ;duplicate count if t = 1
EMA  = alpha*Dt + (1 - alpha) * EMA t-1
*/
void
EMAMeasurements::addUpdateEMA(int duplicateCount)
{
    NFD_LOG_INFO("moving average before: " << this->expMovingAverageDc);
    this->expMovingAverageDc =  DISCOUNT_FACTOR*duplicateCount
                                                       + (1 - DISCOUNT_FACTOR)*this->expMovingAverageDc;
     NFD_LOG_INFO("moving average after: " << this->expMovingAverageDc << ": duplicate count"<<  duplicateCount);
}

void
MulticastSuppression::recordInterest(const Interest interest)
{
    auto name = interest.getName(); //.getPrefix(-1); //removing nonce
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
    // print_map (m_interestHistory, 'i');
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
        // getScheduler().schedule(0_ms, [=]  {
        if (m_interestHistory.count(name) > 0) {
            // need to call moving average
            updateMeasurement(name, 'i');
            m_interestHistory.erase(name);
            NFD_LOG_INFO("Interest successfully deleted from the history " <<name);
            }
        // print_map(m_interestHistory, 'i');
        // });
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
        it->second->setLastDuplicateCount(duplicateCount);
        it->second->addUpdateEMA(duplicateCount);
    }

}

// sum the average for higher prefix granularity
// for example EMA for /a/001, /a/002 will be sum and returned
//  this function will get the EMA for all the name matching "the prefix granularity",  and returns the average
float
MulticastSuppression::getMovingAverage(Name prefix, char type)
{
    auto vec = getEMARecorder(type);
    float sum = 0.0;
    int count = 0;
    for (auto it = vec->lower_bound(prefix); 
    it != std::end(*vec) && it->first.compare(0, prefix.size(), prefix) == 0; ++it)
    {
        sum += it->second->getEMA();
        count++;
    }
    return (sum == 0.0) ? sum : sum/count;
}

} //namespace ams
} //namespace fw
} //namespace nfd
