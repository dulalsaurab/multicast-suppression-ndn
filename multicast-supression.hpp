#include "face-table.hpp"
#include "forwarder-counters.hpp"

#ifndef NFD_DAEMON_FW_AMS_MULTICAST_SUPPRESSION_HPP
#define NFD_DAEMON_FW_AMS_MULTICAST_SUPPRESSION_HPP

namespace nfd {
namespace fw {
namespace ams {

class EMAMeasurements
{

public:
  EMAMeasurements(Name name, double expMovingAverageDc, int lastDuplicateCount);

  void
  addUpdateEMA(int duplicateCount);

  scheduler::EventId&
  getEMAExpiration()
  {
    return this->expirationId;
  }
  void
  setEMAExpiration(scheduler::EventId& expirationId)
  {
    this->expirationId = expirationId;
  }

  float
  getEMA()
  {
    return this->expMovingAverageDc;
  }

  void
  setLastDuplicateCount(int duplicateCount)
  {
    this->lastDuplicateCount = duplicateCount;
  }

private:
  Name name;
  double expMovingAverageDc;
  scheduler::EventId expirationId;
  int lastDuplicateCount; // not sure if needed, lets have it here for now
};


class MulticastSuppression
{
public:
  void 
  recordInterest(const Interest interest);

  void
  recordData(const Data data);

  int
  getDuplicateCount(const Name name, char type)
  {
    auto temp_map = getRecorder(type);
    auto it = temp_map->find(name);
    if (it != temp_map->end())
      return it->second;
    return 0;
  }

  std::map<Name, int>*
  getRecorder(char type)
  {
    return (type == 'i') ? &m_interestHistory : &m_dataHistory;
  }

  std::map<Name, std::shared_ptr<EMAMeasurements>>*
  getEMARecorder(char type)
  {
    return (type =='i') ? &m_EMA_interest : &m_EMA_data;
  }

  // below functions can be merged to one
  bool
  interestInflight(const Interest interest) const
  {
    auto name = interest.getName(); //.getPrefix(-1); //need to remove the nounce before checking
    return (m_interestHistory.find(name) != m_interestHistory.end() );
  }

  bool
  dataInflight(const Data data) const
  {
    auto name = data.getName(); //.getPrefix(-1); //need to remove the nounce before checking
    return (m_dataHistory.find(name) != m_dataHistory.end());
  }

time::milliseconds
getRandTime()
  {
    return time::milliseconds(0 + (std::rand() % (100)));
  }

void
print_map(std::map <Name, int> _map, char type);

void
updateMeasurement(Name name, char type);

float
getMovingAverage(Name prefix, char type);

// set interest or data expiration
void
setUpdateExpiration(time::milliseconds entryLifetime, Name name, char type);

private:
    std::map <Name, int> m_dataHistory;
    std::map <Name, int> m_interestHistory;
    std::map <Name, scheduler::EventId> m_objectExpirationTimer;
    // std::map <Name, EMAMeasurements* > m_EMA_interest;
    // std::map <Name, EMAMeasurements* > m_EMA_data;
    std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_data;
    std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_interest;
};
} //namespace ams
} //namespace fw
} //namespace nfd

#endif // NFD_DAEMON_FW_SUPPRESSION_STRATEGY_HPP
