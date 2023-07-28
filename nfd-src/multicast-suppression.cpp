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
#include <errno.h>

namespace nfd {
namespace face {
namespace ams {

NFD_LOG_INIT(MulticastSuppression);

double DISCOUNT_FACTOR = 0.125;
double MAX_PROPOGATION_DELAY = 5;
const time::milliseconds MAX_MEASURMENT_INACTIVE_PERIOD = 300_s; // 5 minutes

/* This is 2*MAX_PROPOGATION_DELAY.  Basically, when a nodes (C1) forwards a packet, it will take 15ms to reach
its neighbors (C2). The packet will be recevied by the neighbors and they will suppress their forwarding. In case,
if the neighbor didnt received the packet from C1 in 15 ms, it will forward its own packet. Now, the actual duplicate count
in the network is 2, both nodes C1 & C2 should record dc = 2. For this to happen, it takes about 15ms for the packet from C2 to
reach C1. Thus, DEFAULT_INSTANT_LIFETIME = 30ms*/

const time::milliseconds DEFAULT_INSTANT_LIFETIME = 10_ms; // 2*MAX_PROPOGATION_DELAY
const double DUPLICATE_THRESHOLD = 1.3; // parameter to tune
const double ADATIVE_DECREASE = 5.0;
const double MULTIPLICATIVE_INCREASE = 1.2;

// in milliseconds ms
// probably we need to provide sufficient time for other party to hear you?? MAX_PROPAGATION_DELAY??
const double minSuppressionTime = 10.0f;
const double maxSuppressionTime= 15000.0f;
static bool seeded = false;

unsigned int UNSET = -1234;
int CHARACTER_SIZE = 126;
int maxIgnore = 0;


char FIFO_OBJECT[] = "fifo_object_details";
char FIFO_VALUE[] = "fifo_suppression_value";

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
  if (!seeded)
  {
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

/* quick and dirty _FIFO*/

// _FIFO::_FIFO()
// // : m_fifo_suppression_value(fifo_suppression_value)
// // , m_fifo_object_details(fifo_object_details)
// {
//   // mknod(FIFO_OBJECT, S_IFIFO | 0666, 0);
//   mkfifo(FIFO_VALUE, 0666);
//   // mkfifo(FIFO_OBJECT, 0666);
// }

void
_FIFO::fifo_handler(const std::string& content)
{

  char buffer[1024];
  int fifo; 
  
  try {
    fifo = open(FIFO_OBJECT, O_WRONLY, 0666);
  }
  catch (const std::exception &e) {
    NFD_LOG_INFO("Unfortunately came here");
    std::cerr << e.what() << '\n';
  }

  if (fifo < 0)
  {
    NFD_LOG_INFO("Couldn't open fifo file");
    NFD_LOG_INFO("writer open failed: " << fifo << strerror(fifo));
    return;
  }
  try {
      NFD_LOG_INFO("This is the message to write: " << content);
      auto ret_code = write(fifo, content.c_str(), sizeof(content));
      NFD_LOG_INFO("return code: " << ret_code << " after write: " << content);
    }
    catch (const std::exception &e) {
      NFD_LOG_INFO("Unfortunately came here");
      std::cerr << e.what() << '\n';
    }
    auto ret = close(fifo);
    NFD_LOG_INFO("return code after close: " << ret);

    // wait for a response from python process
    auto pipeFile = open(FIFO_OBJECT, O_RDONLY);
    if (pipeFile == -1) {
      std::cerr << "Error opening named pipe." << std::endl;
      return;
    }
    int bytesRead = read(pipeFile, buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = '\0';
    NFD_LOG_INFO("Python: " << buffer);

    close(pipeFile);
}

time::milliseconds
_FIFO::fifo_read()
{
  std::cout << "Opened pipes" << std::endl;
  char buf[10];
  auto read_pipe = open(FIFO_VALUE, O_RDONLY);
  while (true)
  {
    read(read_pipe, &buf, sizeof(char) * 2);
    if (strlen(buf) == 0)
      break;
    else
    {
      std::string s(buf);
      std::cout << "received data from python: " << s << std::endl;
      return time::milliseconds(std::stoi(s));
    }
  }
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

    if (i == prefix.length() - 1) {
      node->suppressionTime = value;
      // node->suppressionTime = minSuppressionTime;
    }

    node = node->children[index];
  }
  // leaf node
  node->isLeaf = true;
}

// std::pair<double, double>
double
NameTree::longestPrefixMatch(const std::string& prefix) // longest prefix match
{
  auto node = this;
  double lastValueFound = UNSET;
  // double minSuppressionTime = 0;
  for (unsigned int i=0; i < prefix.length(); i++)
  {
    auto index = prefix[i] - 'a';
    if (prefix[i+1] == '/') // key at i is the available prefix if the next item is /
      {
        lastValueFound = node->suppressionTime;
        // minSuppressionTime = node->minSuppressionTime;
      }

    if (!node->children[index])
      return lastValueFound;
      // return std::make_pair(lastValueFound, minSuppressionTime);

    if (i == prefix.length() - 1)
      {
        lastValueFound = node->suppressionTime;
        // minSuppressionTime = node->minSuppressionTime;
      }

    node = node->children[index];
  }
  return lastValueFound;
  // return std::make_pair(lastValueFound, minSuppressionTime);;
}

time::milliseconds
NameTree::getSuppressionTimer(const std::string& name)
{
  double val, suppressionTime;
  val = longestPrefixMatch(name);
  suppressionTime = (val == UNSET) ? minSuppressionTime : val;
  // if (suppressionTime == 0) {
    // NDN_LOG_INFO("suppression time is zero");
    // return time::milliseconds (0);
  // }
  time::milliseconds suppressionTimer (getRandomNumber(static_cast<int> (2*suppressionTime))); // timer is randomized value
  NFD_LOG_INFO("Suppression time: " << suppressionTime << " Suppression timer: " << suppressionTimer);
  return suppressionTimer;
}

/* objectName granularity is (-1) name component
  m_lastForwardStaus is set to true if this node has successfully forwarded an interest or data
  else is set to false.
  start with 15ms, MAX propagation time, for a node to hear other node
  or start with 1ms, and let node find stable suppression time region?
*/
EMAMeasurements::EMAMeasurements(double expMovingAverage = 0, int lastDuplicateCount = 0, double suppressionTime = 1)
: m_expMovingAveragePrev (expMovingAverage)
, m_expMovingAverageCurrent (expMovingAverage)
, m_currentSuppressionTime(suppressionTime)
, m_computedMaxSuppressionTime(suppressionTime)
, m_lastDuplicateCount(lastDuplicateCount)
, m_maxDuplicateCount (1)
, m_minSuppressionTime(minSuppressionTime)
, ignore(0)
// , m_fifo()
{
}

/*
 we compute exponential moving average to give higher preference to the most recent interest/data
 EMA = duplicate count if t = 1
 EMA  = alpha*Dt + (1 - alpha) * EMA t-1
*/
void
EMAMeasurements::addUpdateEMA(int duplicateCount, bool wasForwarded, std::string name)
{
  ignore  = (duplicateCount > m_lastDuplicateCount) ? (ignore +1) : 0;

  if (ignore > 0 && ignore < maxIgnore) {
    NDN_LOG_INFO("Duplicate count: " << duplicateCount << " m_lastdup: " << m_lastDuplicateCount << " ignore: " << ignore);
    return;
  }

  // reset duplicateCount and ignore
  m_lastDuplicateCount = duplicateCount;
  ignore = 0;

  m_expMovingAveragePrev = m_expMovingAverageCurrent;
  if (m_expMovingAverageCurrent == 0) {
    m_expMovingAverageCurrent = duplicateCount;
  }
  else
  {
    m_expMovingAverageCurrent =  round ((DISCOUNT_FACTOR*duplicateCount + 
                                             (1 - DISCOUNT_FACTOR)*m_expMovingAverageCurrent)*100.0)/100.0;
    // if this node hadn't forwarded don't update the delay timer ???
  }
  if (m_maxDuplicateCount < duplicateCount) { m_maxDuplicateCount = duplicateCount; }

  if (m_maxDuplicateCount > 1)
    m_minSuppressionTime = (float) MAX_PROPOGATION_DELAY;
  else if (m_maxDuplicateCount == 1 && m_minSuppressionTime > 1)
      m_minSuppressionTime--;

  updateDelayTime(wasForwarded, name);

  NFD_LOG_INFO("Moving average" << " before: " << m_expMovingAveragePrev
                                << " after: " << m_expMovingAverageCurrent
                                << " duplicate count: " << duplicateCount
                                << " suppression time: "<< m_currentSuppressionTime
                                << " computed max: " << m_computedMaxSuppressionTime
              );
}

void
EMAMeasurements::updateDelayTime(bool wasForwarded, std::string name)
{
   /* this is where we should call reinforcement module and get the suppression time
    information to pass
    1. m_expMovingAverageCurrent
    2. m_expMovingAveragePrev
    3. prefix (name) to compute suppression time for
    4. DUPLICATE_THRESHOLD
    5. wasForwarded? not sure

  */
  // construct message for RL
  // std::string message_dup = "|"+message + "|"+std::to_string(duplicate)+"|";

  boost::property_tree::ptree message;

  message.put("a", m_expMovingAverageCurrent);
  message.put("b", m_expMovingAveragePrev);
  message.put("c", name);
  message.put("d", DUPLICATE_THRESHOLD);
  message.put("e", wasForwarded);

  std::ostringstream oss;
  boost::property_tree::write_json(oss, message, false);
  std::string messageString = oss.str();

  // std::string messageString = "|"+m_expMovingAverageCurrent+ "|"+std::to_string(duplicate)+"|";

  m_fifo.fifo_handler(messageString);

  // auto suppression_time = m_fifo.fifo_read();

  //   double temp;
  //   // Implicit action: if you haven’t reached the goal, but your moving average is decreasing then do nothing.
  //   if (m_expMovingAverageCurrent > DUPLICATE_THRESHOLD &&
  //     m_expMovingAverageCurrent >= m_expMovingAveragePrev ) {
  //     // only increase the suppression timer if this node as forwarded
  //     if (wasForwarded)
  //       temp = m_currentSuppressionTime * MULTIPLICATIVE_INCREASE;
  //     else
  //       // temp = m_currentSuppressionTime * MULTIPLICATIVE_INCREASE;
  //       temp = m_currentSuppressionTime + m_maxDuplicateCount;
  //       // temp = m_currentSuppressionTime + 0.5*m_maxDuplicateCount;
  //       // temp = m_currentSuppressionTime;
  //   }
  //   else if (m_expMovingAverageCurrent <= DUPLICATE_THRESHOLD &&
  //           m_expMovingAverageCurrent <= m_expMovingAveragePrev) {
  //     temp = m_currentSuppressionTime - ADATIVE_DECREASE;
  //   }
  //   else {
  //     temp = m_currentSuppressionTime;
  //   }

  //   m_currentSuppressionTime =  std::min(std::max(m_minSuppressionTime, temp), maxSuppressionTime);
    
}


void
MulticastSuppression::recordInterest(const Interest interest, bool isForwarded)
{
    auto name = interest.getName();
    NFD_LOG_INFO("Interest to check/record" << name);
    auto it = m_interestHistory.find(name);
    if (it == m_interestHistory.end()) // check if interest is already in the map
    {
      // auto forwardStatus = isForwarded ? true : getForwardedStatus(name, 'i');
      m_interestHistory.emplace(name, ObjectHistory{1, isForwarded});
      NFD_LOG_INFO ("Interest: " << name << " inserted into map");

      //  remove the entry after the lifetime expries
      time::milliseconds entryLifetime = DEFAULT_INSTANT_LIFETIME;
      NFD_LOG_INFO("Erasing the interest from the map in : " << entryLifetime);
      setUpdateExpiration(entryLifetime, name, 'i');
    }
    else {
      NFD_LOG_INFO("Counter for interest " << name << " incremented");
      if (!getForwardedStatus(name, 'i') && isForwarded) {
        ++it->second.counter;
        it->second.isForwarded = true;
      }
      else {
        ++it->second.counter;
      }
    }
}

void
MulticastSuppression::recordData(Data data, bool isForwarded)
{
    auto name = data.getName(); //.getPrefix(-1); //removing nounce
    NFD_LOG_INFO("Data to check/record " << name);
    auto it = m_dataHistory.find(name);
    if (it == m_dataHistory.end())
    {
      NFD_LOG_INFO("Inserting data " << name << " into the map");
      m_dataHistory.emplace(name, ObjectHistory{1, isForwarded});

      time::milliseconds entryLifetime = DEFAULT_INSTANT_LIFETIME;
      NFD_LOG_INFO("Erasing the data from the map in : " << entryLifetime);
      setUpdateExpiration(entryLifetime, name, 'd');
    }
    else
    {
      /* these two flags can give rise to 4 different cases
        1. previously forwaded, now received:  increase counter, not the flag
        2. previously received, now received:  increase counter, not the flag
        3. previously forwaded, now trying to forward: ignore, no need to increase the counter (only for data forwading)
          (if only one node, need to forward anyway as soon as possible)
        4. previously received, now trying to forward: increase counter, update flag
      */
      if (!getForwardedStatus(name, 'd') && isForwarded) {
        NFD_LOG_INFO("Counter for  data " << name << " incremented, and the flag is updated");
        ++it->second.counter;
        it->second.isForwarded = true;
      }
      else if (!isForwarded) {
        NFD_LOG_INFO("Counter for  data " << name << " incremented , no change in flag");
        ++it->second.counter;
      }
      else NFD_LOG_INFO("do nothing"); // do nothing

    }
    // need to check if we have the interest in the map
    // if present, need to remove it from the map
    ndn::Name name_cop = name;
    name_cop.appendNumber(0);
    auto itr_timer = m_objectExpirationTimer.find(name_cop);
    if (itr_timer != m_objectExpirationTimer.end())
    {
      NFD_LOG_INFO("Data overheard, deleting interest " <<name << " from the map");
      itr_timer->second.cancel();
      // schedule deletion now
      if (m_interestHistory.count(name) > 0)
      {
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
    if (vec->count(name) > 0)
    {
      //  record interest into moving average
      updateMeasurement(name,  type);
      vec->erase(name);
      NFD_LOG_INFO("Name: " << name << " type: " << type << " expired, and deleted from the instant history");
    }
    });

    name =  (type == 'i') ? name.appendNumber(0) : name.appendNumber(1);
    auto itr_timer = m_objectExpirationTimer.find(name);
    if (itr_timer != m_objectExpirationTimer.end())
    {
      NFD_LOG_INFO("Updating timer for name: " << name << "type: " << type);
      itr_timer->second.cancel();
      itr_timer->second = eventId;
    }
    else
    {
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
    bool wasForwarded = getForwardedStatus(name, type);

    NDN_LOG_INFO("Name:  " << name << " Duplicate Count: " << duplicateCount << " type: " << type);
    // granularity = name - last component e.g. /a/b --> /a
    name = name.getPrefix(-1);
    auto it = vec->find(name);

    // no records
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
      emaEntry->addUpdateEMA(duplicateCount, wasForwarded, name.toUri());
      // nameTree->insert(name.toUri(), emaEntry->getCurrentSuppressionTime(), emaEntry->getMinimumSuppressionTime());
      nameTree->insert(name.toUri(), emaEntry->getCurrentSuppressionTime());
    }
    // update existing record
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
      it->second->addUpdateEMA(duplicateCount, wasForwarded, name.toUri());
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