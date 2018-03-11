#pragma once

#include <stdint.h>
#include "usb/classdriver/base.hpp"

#include "printk.hpp"

namespace
{
  void DumpHex(const char* title, const uint8_t* buf, int len)
  {
    if (title)
    {
      printk("%s:", title);
    }

    for (int i = 0; i < len; ++i)
    {
      printk(" %02x", buf[i]);
    }
    printk("\n");
  }
}

namespace usb::classdriver
{
  class HIDKeyboardDriver : public ClassDriver
  {
    int ep_interrupt_in = 0;
    uint8_t buf_[8] = {};

  public:
    // use base constructor
    using ClassDriver::ClassDriver;

    Error Initialize() override
    {
      ClassDriver::Initialize();

      printk("HID Keyboard Driver: Initialize\n");
      if (ep_interrupt_in == 0)
      {
        printk("HID Keyboard Driver: endpoints are not set\n");
        return error::kNoSuitableEndpoint;
      }

      printk("HID Keyboard Driver: Issuing receive\n");
      EndpointSet()->Receive(ep_interrupt_in, buf_, sizeof(buf_));

      return error::kSuccess;
    }

    Error SetEndpoints(EndpointConfig* configs, int len) override
    {
      for (int i = 0; i < len; ++i)
      {
        if (configs[i].ep_type == EndpointType::kInterrupt && configs[i].dir_in)
        {
          ep_interrupt_in = configs[i].ep_num;
          return error::kSuccess;
        }
      }
      return error::kNoSuitableEndpoint;
    }

    void OnCompleted(int ep_num) override
    {
      printk("HID Keyboard Driver: OnCompleted\n");
      DumpHex("buf_", buf_, sizeof(buf_));
      EndpointSet()->Receive(ep_interrupt_in, buf_, sizeof(buf_));
    }

    void* operator new(size_t size) { return AllocObject<HIDKeyboardDriver>(); }
    void operator delete(void* ptr) { return FreeObject(ptr); }
  };
}
