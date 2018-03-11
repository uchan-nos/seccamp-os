#include "usb/busdriver.hpp"

#include <cstring>

#include <CppUTest/CommandLineTestRunner.h>
#if CPPUTEST_USE_NEW_MACROS
#undef new
#endif

namespace
{
  struct FakeEndpointSet : public usb::EndpointSet
  {
    void* buf_;
    int len_;
    virtual usb::Error ControlOut(int ep_num, uint64_t setup_data,
                                  const void* buf, int len)
    {
      return usb::error::kSuccess;
    }

    virtual usb::Error ControlIn(int ep_num, uint64_t setup_data,
                                 void* buf, int len)
    {
      memcpy(buf, buf_, len_ < len ? len_ : len);
      return usb::error::kSuccess;
    }

    virtual usb::Error Send(int ep_num, const void* buf, int len)
    {
      return usb::error::kSuccess;
    }

    virtual usb::Error Receive(int ep_num, void* buf, int len)
    {
      return usb::error::kSuccess;
    }
  };

  struct
  {
    usb::EndpointConfig* configs;
    int len;
    void* arg;
  } callback_args;

  bool callback_called = false;

  usb::Error ConfigureEndpointsCallback(
      usb::EndpointConfig* configs, int len, void* arg)
  {
    callback_args.configs = configs;
    callback_args.len = len;
    callback_args.arg = arg;
    callback_called = true;
    return usb::error::kSuccess;
  }

  uint8_t device_desc[18] = {
    18, 1, 0, 1, 0 /* class */, 0 /* sub class */, 0 /* protocol */,
    8, 0xff, 0xff, 1, 0, 0, 1, 0, 0, 0, 1 /* #configs */
  };
  uint8_t config_desc[34] = {
    // Configuration Descriptor
    9, 2, 34, 0, 1 /* #interfaces */, 1 /* conf value */, 0, 0, 0,
    // Interface Descriptor
    9, 4, 0 /* interface number */, 0, 1 /* #eps */, 3 /* class */, 1 /* sub class */, 1, 0,
    // HID Descriptor
    9, 33, 1, 1, 0, 1 /* #descs */, 34 /* desc type */, 0x3f /* desc len */, 0,
    // Endpoint Descriptor
    7, 5, 0x81 /* No.1, IN */, 3 /* Interrupt */, 8, 10
  };

}

TEST_GROUP(USBDevice)
{
  FakeEndpointSet epset;
  usb::Device* usb_device;

  TEST_SETUP()
  {
    usb_device = new usb::Device{&epset, ConfigureEndpointsCallback, nullptr};
  }

  TEST_TEARDOWN()
  {
    delete usb_device;
  }
};

TEST(USBDevice, ConfigureEndpoints)
{

  epset.buf_ = device_desc;
  epset.len_ = sizeof(device_desc);

  usb_device->ClearDescriptorBuffer();
  CHECK_EQUAL(usb::error::kSuccess, usb_device->ConfigureEndpoints());
  CHECK_EQUAL(18, usb_device->DescriptorBuffer()[0]);
}

TEST(USBDevice, OnEndpointConfigured)
{
  epset.buf_ = device_desc;
  epset.len_ = sizeof(device_desc);

  usb_device->ClearDescriptorBuffer();
  usb_device->ConfigureEndpoints();

  epset.buf_ = config_desc;
  epset.len_ = sizeof(config_desc);

  usb_device->OnCompleted(0);
  CHECK_EQUAL(9, usb_device->DescriptorBuffer()[0]); // config desc
  CHECK_EQUAL(9, usb_device->DescriptorBuffer()[9]); // interface desc
  CHECK_EQUAL(9, usb_device->DescriptorBuffer()[18]); // HID desc
  CHECK_EQUAL(7, usb_device->DescriptorBuffer()[27]); // endpoint desc
}
