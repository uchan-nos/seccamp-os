#include "usb/xhci/interrupt.hpp"
#include "eventring_util.hpp"

#include <CppUTest/CommandLineTestRunner.h>
//#include <CppUTestExt/GMock.h>

namespace
{
  struct FakeXHCIController : public usb::xhci::Controller
  {
    usb::xhci::EventRing* er_;

    FakeXHCIController(usb::xhci::EventRing* er)
      : er_{er}
    {}

    usb::Error Initialize() { return usb::error::kSuccess; }
    usb::xhci::Ring* CommandRing() { return nullptr; }
    usb::xhci::EventRing* PrimaryEventRing() { return er_; }
    void RingDoorbell(uint8_t target, uint16_t stream_id) {};
    usb::xhci::Port PortAt(size_t index) { return usb::xhci::Port{nullptr}; }
    uint8_t MaxPorts() const { return 255; }
  };

  bool called = false;
  const usb::xhci::CallbackContext* called_ctx;
  usb::InterruptMessage called_msg;

  usb::Error EnqueueMessage(
      const usb::xhci::CallbackContext* ctx,
      const usb::InterruptMessage* msg)
  {
    called = true;
    called_ctx = ctx;
    called_msg = *msg;
    return usb::error::kSuccess;
  }
}

TEST_GROUP(XHCIInterrupt) {
  usb::xhci::InterrupterRegisterSet intr;
  EventRingProducer er_producer;

  usb::xhci::EventRing er;
  FakeXHCIController xhc{&er};

  TEST_SETUP()
  {
    er.Initialize(8, &intr);
    er_producer.Initialize(&intr);
  }

  TEST_TEARDOWN()
  {
  }
};

TEST(XHCIInterrupt, enqueue)
{
  usb::xhci::PortStatusChangeEventTRB trb{};
  trb.bits.port_id = 42;
  er_producer.Push(trb);

  CHECK_FALSE(called);

  usb::xhci::CallbackContext ctx{&xhc, nullptr, EnqueueMessage};
  usb::xhci::InterruptHandler(&ctx);

  CHECK(called);
  CHECK_EQUAL(42, called_msg.attr);
}
