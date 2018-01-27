#include <CppUTest/CommandLineTestRunner.h>
#include "usb/xhci/ring.hpp"

TEST_GROUP(XHCIRing) {
  static const size_t kBufSize = 4;
  uint8_t buf_raw[sizeof(usb::xhci::TRB) * kBufSize + 63];
  usb::xhci::TRB* buf;
  usb::xhci::Ring* ring;

  TEST_SETUP()
  {
    uintptr_t addr = reinterpret_cast<uintptr_t>(buf_raw);
    // get an address aligned by 64 bytes
    addr = (addr + 63) & ~static_cast<uintptr_t>(63);
    buf = reinterpret_cast<usb::xhci::TRB*>(addr);
    ring = new usb::xhci::Ring{buf, kBufSize};
  }

  TEST_TEARDOWN()
  {
    delete ring;
  }
};

TEST(XHCIRing, Push)
{
  usb::xhci::NormalTRB trb{};
  ring->Push(trb);

  CHECK_EQUAL(1, buf[0].bits.trb_type);
  CHECK_EQUAL(1, buf[0].bits.cycle_bit);

  usb::xhci::NoOpTRB noop{};
  ring->Push(noop);
  ring->Push(noop);

  CHECK_EQUAL(6, buf[3].bits.trb_type); // Link TRB
  CHECK_EQUAL(1, buf[3].bits.cycle_bit);

  auto link = reinterpret_cast<usb::xhci::LinkTRB*>(&buf[3]);
  CHECK_EQUAL(1, link->bits.toggle_cycle);
  CHECK_EQUAL(reinterpret_cast<uint64_t>(&buf[0]), link->Pointer());

  ring->Push(noop);
  CHECK_EQUAL(8, buf[0].bits.trb_type);
  CHECK_EQUAL(0, buf[0].bits.cycle_bit);
}
