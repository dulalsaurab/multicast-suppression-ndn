/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#include "link-service.hpp"
#include "face.hpp"

namespace nfd::face {

uint8_t MAX_PROPAGATION_DELAY = 30;
const uint8_t OBJECT_TYPE_INTEREST = 0;
const uint8_t OBJECT_TYPE_DATA = 1;

NFD_LOG_INIT(LinkService);

LinkService::~LinkService() = default;

void
LinkService::setFaceAndTransport(Face& face, Transport& transport) noexcept
{
  BOOST_ASSERT(m_face == nullptr);
  BOOST_ASSERT(m_transport == nullptr);

  m_face = &face;
  m_transport = &transport;
}

void
LinkService::sendInterest(const Interest& interest)
{
  BOOST_ASSERT(m_transport != nullptr);
  NFD_LOG_FACE_TRACE(__func__);

  if (this->getFace()->getLinkType() != ndn::nfd::LINK_TYPE_MULTI_ACCESS)
  {
    ++this->nOutInterests;
    doSendInterest(interest);
    return;
  }

  time::milliseconds suppressionTimer (nfd::face::ams::getRandomNumber(MAX_PROPAGATION_DELAY)); // timer is randomized value
  NFD_LOG_INFO ("Scheduling interest: " <<  interest.getName() << " to forward after: " << suppressionTimer);

  auto entry_name = interest.getName();
  entry_name.appendNumber(OBJECT_TYPE_INTEREST);

  auto eventId = getScheduler().schedule(suppressionTimer, [this, interest, entry_name] {
    NFD_LOG_INFO ("Interest " <<  interest.getName() << " sent, finally");
    ++this->nOutInterests;
    doSendInterest(interest);
    if (m_scheduledEntry.count(entry_name) > 0)
      m_scheduledEntry.erase(entry_name);
  });
  m_scheduledEntry.emplace(entry_name, eventId);
}

void
LinkService::sendData(const Data& data)
{
  BOOST_ASSERT(m_transport != nullptr);
  NFD_LOG_FACE_TRACE(__func__);
  if (this->getFace()->getLinkType() != ndn::nfd::LINK_TYPE_MULTI_ACCESS)
  {
    ++this->nOutData;
    doSendData(data);
    return;
  }
  time::milliseconds suppressionTimer (nfd::face::ams::getRandomNumber(MAX_PROPAGATION_DELAY)); // timer is randomized value
  NFD_LOG_INFO ("Scheduling data: " <<  data.getName() << " to forward after: " << suppressionTimer);

  auto entry_name = data.getName();
  entry_name.appendNumber(OBJECT_TYPE_DATA);

  auto eventId = getScheduler().schedule(suppressionTimer, [this, data, entry_name] {
    NFD_LOG_INFO ("Data " <<  data.getName() << " sent, finally");
    ++this->nOutData;
    doSendData(data);
    if (m_scheduledEntry.count(entry_name) > 0)
      m_scheduledEntry.erase(entry_name);
  });
  m_scheduledEntry.emplace(entry_name, eventId);
}

void
LinkService::sendNack(const ndn::lp::Nack& nack)
{
  BOOST_ASSERT(m_transport != nullptr);
  NFD_LOG_FACE_TRACE(__func__);

  ++this->nOutNacks;

  doSendNack(nack);
}

bool
LinkService::cancelIfSchdeuled(Name name, uint8_t objectType)
{
  auto entry_name = name.appendNumber(objectType);
  auto it = m_scheduledEntry.find(entry_name);
  if (it != m_scheduledEntry.end()) {
    it->second.cancel();
    m_scheduledEntry.erase(entry_name);
    return true;
  }
  return false;
}

void
LinkService::receiveInterest(const Interest& interest, const EndpointId& endpoint)
{
  NFD_LOG_FACE_TRACE(__func__);
  // Record multicast interest
  if (this->getFace()->getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS)
  {
    NFD_LOG_INFO("Multicast interest received: " << interest.getName());
    // Check if a same interest is scheduled, if so drop it
    if (cancelIfSchdeuled(interest.getName(), OBJECT_TYPE_INTEREST))
      NDN_LOG_INFO("Interest drop, Interest " << interest.getName() << " overheard, duplicate forwarding dropped");
  }
  ++this->nInInterests;
  afterReceiveInterest(interest, endpoint);
}

void
LinkService::receiveData(const Data& data, const EndpointId& endpoint)
{
  NFD_LOG_FACE_TRACE(__func__);
  // Record multicast Data received
  if (this->getFace()->getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS)
  {
    NFD_LOG_INFO("Multicast data received: " << data.getName());
    if (cancelIfSchdeuled(data.getName(), OBJECT_TYPE_DATA))
      NDN_LOG_INFO("Data drop, Data " << data.getName() << " overheard, duplicate forwarding dropped");

    if (cancelIfSchdeuled(data.getName(), OBJECT_TYPE_INTEREST)) // also can drop interest if shceduled for this data
      NDN_LOG_INFO("Interest drop, Data " << data.getName() << " overheard, drop the corresponding scheduled interest");
  }

  ++this->nInData;
  afterReceiveData(data, endpoint);
}

void
LinkService::receiveNack(const ndn::lp::Nack& nack, const EndpointId& endpoint)
{
  NFD_LOG_FACE_TRACE(__func__);

  ++this->nInNacks;

  afterReceiveNack(nack, endpoint);
}

void
LinkService::notifyDroppedInterest(const Interest& interest)
{
  ++this->nInterestsExceededRetx;
  onDroppedInterest(interest);
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<LinkService>& flh)
{
  const Face* face = flh.obj.getFace();
  if (face == nullptr) {
    os << "[id=0,local=unknown,remote=unknown] ";
  }
  else {
    os << "[id=" << face->getId() << ",local=" << face->getLocalUri()
       << ",remote=" << face->getRemoteUri() << "] ";
  }
  return os;
}

} // namespace nfd::face
