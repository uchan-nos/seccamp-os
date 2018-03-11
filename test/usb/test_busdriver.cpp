#include "usb/busdriver.hpp"
#include "usb/setupdata.hpp"
#include "usb/endpoint.hpp"

#include <CppUTest/CommandLineTestRunner.h>

namespace
{
  struct FakeEndpointSet : public usb::EndpointSet
  {
    std::map<std::string, int> called_count_;

    usb::Error ControlOut(int ep_num, uint64_t setup_data,
                          const void* buf, int len) override
    {
      ++called_count_["ControlOut"];
      return usb::error::kSuccess;
    }

    usb::Error ControlIn(int ep_num, uint64_t setup_data,
                         void* buf, int len) override
    {
      ++called_count_["ControlIn"];
      return usb::error::kSuccess;
    }

    usb::Error Send(int ep_num, const void* buf, int len) override
    {
      ++called_count_["Send"];
      return usb::error::kSuccess;
    }

    usb::Error Receive(int ep_num, void* buf, int len) override
    {
      ++called_count_["Received"];
      return usb::error::kSuccess;
    }
  };
}

TEST_GROUP(USBBusDriver)
{
  FakeEndpointSet epset;

  TEST_SETUP()
  {
  }
};

TEST(USBBusDriver, ComposeSetupData)
{
  usb::SetupData setup_data{};
  setup_data.bits.direction = 1;
  setup_data.bits.type = 0;
  setup_data.bits.recipient = 2;
  setup_data.bits.request = 0xcd;
  setup_data.bits.value = 0x89ab;
  setup_data.bits.index = 0x4567;
  setup_data.bits.length = 0x0123;

  CHECK_EQUAL(0x0123456789abcd82ul, setup_data.data);
}
