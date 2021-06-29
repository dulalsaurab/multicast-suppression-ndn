#include "face-table.hpp"
#include "forwarder-counters.hpp"

#ifndef NFD_DAEMON_FW_AMS_MULTICAST_SUPPRESSION_HPP
#define NFD_DAEMON_FW_AMS_MULTICAST_SUPPRESSION_HPP

namespace nfd {
namespace fw {
namespace ams {

struct Measurments
{
  double expMovingAverageDc;
  scheduler::EventId expirationId;
};

class MulticastSuppression
{
public:

  void 
  recordInterest(const Interest interest);

  void
  recordData(const Data data);

  int
  getDuplicateCount(const Name name, char type) const
  {
    auto temp_map = (type =='i') ? m_interestHistory : m_dataHistory;
    auto it = temp_map.find(name);
    if (it != temp_map.end())
      return it->second;
    return 0;
  }

  std::map<Name, int>
  getRecorder(char type)
  {
    if (type == 'i') return m_interestHistory;
    else return m_dataHistory;
  }

  // below functions can be merged to one
  bool
  interestInflight(const Interest interest) const
  {
    auto name = interest.getName().getPrefix(-1); //need to remove the nounce before checking
    return (m_interestHistory.find(name) != m_interestHistory.end() );
  }

  bool
  dataInflight(const Data data) const
  {
    auto name = data.getName().getPrefix(-1); //need to remove the nounce before checking
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
  addUpdateEMA(Measurments& m, int duplicateCount);

  // EMA code
  void
  updateMeasurment(Name name, char type, int duplicateCount);

  float
  getEMA(Name name, char type);


private:
    std::map <Name, int> m_dataHistory;
    std::map <Name, int> m_interestHistory;
    std::map <Name, scheduler::EventId> m_objectExpirationTimer;
    std::map <Name, Measurments> m_EMA_interest;
    std::map <Name, Measurments> m_EMA_data;

};
} //namespace ams
} //namespace fw
} //namespace nfd

#endif // NFD_DAEMON_FW_SUPPRESSION_STRATEGY_HPP
