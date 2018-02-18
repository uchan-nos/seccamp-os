#include <CppUTest/CommandLineTestRunner.h>
#include "usb/xhci/event_ring.hpp"

TEST_GROUP(XHCIEventRing) {
  static const size_t kBufSize = 4;
  usb::xhci::EventRing er;
  usb::xhci::InterrupterRegisterSet interrupter{};
  usb::Error error;

  TEST_SETUP()
  {
    error = er.Initialize(kBufSize, &interrupter);
  }

  TEST_TEARDOWN()
  {
  }
};

TEST(XHCIEventRing, Initialize)
{
  CHECK_EQUAL(usb::error::kSuccess, error);

  auto erst = reinterpret_cast<usb::xhci::EventRingSegmentTableEntry*>(
      interrupter.ERSTBA.Read().Pointer());
  CHECK(erst != nullptr);

  auto segm = reinterpret_cast<usb::xhci::TRB*>(erst[0].Pointer());
  CHECK(segm != nullptr);

  CHECK_EQUAL(1, interrupter.ERSTSZ.Read().Size());

  CHECK_EQUAL(reinterpret_cast<uint64_t>(&segm[0]),
              interrupter.ERDP.Read().Pointer());

  CHECK_FALSE(er.HasFront());
}

TEST(XHCIEventRing, Pop)
{
  auto p = reinterpret_cast<usb::xhci::TRB*>(interrupter.ERDP.Read().Pointer());

  {
    usb::xhci::CommandCompletionEventTRB trb{};
    // The producer cycle state of event ring shall be initialized 1.
    trb.bits.cycle_bit = 1;
    for (size_t i = 0; i < 4; ++i)
    {
      p->data[i] = trb.data[i];
    }
  }

  CHECK(er.HasFront());

  {
    auto trb = er.Front();
    CHECK_EQUAL(usb::xhci::CommandCompletionEventTRB::Type, trb->bits.trb_type);
  }

  er.Pop();

  CHECK_FALSE(er.HasFront());
}
