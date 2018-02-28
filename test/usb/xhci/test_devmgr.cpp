#include "usb/xhci/devmgr.hpp"

#include <CppUTest/CommandLineTestRunner.h>

inline SimpleString StringFrom(enum usb::xhci::Device::State state)
{
  return StringFrom(static_cast<int>(state));
}

TEST_GROUP(XHCIDeviceManager)
{
  usb::xhci::DeviceManager devmgr;

  TEST_SETUP()
  {
    devmgr.Initialize(4);
  }
};

TEST(XHCIDeviceManager, Get)
{
  auto res = devmgr.Get(0);
  CHECK_EQUAL(usb::error::kSuccess, res.error);

  usb::xhci::Device* dev = res.value;
  CHECK_EQUAL(usb::xhci::Device::State::kBlank, dev->State());

  res = devmgr.Get(3);
  CHECK_EQUAL(usb::error::kSuccess, res.error);

  res = devmgr.Get(4);
  CHECK_EQUAL(usb::error::kInvalidDeviceId, res.error);
}

TEST(XHCIDeviceManager, DeviceContexts)
{
  auto devctxs = devmgr.DeviceContexts();
  auto dev = devmgr.Get(0).value;

  CHECK_FALSE(devctxs[1]);
  CHECK_FALSE(usb::IsError(devmgr.AssignSlot(dev, 1)));
  CHECK_TRUE(devctxs[1]);

  // Device Context Base Address must be masked bits 5:0.
  auto ptr = reinterpret_cast<uintptr_t>(devctxs[1]);
  ptr &= 0xffffffffffffffc0lu;

  CHECK_EQUAL(ptr, reinterpret_cast<uintptr_t>(dev->DeviceContext()));
}

TEST(XHCIDeviceManager, Remove)
{
  auto devctxs = devmgr.DeviceContexts();
  auto dev = devmgr.Get(1).value;

  devmgr.AssignSlot(dev, 2);
  CHECK_EQUAL(usb::xhci::Device::State::kSlotAssigned, dev->State());
  CHECK_TRUE(devctxs[2]);

  devmgr.Remove(dev);
  CHECK_EQUAL(usb::xhci::Device::State::kBlank, dev->State());
  CHECK_FALSE(devctxs[2]);
}

TEST(XHCIDeviceManager, FindByPort)
{
  CHECK_FALSE(devmgr.FindByPort(2, 0));

  auto dev = devmgr.Get(1).value;
  dev->DeviceContext()->slot_context.bits.root_hub_port_num = 2;
  dev->DeviceContext()->slot_context.bits.route_string = 0;

  CHECK_TRUE(devmgr.FindByPort(2, 0));
}

TEST(XHCIDeviceManager, FindByState)
{
  CHECK_TRUE(devmgr.FindByState(usb::xhci::Device::State::kBlank));
  CHECK_FALSE(devmgr.FindByState(usb::xhci::Device::State::kSlotAssigning));

  devmgr.Get(0).value->SelectForSlotAssignment();

  CHECK_TRUE(devmgr.FindByState(usb::xhci::Device::State::kSlotAssigning));
}

TEST(XHCIDeviceManager, FindBySlot)
{
  CHECK_FALSE(devmgr.FindBySlot(2));

  auto dev = devmgr.Get(0).value;
  dev->SelectForSlotAssignment();
  dev->AssignSlot(2);

  CHECK_TRUE(devmgr.FindBySlot(2));
}
