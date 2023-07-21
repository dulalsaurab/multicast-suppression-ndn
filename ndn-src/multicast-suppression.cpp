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

const time::milliseconds DEFAULT_INSTANT_LIFETIME = 30_ms; // 2*MAX_PROPOGATION_DELAY
const double DUPLICATE_THRESHOLD = 1.5; // parameter to tune
const double ADATIVE_DECREASE = 5.0;
const double MULTIPLICATIVE_INCREASE = 1.3;

// in milliseconds ms
// probably we need to provide sufficient time for other party to hear you?? MAX_PROPAGATION_DELAY??
const double minSuppressionTime = 15.0f;
const double maxSuppressionTime= 15000.0f;
static bool seeded = false;

unsigned int UNSET = -1234;
const uint8_t CHARACTER_SIZE = 126;
const uint8_t MAX_IGNORE = 3;

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

void
NameTree::insert(const std::string& prefix, double value)
{
  auto node = this; // this should be root
  for (unsigned int i = 0; i < prefix.length(); i++)
  {
    auto index = prefix[i] - 'a';
    if (!node->children[index])
      node->children[index] = std::make_unique<NameTree>();

    if (i == prefix.length() - 1)
    node->suppressionTime = value;

    node = node->children[index].get();
  }
  // leaf node
  node->isLeaf = true;
}

double
NameTree::longestPrefixMatch(const std::string& prefix) // longest prefix match
{
  auto node = this;
  double lastValueFound = UNSET;
  for (unsigned int i=0; i < prefix.length()-1; i++)
  {
    auto index = prefix[i] - 'a';
    if (prefix[i+1] == '/') // key at i is the available prefix if the next item is /
      lastValueFound = node->suppressionTime;

    if (!node->children[index])
      return lastValueFound;

    if (i == prefix.length() - 1)
      lastValueFound = node->suppressionTime;

    node = node->children[index].get();
  }
  return lastValueFound;
}

time::milliseconds
NameTree::getSuppressionTimer(const std::string& name)
{
  double val, suppressionTime;
  val = longestPrefixMatch(name);
  suppressionTime = (val == UNSET) ? minSuppressionTime : val;
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
EMAMeasurements::EMAMeasurements(double expMovingAverage = 0, uint8_t lastDuplicateCount = 0, double suppressionTime = 1)
: m_expMovingAveragePrev (expMovingAverage)
, m_expMovingAverageCurrent (expMovingAverage)
, m_currentSuppressionTime(suppressionTime)
, m_lastDuplicateCount(lastDuplicateCount)
, m_maxDuplicateCount (1)
, m_minSuppressionTime(minSuppressionTime)
, m_ignoreDuplicateRecoring(0)
{
}

/*
 we compute exponential moving average to give higher preference to the most recent interest/data
 EMA = duplicate count if t = 1
 EMA  = alpha*Dt + (1 - alpha) * EMA t-1
*/
void
EMAMeasurements::addUpdateEMA(uint8_t duplicateCount, bool wasForwarded)
{
  // If duplicate count is greater than last duplicate count, increase the ignore counter
  // else reset it to 0.
  m_ignoreDuplicateRecoring  = (duplicateCount > m_lastDuplicateCount) ? (m_ignoreDuplicateRecoring+1) : 0;

  if (m_ignoreDuplicateRecoring > 0 && m_ignoreDuplicateRecoring < MAX_IGNORE) {
    NDN_LOG_INFO("Duplicate count: " << duplicateCount << " m_lastdup: "
                  << m_lastDuplicateCount << " ignore counter: " << m_ignoreDuplicateRecoring);
    return;
  }

  // Update/Reset duplicate count and ignore counter
  m_lastDuplicateCount = duplicateCount;
  m_ignoreDuplicateRecoring = 0;

  m_expMovingAveragePrev = m_expMovingAverageCurrent;
  if (m_expMovingAverageCurrent == 0) {
    m_expMovingAverageCurrent = duplicateCount;
  }
  else {
    // rounding to 2 decimal place
    m_expMovingAverageCurrent =  round ((DISCOUNT_FACTOR*duplicateCount +
                                       (1 - DISCOUNT_FACTOR)*m_expMovingAverageCurrent)*10.0)/10.0;
  }
  // Update maximum duplicate count
  if (m_maxDuplicateCount < duplicateCount) {
    m_maxDuplicateCount = duplicateCount;
  }
  // Update min suppression time
  if (m_maxDuplicateCount > 1) {
    m_minSuppressionTime = (float) MAX_PROPOGATION_DELAY;
  } else if (m_maxDuplicateCount == 1 && m_minSuppressionTime > 1) {
    m_minSuppressionTime--;
  }

  // Update the suppression time, only if this node has forwarded
  updateDelayTime(wasForwarded);

  // Log the results
  NFD_LOG_INFO("Moving average" << " before: " << m_expMovingAveragePrev
                                << " after: " << m_expMovingAverageCurrent
                                << " duplicate count: " << duplicateCount
                                << " suppression time: "<< m_currentSuppressionTime);
}

void
EMAMeasurements::updateDelayTime(bool wasForwarded)
{
  double temp;
  // Implicit action: if you haven’t reached the goal, but your moving average is decreasing then do nothing.
  if (m_expMovingAverageCurrent > DUPLICATE_THRESHOLD &&
    m_expMovingAverageCurrent >= m_expMovingAveragePrev ) {
    // only increase the suppression timer if this node as forwarded
    temp = (wasForwarded) ? (m_currentSuppressionTime * MULTIPLICATIVE_INCREASE) : m_currentSuppressionTime;

  }
  else if (m_expMovingAverageCurrent <= DUPLICATE_THRESHOLD &&
          m_expMovingAverageCurrent <= m_expMovingAveragePrev) {
    temp = m_currentSuppressionTime - ADATIVE_DECREASE;
  }
  else {
    temp = m_currentSuppressionTime;
  }
  m_currentSuppressionTime =  std::min(std::max(m_minSuppressionTime, temp), maxSuppressionTime);
}


void
MulticastSuppression::recordInterest(const Interest& interest, bool isForwarded)
{
  auto name = interest.getName();
  NFD_LOG_INFO("Interest to check/record: " << name);

  auto it = m_interestHistory.find(name);
  // Check if interest is already in the map
  if (it == m_interestHistory.end()) {
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

    it->second.isForwarded = (!getForwardedStatus(name, 'i') && isForwarded) ? true : it->second.isForwarded;
    ++it->second.counter;
  }
}

void
MulticastSuppression::recordData(const Data& data, bool isForwarded)
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
    /* These two flags can give rise to 4 different cases
      1. previously forwaded, now received:  increase counter, not the flag
      2. previously received, now received:  increase counter, not the flag
      3. previously forwaded, now trying to forward: ignore, no need to increase the counter (only for data forwading)
        (if only one node, need to forward anyway as soon as possible)
      3. previously received, now trying to forward: increase counter, update flag
    */
    if (!getForwardedStatus(name, 'd') && isForwarded) {
      NFD_LOG_INFO("Counter for  data " << name << " incremented, but the flag is not updated");
      ++it->second.counter;
      it->second.isForwarded = true;
    }
    else if (!isForwarded) {
      NFD_LOG_INFO("Counter for  data " << name << " incremented , no change in flag");
      ++it->second.counter;
    }
    else {
      NFD_LOG_INFO("do nothing"); // do nothing
    }

  }
  // Need to check if we have the interest in the map
  // if present, need to remove it from the map
  ndn::Name interestOrDataObjectName = name;
  interestOrDataObjectName.appendNumber(0);
  auto itr_timer = m_objectExpirationTimer.find(interestOrDataObjectName);
  if (itr_timer != m_objectExpirationTimer.end()) {
    NFD_LOG_INFO("Data overheard, deleting interest " <<name << " from the map");
    itr_timer->second.cancel();

    // Schedule deletion now
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
  auto objectHistory = getRecorder(type);
  auto eventId = getScheduler().schedule(entryLifetime, [=]  {
      if (objectHistory->count(name) > 0) {
        //  record interest into moving average
        updateMeasurement(name,  type);
        objectHistory->erase(name);
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

void MulticastSuppression::updateMeasurement(Name name, char type)
{
  auto objectHistory = getEMARecorder(type);
  auto nameTree = getNameTree(type);
  auto duplicateCount = getDuplicateCount(name, type);
  bool wasForwarded = getForwardedStatus(name, type);

  name = name.getPrefix(-1);  // granularity = name - last component e.g. /a/b --> /a
  auto& emaEntry = (*objectHistory)[name];  // try to find existing record

  if (!emaEntry) {
      // no record found, create a new one
      emaEntry = std::make_shared<EMAMeasurements>();
      NFD_LOG_INFO("Creating EMA record for name: " << name << " type: " << type);
      auto expirationId = getScheduler().schedule(MAX_MEASURMENT_INACTIVE_PERIOD, [=] {
          objectHistory->erase(name);  // remove expired record
          nameTree->insert(name.toUri(), UNSET);  // unset the value in the name tree
      });
      emaEntry->setEMAExpiration(expirationId);
      nameTree->insert(name.toUri(), emaEntry->getCurrentSuppressionTime());
  } else {
      // existing record found, update it
      NFD_LOG_INFO("Updating EMA record for name: " << name << " type: " << type);
      emaEntry->getEMAExpiration().cancel();
      auto expirationId = getScheduler().schedule(MAX_MEASURMENT_INACTIVE_PERIOD, [=] {
          objectHistory->erase(name);  // remove expired record
          nameTree->insert(name.toUri(), UNSET);  // unset the value in the name tree
      });
      emaEntry->setEMAExpiration(expirationId);
      nameTree->insert(name.toUri(), emaEntry->getCurrentSuppressionTime());
  }
  emaEntry->addUpdateEMA(duplicateCount, wasForwarded);
}

time::milliseconds
MulticastSuppression::getDelayTimer(Name name, char type)
{
  NFD_LOG_INFO("Getting supperssion timer for name: " << name);
  auto nameTree = getNameTree(type);
  if (!nameTree) {
    NFD_LOG_ERROR("Name tree is null");
    return time::milliseconds(0);
  }

  auto suppressionTimer = nameTree->getSuppressionTimer(name.getPrefix(-1).toUri());
  NFD_LOG_INFO("Suppression timer for name: " << name << " and type: "<< type << " = " << suppressionTimer);
  return suppressionTimer;
}

} //namespace ams
} //namespace face
} //namespace nfd
