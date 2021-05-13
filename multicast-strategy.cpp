/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "multicast-strategy.hpp"
#include "algorithm.hpp"
#include "common/logger.hpp"
#include <vector>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include "common/global.hpp"

namespace nfd {
namespace fw {

NFD_REGISTER_STRATEGY(MulticastStrategy);

NFD_LOG_INIT(MulticastStrategy);

const time::milliseconds MulticastStrategy::RETX_SUPPRESSION_INITIAL(10);
const time::milliseconds MulticastStrategy::RETX_SUPPRESSION_MAX(250);

/* quick and dirty _FIFO*/
_FIFO::_FIFO(const char *fifo_suppression_value, const char *fifo_object_details)
  : m_fifo_suppression_value(fifo_suppression_value)
  , m_fifo_object_details(fifo_object_details)
  {
    // mknod(m_fifo_object_details, S_IFIFO | 0666, 0);
    mkfifo(m_fifo_suppression_value, 0666);
    mkfifo(m_fifo_object_details, 0666);
  }

  void 
  _FIFO::fifo_write(std::string  message, int duplicate)
  {
    auto fifo = open(m_fifo_object_details, O_RDWR);
    if (!fifo)
    {
      std::cout << "Couldn't open fifo file" << std::endl;
      return;
    }
    try {
        std::string message_dup = "|"+message + "|"+std::to_string(duplicate)+"|";
        std::cout << "This is the message to write: " << message_dup << std::endl;
        write(fifo, message_dup.c_str(), sizeof(message_dup));
        // write(fifo, dup.c_str(),  sizeof(dup));
      }
      catch (const std::exception &e) {
        std::cout << "Unfortunately came here" << std::endl;
        std::cerr << e.what() << '\n';
      }  
      close(fifo);
  }
 
  time::milliseconds
  _FIFO::fifo_read()
  {
    char buf[10];
    auto fifo = open(m_fifo_suppression_value, O_RDONLY);
    read(fifo, &buf, sizeof(char) * 10);
    std::cout << "Trying to get message from python" << buf << std::endl;
    int suppression_value = 0;
    std::string s (buf);
    std::string delimiter = "|";
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) 
    {
        token = s.substr(0, pos);
        s.erase(0, pos + delimiter.length());
    }
    try {
      suppression_value = std::stoi(token);
    } catch (std::exception const &e) {
      std::cout << "Couldnt convert char to string" << std::endl; 
      // return time::milliseconds(suppression_value);
    }
    return  time::milliseconds(suppression_value);
  }

MulticastStrategy::MulticastStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                      RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                      RETX_SUPPRESSION_MAX)
  , m_fifo(FIFO_VALUE, FIFO_OBJECT)
  , m_forwarder (forwarder)
  // , m_scheduler(m_face.getIoService())
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("MulticastStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "MulticastStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
MulticastStrategy::getStrategyName()
{
  static const auto strategyName = Name("/localhost/nfd/strategy/multicast").appendVersion(4);
  return strategyName;
}

void
MulticastStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  time::milliseconds suppression_interval = 0_ms;
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  for (const auto& nexthop : nexthops) {
    Face& outFace = nexthop.getFace();

    RetxSuppressionResult suppressResult = m_retxSuppression.decidePerUpstream(*pitEntry, outFace);

    if (suppressResult == RetxSuppressionResult::SUPPRESS) {
      NFD_LOG_DEBUG(interest << " from=" << ingress << " to=" << outFace.getId() << " suppressed");
      continue;
    }

    if (!isNextHopEligible(ingress.face, interest, nexthop, pitEntry)) {
      continue;
    }

  // std::string obj_str = interest.getName().toUri();
  // if ((obj_str.find("localhost") == std::string::npos) 
  //     && (obj_str.find("localhop") == std::string::npos)
  //     && (obj_str.find("ndn") == std::string::npos)
  //     ) 
  // {
  //   //  suppression timer code  
  //   // need to apply suppression logic here before sending an interest
  //   m_forwarder.getObjectHistory();
  //   auto dup = m_forwarder.getDuplicateCounter();
  //   const Name& name = interest.getName().getPrefix(-1);

  //     // auto obj = interest.getPrefix(-1)
  //     std::cout << "Total number of duplicate: " << dup << "and name" << name << std::endl; 
  //     m_fifo.fifo_write(name.toUri(), dup);

  //     suppression_interval = m_fifo.fifo_read();

  //     NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
  //     NFD_LOG_DEBUG("Got the suppression time of: " << suppression_interval); 
  //     std::cout <<"Got the suppression time of: " << suppression_interval << std::endl;
  //     m_forwarder.resetDuplicateCounter();
  // }
    // suppression timer code 

    auto sentOutRecord =  getScheduler().schedule(suppression_interval, [this, pitEntry, &outFace, interest, suppressResult] 
    {
       auto sentOutRecord =  this->sendInterest(pitEntry, outFace, interest);
       if (sentOutRecord && suppressResult == RetxSuppressionResult::FORWARD) {
         m_retxSuppression.incrementIntervalForOutRecord(*sentOutRecord);
         }
     });
  }
}

void
MulticastStrategy::afterNewNextHop(const fib::NextHop& nextHop,
                                   const shared_ptr<pit::Entry>& pitEntry)
{
  // no need to check for suppression, as it is a new next hop

  auto nextHopFaceId = nextHop.getFace().getId();
  auto& interest = pitEntry->getInterest();

  // try to find an incoming face record that doesn't violate scope restrictions
  for (const auto& r : pitEntry->getInRecords()) {
    auto& inFace = r.getFace();
    if (isNextHopEligible(inFace, interest, nextHop, pitEntry)) {

      NFD_LOG_DEBUG(interest << " from=" << inFace.getId() << " pitEntry-to=" << nextHopFaceId);
      this->sendInterest(pitEntry, nextHop.getFace(), interest);

      break; // just one eligible incoming face record is enough
    }
  }

  // if nothing found, the interest will not be forwarded
}

} // namespace fw
} // namespace nfd
