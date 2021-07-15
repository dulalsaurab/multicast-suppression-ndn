/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/logger.hpp>

#include <iostream>
#include <unistd.h>

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespaces should be used to prevent/limit name conflicts
namespace examples {

class Producer
{
public:
  Producer()
  : m_scheduler(m_face.getIoService())
  {
  }

  void
  run(const Name& onName)
  {
    std::cout << "setting interest filter on name" << onName << std::endl;
    m_face.setInterestFilter(onName,
                             bind(&Producer::onInterest, this, _1, _2),
                             nullptr, // RegisterPrefixSuccessCallback is optional
                             bind(&Producer::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
  }

private:
  void
  onInterest(const InterestFilter&, const Interest& interest)
  {
    std::cout << ">> I: " << interest << std::endl;

    static const std::string content("Hello, world!");

    // Create Data packet
    auto data = make_shared<Data>(interest.getName());
    data->setFreshnessPeriod(4_s);
    data->setContent(reinterpret_cast<const uint8_t*>(content.data()), content.size());

    // Sign Data packet with default identity
    m_keyChain.sign(*data);

    // Return Data packet to the requester
    std::cout << "<< Data: " << *data << std::endl;
    
    // send data between 250 to 350 ms
    auto rand_sleep = time::milliseconds(250 + (std::rand() % (100)));
    std::cout << "next data send out schedule in :" << rand_sleep <<  "time" << std::endl;
    m_scheduler.schedule(rand_sleep, [this, data] { 
     std::cout << "sending data "<< " now " << std::endl; 
      m_face.put(*data);
      });
    // m_face.put(*data);
  }

  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix '" << prefix
              << "' with the local forwarder (" << reason << ")" << std::endl;
    m_face.shutdown();
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  ndn::Scheduler m_scheduler;
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  srand( (unsigned)time( NULL));
  ndn::Name onName;
  onName= argv[1];
  std:: cout << "this is the name: " << onName << std::endl;
  try {
    ndn::examples::Producer producer;
    producer.run(onName);
    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}