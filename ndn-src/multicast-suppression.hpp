#include "core/common.hpp"
#include <chrono>
#include <ndn-cxx/util/random.hpp>

#ifndef NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP
#define NFD_DAEMON_FACE_AMS_MULTICAST_SUPPRESSION_HPP

namespace nfd {
namespace face {
namespace ams {

/* Simple trie implementaiton for the name tree, and prefix match */
class NameTree
{
public:
  std::map<char, NameTree*> children;
  bool isLeaf;
  double suppressionTime;

  NameTree();

  void 
  insert(std::string prefix, double value);

  double
  longestPrefixMatch(const std::string& prefix); //longest prefix match

  time::milliseconds
  getSuppressionTimer(const std::string& prefix);

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
  updateDelayTime();

  double
  getCurrentSuppressionTime()
  {
    return m_currentSuppressionTime;
  }

  void
  setSSthress(double val, int factor = 2)
  {
    m_ssthress = val/factor;
  }

private:
  double m_expMovingAveragePrev;
  double m_expMovingAverageCurrent;
  double m_currentSuppressionTime;
  scheduler::EventId m_expirationId;
  double m_computedMaxSuppressionTime;
  int m_lastDuplicateCount;
  double m_ssthress;
  int ignore;
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

  NameTree*
  getNameTree(char type)
  {
    return (type =='i') ? &m_interestNameTree : &m_dataNameTree; 
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
updateMeasurement(Name name, char type);

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
    NameTree m_dataNameTree;
    NameTree m_interestNameTree;
};
} //namespace ams
} //namespace face
} //namespace nfd

#endif // NFD_DAEMON_FACE_SUPPRESSION_STRATEGY_HPP
