/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
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

namespace nfd {
namespace face {

NFD_LOG_INIT(LinkService);

LinkService::LinkService()
  : m_face(nullptr)
  , m_transport(nullptr)
{
}

LinkService::~LinkService()
{
}

void
LinkService::setFaceAndTransport(Face& face, Transport& transport)
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
  // apply suppression algorithm if sending through multicast face
  // check if the interest is already in flight
  if (m_multicastSuppression.interestInflight(interest)) {
    NFD_LOG_INFO ("Interest " <<  interest.getName() << " is in flight, drop the forwarding");
    return; // need to catch this, what should be the behaviour after dropping the interest?? 
  }
  // wait random amount of time before forwarding
  // check if another interest is overheard during the wait, if heard, cancle the forwarding
  auto suppressionTime = m_multicastSuppression.getDelayTimer(interest.getName(), 'i');
  NFD_LOG_INFO("EMA duplicate count for Interest: " << interest.getName()
                                    << " = " << m_multicastSuppression.getMovingAverage(interest.getName(), 'i'));
  NFD_LOG_INFO ("Interest " <<  interest.getName() << " not in flight, waiting" << suppressionTime << "before forwarding");

  auto eventId = getScheduler().schedule(suppressionTime, [this, interest] {
    // finally when time comes for forwarding, check if interest was received during the wait, if so drop the forwarding  
    if (m_multicastSuppression.interestInflight(interest)) {
      NFD_LOG_INFO ("Interest " <<  interest.getName() << "heard while waiting, forwarding is dropped");
      return;  // need to catch this, behaviour after interest drop?? 
    }
    NFD_LOG_INFO ("Interest " <<  interest.getName() << " sent, finally");
    ++this->nOutInterests;
    doSendInterest(interest);
    // record your own interest as well
    m_multicastSuppression.recordInterest(interest);
  });
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
  /*
    Same suppression logic as that of interest cannot be applied to multicast data.
    Doing so we might end up suppressing data for the interest received at different interval
    from different node, additionally it will also trigger multiple retransmission from the node
    that didn't received the data due to suppression. This might not happen if unsolicated data are
    cached, but the node that's supposed to send the data can't gurantee it.
  */
  //  for now, lets wait for some time before forwarding, if overheard, drop the reply
  auto suppressionTime = m_multicastSuppression.getDelayTimer(data.getName(), 'd');
  auto eventId = getScheduler().schedule(suppressionTime, [this, data] {
    NFD_LOG_INFO("Sending data finally, via multicast face " << data.getName());
    ++this->nOutData;
    doSendData(data);
    m_multicastSuppression.recordData(data);
  });
}

void
LinkService::sendNack(const ndn::lp::Nack& nack)
{
  BOOST_ASSERT(m_transport != nullptr);
  NFD_LOG_FACE_TRACE(__func__);

  ++this->nOutNacks;

  doSendNack(nack);
}

void
LinkService::receiveInterest(const Interest& interest, const EndpointId& endpoint)
{
  NFD_LOG_FACE_TRACE(__func__);
  // record multicast interest
  if (this->getFace()->getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS)
  {
    NFD_LOG_INFO("Multicast interest received: " << interest.getName());
    m_multicastSuppression.recordInterest(interest);
  }
  ++this->nInInterests;
  afterReceiveInterest(interest, endpoint);
}

void
LinkService::receiveData(const Data& data, const EndpointId& endpoint)
{
  NFD_LOG_FACE_TRACE(__func__);
  // record multicast Data received
  if (this->getFace()->getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS)
  {
    NFD_LOG_INFO("Multicast data received: " << data.getName());
     m_multicastSuppression.recordData(data);
  }
  ++this->nInData;
  // record multicast data
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

} // namespace face
} // namespace nfd
