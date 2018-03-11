#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "usb/error.hpp"
#include "usb/malloc.hpp"
#include "usb/setupdata.hpp"
#include "usb/endpoint.hpp"
#include "usb/classdriver/base.hpp"

namespace usb
{
  Error GetDescriptor(EndpointSet* epset, int ep_num, uint8_t desc_type,
                      uint8_t desc_index, void* buf, int len);

  using ConfigureEndpointsCallbackType =
    Error (EndpointConfig* configs, int len, void* arg);

  class Device
  {
    EndpointSet* epset_;
    ConfigureEndpointsCallbackType* configure_endpoints_callback;
    void* configure_endpoints_callback_arg;

    uint8_t desc_buf_[128];

    int initialize_phase_ = 0;
    int num_configurations_ = 0;
    int current_configuration_desc_index_ = 0;
    Error on_completed_last_error_ = error::kSuccess;

    // Dispatch table. Index is a endpoint number.
    classdriver::ClassDriver* dispatch_table[16] = {};
    classdriver::ClassDriver* class_drivers[4] = {};
    int num_class_drivers = 0;

    Error GetDescriptor(uint8_t desc_type, uint8_t desc_index)
    {
      return ::usb::GetDescriptor(epset_, 0, desc_type, desc_index, desc_buf_, sizeof(desc_buf_));
    }

  public:
    Device(EndpointSet* epset, ConfigureEndpointsCallbackType* callback, void* arg)
      : epset_{epset},
        configure_endpoints_callback{callback},
        configure_endpoints_callback_arg{arg}
    {}

    void OnCompleted(int ep_num);
    void OnEndpointConfigured(unsigned int ep_nums);
    Error ConfigureEndpoints();

    EndpointSet* EndpointSet() { return epset_; }

    // for test
    void ClearDescriptorBuffer() { memset(desc_buf_, 0, sizeof(desc_buf_)); }
    uint8_t* DescriptorBuffer() { return desc_buf_; }

    void* operator new(size_t size) { return AllocObject<Device>(); }
    void operator delete(void* ptr) { return FreeObject(ptr); }
  };
}
