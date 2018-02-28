#include "usb/xhci/interrupt.hpp"
#include "eventring_util.hpp"

#include <CppUTest/CommandLineTestRunner.h>
//#include <CppUTestExt/GMock.h>

inline SimpleString StringFrom(enum usb::xhci::Device::State state)
{
  return StringFrom(static_cast<int>(state));
}

namespace
{
  struct FakeXHCIController : public usb::xhci::Controller
  {
    usb::xhci::Ring cr_;
    usb::xhci::EventRing er_;
    usb::xhci::DoorbellRegister db_;
    usb::xhci::PortRegisterSet port_reg_set_;
    usb::xhci::DeviceManager devmgr_;

    usb::Error Initialize() { return usb::error::kSuccess; }
    usb::xhci::Ring* CommandRing() { return &cr_; }
    usb::xhci::EventRing* PrimaryEventRing() { return &er_; }
    usb::xhci::DoorbellRegister* DoorbellRegisterAt(uint8_t index) { return &db_; }
    usb::xhci::Port PortAt(uint8_t port_num)
    {
      return usb::xhci::Port{*this, port_num, port_reg_set_};
    }
    uint8_t MaxPorts() const { return 255; }
    usb::xhci::DeviceManager* DeviceManager()
    {
      return &devmgr_;
    }
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

  FakeXHCIController xhc;

  TEST_SETUP()
  {
    xhc.devmgr_.Initialize(4);
    xhc.cr_.Initialize(8);
    xhc.er_.Initialize(8, &intr);
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
  CHECK_EQUAL(42, called_msg.attr1);
}

TEST(XHCIInterrupt, port_is_connected)
{
  const uint8_t port_id = 2;

  usb::xhci::CallbackContext ctx{&xhc, nullptr, nullptr};
  usb::InterruptMessage msg{
    usb::InterruptMessageType::kXHCIPortStatusChangeEvent,
    port_id, 0, 0, 0,
    nullptr
  };

  // port is connected
  usb::xhci::PORTSC_Bitmap portsc{};
  portsc.bits.current_connect_status = true;
  xhc.port_reg_set_.PORTSC.Write(portsc);

  // nothing in the command ring.
  auto trb = xhc.cr_.Buffer();
  CHECK_FALSE(trb->bits.cycle_bit);

  usb::xhci::OnPortStatusChanged(&ctx, msg);

  // OnPortStatusChanged should issue an EnableSlotCommand to the command ring.
  CHECK_TRUE(trb->bits.cycle_bit);
  CHECK_EQUAL(usb::xhci::EnableSlotCommandTRB::Type, trb->bits.trb_type);
}

TEST(XHCIInterrupt, port_is_disconnected)
{
  const uint8_t port_id = 2;
  const uint8_t slot_id = 1;
  const uint8_t device_id = 0;

  usb::xhci::CallbackContext ctx{&xhc, nullptr, nullptr};
  usb::InterruptMessage msg{
    usb::InterruptMessageType::kXHCIPortStatusChangeEvent,
    port_id, 0, 0, 0,
    nullptr
  };

  // port is not connected
  usb::xhci::PORTSC_Bitmap portsc{};
  portsc.bits.current_connect_status = false;
  xhc.port_reg_set_.PORTSC.Write(portsc);

  // device manager has an entity at index 1
  auto dev = xhc.devmgr_.Get(device_id).value;
  dev->AssignSlot(slot_id);
  dev->DeviceContext()->slot_context.bits.route_string = 0;
  dev->DeviceContext()->slot_context.bits.root_hub_port_num = port_id;
  CHECK_EQUAL(usb::xhci::Device::State::kSlotAssigned, dev->State());

  usb::xhci::OnPortStatusChanged(&ctx, msg);

  // OnPortStatusChanged should free the device.
  CHECK_EQUAL(usb::xhci::Device::State::kBlank, dev->State());
}
