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
#include <ndn-cxx/util/logger.hpp>
#include <iostream>
#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <chrono>
#include <cstdint>


uint64_t 
timeSinceEpochMillisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespaces should be used to prevent/limit name conflicts
namespace examples {

class Consumer
{
public:
  void
  run(std::string name, int i)
  {
    Name interestName(name);
    interestName.appendNumber(i);
    interestName.appendVersion();

    Interest interest(interestName);
    interest.setCanBePrefix(false);
    interest.setMustBeFresh(true);
    interest.setInterestLifetime(1_s); // The default is 4 seconds

    std::cout << timeSinceEpochMillisec() << "Sending Interest " << interest << std::endl;
   
    m_face.expressInterest(interest,
                           bind(&Consumer::onData, this,  _1, _2),
                           bind(&Consumer::onNack, this, _1, _2),
                           bind(&Consumer::onTimeout, this, _1));
    m_face.processEvents();
  }

private:
  void
  onData(const Interest&, const Data& data) const
  {
    std::cout << timeSinceEpochMillisec()<< "Received Data " << data << std::endl;
  }

  void
  onNack(const Interest&, const lp::Nack& nack) const
  {
    std::cout << "Received Nack with reason " << nack.getReason() << std::endl;
  }

  void
  onTimeout(const Interest& interest) const
  {
    std::cout << "Timeout for " << interest << std::endl;
  }

private:
  Face m_face;
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  std::string prefix;
  int interestCount, atInterval;
  prefix = argv[1];
  interestCount = std::stoi(argv[2]);
  atInterval = std::stoi(argv[3]);

  std::cout << prefix << interestCount << atInterval << std::endl;
  ndn::examples::Consumer consumer;
  for (int i=0; i <= interestCount; i++ )
  {
    consumer.run(prefix, i);
    usleep(atInterval);
  }
  // // std::string temp_p = "/uofm/abc";
  // // consumer.run(temp_p, 1);  
  // for (auto prefix: prefixes)
  // {
  //   // usleep (1000); //request for data in every 100ms
  //   try {
  //     // ndn::examples::Consumer consumer;
  //     // for each prefix request 1000 times 
  //     for (int i=0; i < 1000; i++)
  //     {
  //         usleep(100);
  //         auto temp_p = prefix;
  //         // temp_p = prefix.append("/");
  //         // temp_p = temp_p.append(std::to_string(i));
  //         // std::cout << "request for interest: " << temp_p << std::endl;
  //         consumer.run(temp_p, i);
  //     }
  //   }
  //   catch (const std::exception& e) {
  //     std::cerr << "ERROR: " << e.what() << std::endl;
  //     // return 1;
  //   }
  // }

}
