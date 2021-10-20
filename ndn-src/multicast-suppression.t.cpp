#include "face/multicast-suppression.hpp"

#include "fw/multicast-strategy.hpp"

// #include "tests/test-common.hpp

#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/fw/topology-tester.hpp"

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace ams {
namespace tests {

using namespace nfd::fw::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestAMS, GlobalIoTimeFixture)

using nfd::Face;

class AMSFixture : public GlobalIoTimeFixture
{
protected:
  explicit
  AMSFixture()
  {
    nodeC1 = topo.addForwarder("C1");
    nodeC2 = topo.addForwarder("C2");
    nodeP = topo.addForwarder("P");

    for (TopologyNode node : {nodeC1, nodeC2, nodeP}) {
      topo.setStrategy<nfd::fw::MulticastStrategy>(node);
    }

    auto w = topo.addLink("W", 10_ms, {nodeC1, nodeC2, nodeP}, ndn::nfd::LINK_TYPE_MULTI_ACCESS);

    faceC1 = &w->getFace(nodeC1);
    faceC2 = &w->getFace(nodeC2);
    faceP = &w->getFace(nodeP);

    consumer1 = topo.addAppFace("consumer1", nodeC1);
    topo.registerPrefix(nodeC1, *faceC1, PRODUCER_PREFIX);
    consumer2 = topo.addAppFace("consumer2", nodeC2);
    topo.registerPrefix(nodeC2, *faceC2, PRODUCER_PREFIX);
    producer = topo.addAppFace("producer", nodeP, PRODUCER_PREFIX);
    topo.addEchoProducer(producer->getClientFace(), PRODUCER_PREFIX, 2_s);
  }

  // void
  // runConsumer(shared_ptr<TopologyAppLink> &con, size_t numInterests = 1)
  // {
  //   topo.addIntervalConsumer(con->getClientFace(), PRODUCER_PREFIX, 1_s, numInterests);
  //   this->advanceClocks(10_ms, time::seconds(numInterests));
  // }

protected:
  TopologyTester topo;
  TopologyNode nodeC1;
  TopologyNode nodeC2;
  TopologyNode nodeP;

  Face* faceC1;
  Face* faceC2;
  Face* faceP;
  
  shared_ptr<TopologyLink> linkPC1;
  shared_ptr<TopologyLink> linkPC2;
  shared_ptr<TopologyLink> linkC1C2;

  shared_ptr<TopologyAppLink> consumer1;
  shared_ptr<TopologyAppLink> consumer2;
  shared_ptr<TopologyAppLink> producer;

  static const Name PRODUCER_PREFIX;
};

const Name AMSFixture::PRODUCER_PREFIX("/hr/C");

BOOST_FIXTURE_TEST_CASE(Basic, AMSFixture)
{
  Name name(PRODUCER_PREFIX);
  name.append("test");
  auto interest = makeInterest(name);

  consumer1->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(5_ms, 1_s);

  // we expect both C2 and P to receive the interest sent by consumer1.
  BOOST_CHECK_EQUAL(faceC1->getCounters().nOutInterests, 1);
  BOOST_CHECK_EQUAL(faceP->getCounters().nInInterests, 1);
  BOOST_CHECK_EQUAL(faceC2->getCounters().nInInterests, 1);

   // this interest should be suppressed because consumer2 have already overheard the first interest
   consumer2->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
   this->advanceClocks(3_ms, 90_ms);
   BOOST_CHECK_EQUAL(faceC2->getCounters().nOutInterests, 0);

   this->advanceClocks(10_ms, 3_s);
   //  at this stage both consumer c1, and c2 should get the data packet back
   BOOST_CHECK_EQUAL(faceC1->getCounters().nInData, 1);
   BOOST_CHECK_EQUAL(faceC2->getCounters().nInData, 1);

  // interest here is differnt than the previous one, but the prefix (component -1) is same so the EMA for the prefix will be computed
   interest = makeInterest(name.getPrefix(-1).append("test1"));
   consumer1->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
   this->advanceClocks(10_ms, 2_ms);
   BOOST_CHECK_EQUAL(faceC1->getCounters().nOutInterests, 1);

}

BOOST_AUTO_TEST_SUITE_END() // TestAsfStrategy
// BOOST_AUTO_TEST_SUITE_END() // Fw

}
} // namespace tests
} // namespace ams
} // namespace fw
} // namespace nfd
