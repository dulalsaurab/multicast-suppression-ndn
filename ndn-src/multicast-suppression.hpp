#include "core/common.hpp"
#include <chrono>
#include <ndn-cxx/util/random.hpp>

#ifndef NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP
#define NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP

namespace nfd {
namespace face {
namespace ams {

struct InstanceHistory
{
  ndn::Name serviceName;
  ndn::Name applicationPrefix;
  ndn::time::seconds serviceLifetime;
  ndn::time::system_clock::TimePoint publishTimestamp;
  std::map<std::string, std::string> serviceMetaInfo;
};


class EMAMeasurements
{

public:
  EMAMeasurements(double expMovingAverage, int lastDuplicateCount, double suppressionTime);

  void
  addUpdateEMA(int duplicateCount);

  scheduler::EventId&
  getEMAExpiration()
  {
    return this->m_expirationId;
  }
  void
  setEMAExpiration(scheduler::EventId& expirationId)
  {
    this->m_expirationId = expirationId;
  }

  float
  getEMACurrent()
  {
    return this->m_expMovingAverageCurrent;
  }

  float
  getEMAPrev()
  {
    return this->m_expMovingAveragePrev;
  }

  void
  setLastDuplicateCount(int duplicateCount)
  {
    this->m_averageDuplicateCount = duplicateCount;
  }

  int
  getAverageDuplicateCount()
  {
    return this->m_averageDuplicateCount;
  }

  void
  updateDelayTime();

  time::milliseconds
  getCurrentSuppressionTime();

private:
  Name m_name;
  double m_expMovingAveragePrev;
  double m_expMovingAverageCurrent;
  double m_currentSuppressionTime;
  scheduler::EventId m_expirationId;
  int m_averageDuplicateCount; // not sure if needed, lets have it here for now
  const unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

  ndn::random::RandomNumberEngine& m_rng;
  std::uniform_int_distribution<int> m_rangeUniformRandom;
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

  bool
  interestInflight(const Interest interest) const
  {
    auto name = interest.getName();
    return (m_interestHistory.find(name) != m_interestHistory.end() );
  }

  bool
  dataInflight(const Data data) const
  {
    auto name = data.getName();
    return (m_dataHistory.find(name) != m_dataHistory.end());
  }

time::milliseconds
getRandomTime()
  {
    return time::milliseconds(1 + (std::rand() % (10)));
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

time::milliseconds
getDelayTimer(Name name, char type);

private:
    std::map <Name, int> m_dataHistory;
    std::map <Name, int> m_interestHistory;
    std::map <Name, scheduler::EventId> m_objectExpirationTimer;
    std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_data;
    std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_interest;
};
} //namespace ams
} //namespace face
} //namespace nfd

#endif // NFD_DAEMON_FACE_SUPPRESSION_STRATEGY_HPP
