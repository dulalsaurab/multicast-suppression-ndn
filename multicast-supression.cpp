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
const time::milliseconds MEASURMENT_LIFETIME = 300000_ms; // 5 minutes 

void
MulticastSuppression::recordInterest(const Interest interest)
{
    auto name = interest.getName().getPrefix(-1); //removing nonce
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
    scheduler::EventId eventId;
    eventId = getScheduler().schedule(entryLifetime, [=]  {
                    auto temp_itr = m_interestHistory.find(name);
                    if (temp_itr != m_interestHistory.end())
                         {
                            //  record interest into moving average
                            updateMeasurment(name,  'i', getDuplicateCount(name, 'i'));
                            m_interestHistory.erase(name);
                         }
        });

    name.appendNumber(0); // 0 for interest
    auto itr_timer = m_objectExpirationTimer.find(name);
    if (itr_timer != m_objectExpirationTimer.end()) {
            NFD_LOG_INFO("Updating timer for interest");
            itr_timer->second.cancel();
            itr_timer->second = eventId;
        }
    else {
        m_objectExpirationTimer.emplace(name, eventId);
    }
    print_map (m_interestHistory, 'i');
}

void
MulticastSuppression::recordData(const Data data)
{
    auto name = data.getName().getPrefix(-1); //removing nounce
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
                    auto temp_itr = m_interestHistory.find(name);
                    if (temp_itr != m_interestHistory.end()) {
                        // need to call moving average
                        updateMeasurment(name, 'i', getDuplicateCount(name, 'i'));
                        m_interestHistory.erase(name);
                        }
                    print_map(m_interestHistory, 'i'); 
        // });

    }

    time::milliseconds entryLifetime = data.getFreshnessPeriod();
    NFD_LOG_INFO("Erasing the data from the map in : " << entryLifetime); 
    scheduler::EventId eventId;
    eventId = getScheduler().schedule(entryLifetime, [=]  {
                    auto temp_itr = m_dataHistory.find(name);
                    if (temp_itr != m_dataHistory.end()) {
                        updateMeasurment(name, 'd', getDuplicateCount(name, 'd'));
                        m_dataHistory.erase(name); //cannot use above ** it ** because of index changes and memory leak  
                    }
        });

    name.appendNumber(1); //1 for data
    itr_timer = m_objectExpirationTimer.find(name);
    if (itr_timer != m_objectExpirationTimer.end()) {
            NFD_LOG_INFO("Updating timer for data");
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

// EMA section
// objectName granularity is (-1) name component
void
MulticastSuppression::updateMeasurment(Name name, char type, int duplicateCount)
{
  std::map <Name, Measurments> *vec;
  vec = (type =='i') ? &m_EMA_interest : &m_EMA_data;

auto it = vec->find(name);
if (it != vec->end())
{ 
    addUpdateEMA(it->second, duplicateCount);
    // update expiration time
    NFD_LOG_INFO ("if: Moving average for type: " <<  type<<  " name: " << name << "=" << it->second.expMovingAverageDc );
    auto eventId = getScheduler().schedule(MEASURMENT_LIFETIME, [=]  {
                    auto temp_itr = vec->find(name);
                    if (temp_itr != vec->end())
                        vec->erase(name);
     });
     it->second.expirationId.cancel();
     it->second.expirationId = eventId;
}
else
{
    Measurments m;
    // if no records, moving average = duplicateCount
    m.expMovingAverageDc = duplicateCount;
    // addUpdateEMA(m, duplicateCount); 
    NFD_LOG_INFO ("else: Moving average for type: " << type << " name: " << name << "=" << m.expMovingAverageDc );
    auto eventId = getScheduler().schedule(MEASURMENT_LIFETIME, [=]  {
                    auto temp_itr = vec->find(name);
                    if (temp_itr != vec->end())
                        vec->erase(name);
     });
    m.expirationId = eventId; 
    vec->emplace(name, m);
}
}
    /*
    // we compute exponential moving average to git higher preference to recent data
    EMA = Dt ;duplicate count if t = 1
    EMA  = alpha*Dt + (1 - alpha) * EMA t-1 
  */
  void
  MulticastSuppression::addUpdateEMA(Measurments& m, int duplicateCount)
  {
    m.expMovingAverageDc =  DISCOUNT_FACTOR*duplicateCount 
                                                    + (1 - DISCOUNT_FACTOR)*m.expMovingAverageDc;
  }


float
MulticastSuppression::getEMA(Name prefix, char type)
{
    // this will sum the average for higher granular prefix
    // for example EMA for /a/001, /a/002 will be sum and returned
    std::map <Name, Measurments> *vec;
    vec = (type =='i') ? &m_EMA_interest : &m_EMA_data;

    float sum = 0.0;
    int count = 0;
    for (auto it = vec->lower_bound(prefix);
    it != std::end(*vec) && it->first.compare(0, prefix.size(), prefix) == 0; ++it)
    {
        sum += it->second.expMovingAverageDc;
        count++;
    }
    return (sum == 0.0)? sum : sum/count;
}

} //namespace ams
} //namespace fw
} //namespace nfd
