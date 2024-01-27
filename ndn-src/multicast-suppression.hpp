#include "core/common.hpp"
#include <chrono>
#include <ndn-cxx/util/random.hpp>

#ifndef NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP
#define NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP

namespace nfd {
namespace face {
namespace ams {

int
getSeed();

int
getRandomNumber(int upperBound);

/* Simple trie implementaiton for the name tree, and prefix match */
class NameTree
{
public:
  // std::map<char, NameTree*> children;
  std::map<char, std::unique_ptr<NameTree>> children;
  bool isLeaf;
  double suppressionTime;

  NameTree();

  void
  insert(const std::string& prefix, double value);

  double
  longestPrefixMatch(const std::string& prefix); //longest prefix match

  time::milliseconds
  getSuppressionTimer(const std::string& prefix);

};

class EMAMeasurements
{

public:
  EMAMeasurements(double expMovingAverage, uint8_t lastDuplicateCount, double suppressionTime);

  void
  addUpdateEMA(uint8_t duplicateCount, bool wasForwarded);

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
  updateDelayTime(bool wasForwarded);

  double
  getCurrentSuppressionTime()
  {
    return m_currentSuppressionTime;
  }

private:
  double m_expMovingAveragePrev;
  double m_expMovingAverageCurrent;
  double m_currentSuppressionTime;
  scheduler::EventId m_expirationId;
  uint8_t m_lastDuplicateCount;
  uint8_t m_maxDuplicateCount;
  double m_minSuppressionTime;
  double m_ssthress;
  uint8_t m_ignoreDuplicateRecoring;
};


class MulticastSuppression
{
public:

  struct ObjectHistory
  {
    uint8_t counter;
    bool isForwarded;
  };

  void
  recordInterest(const Interest& interest, bool isForwarded = false);

  void
  recordData(const Data& data, bool isForwarded = false);

  uint8_t
  getDuplicateCount(const Name name, char type)
  {
    auto temp_map = getRecorder(type);
    auto it = temp_map->find(name);
    if (it != temp_map->end())
      return it->second.counter;
    return 0;
  }

  std::map<Name, ObjectHistory>*
  getRecorder(char type)
  {
    return (type == 'i') ? &m_interestHistory : &m_dataHistory;
  }

  std::map<Name, std::shared_ptr<EMAMeasurements>>*
  getEMARecorder(char type)
  {
    return (type =='i') ? &m_EMA_interest : &m_EMA_data;
  }

  NameTree*
  getNameTree(char type)
  {
    return (type =='i') ? &m_interestNameTree : &m_dataNameTree;
  }

  bool
  interestInflight(const Interest& interest) const
  {
    auto name = interest.getName();
    return (m_interestHistory.find(name) != m_interestHistory.end() );
  }

  bool
  dataInflight(const Data& data) const
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
updateMeasurement(Name name, char type);

// Set interest or data expiration
void
setUpdateExpiration(time::milliseconds entryLifetime, Name name, char type);

time::milliseconds
getDelayTimer(Name name, char type);

bool
getForwardedStatus(const ndn::Name& prefix, char type)
{
  auto recorder = getRecorder(type);
  auto it = recorder->find(prefix);
  // if record exist, send whatever is the status else, send false
  return it != recorder->end() ? it->second.isForwarded : false;
}

private:
  std::map<ndn::Name, ObjectHistory> m_dataHistory;
  std::map<ndn::Name, ObjectHistory> m_interestHistory;
  std::map <Name, scheduler::EventId> m_objectExpirationTimer;
  std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_data;
  std::map<Name, std::shared_ptr<EMAMeasurements>> m_EMA_interest;
  NameTree m_dataNameTree;
  NameTree m_interestNameTree;
};
} //namespace ams
} //namespace face
} //namespace nfd

#endif // NFD_DAEMON_FACE_SUPPRESSION_STRATEGY_HPP
