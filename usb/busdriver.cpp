#include "usb/busdriver.hpp"

#include <stdint.h>
#include <string.h>

#include "usb/endpoint.hpp"
#include "usb/setupdata.hpp"
#include "usb/classdriver/hid_keyboard.hpp"

#include "printk.hpp"

usb::classdriver::HIDKeyboardDriver* usb_keyboard_driver;

namespace
{
  usb::classdriver::ClassDriver* AllocDriver(
      usb::EndpointSet* epset,
      uint8_t class_code, uint8_t subclass_code, uint8_t protocol_code)
  {
    if (class_code == 3 && subclass_code == 1 && protocol_code == 1)
    {
      usb_keyboard_driver = new usb::classdriver::HIDKeyboardDriver(epset);
      return usb_keyboard_driver;
    }
    return nullptr;
  }
}

namespace usb
{
  Error GetDescriptor(EndpointSet* epset, int ep_num, uint8_t desc_type,
                      uint8_t desc_index, void* buf, int len)
  {
    SetupData setup_data{};
    setup_data.bits.direction = request_type::kIn;
    setup_data.bits.type = request_type::kStandard;
    setup_data.bits.recipient = request_type::kDevice;
    setup_data.bits.request = request::kGetDescriptor;
    setup_data.bits.value = (static_cast<uint16_t>(desc_type) << 8) | desc_index;
    setup_data.bits.index = 0;
    setup_data.bits.length = len;
    return epset->ControlIn(ep_num, setup_data.data, buf, len);
  }

  void Device::OnCompleted(int ep_num)
  {
    if (dispatch_table[ep_num] != nullptr &&
        dispatch_table[ep_num]->IsInitialized())
    {
      printk("calling class driver %d OnCompleted: %08lx\n",
          ep_num, dispatch_table[ep_num]);
      dispatch_table[ep_num]->OnCompleted(ep_num);
      return;
    }

    if (initialize_phase_ == 1)
    {
      // A device descriptor has been received.
      const auto desc_len = desc_buf_[0];
      const auto desc_type = desc_buf_[1];

      if (desc_type != descriptor_type::kDevice || desc_len != 18)
      {
        on_completed_last_error_ = error::kInvalidDescriptor;
        printk("Initialize phase 1: failed %u\n", on_completed_last_error_);
        return;
      }

      const auto dev_class = desc_buf_[4];
      const auto dev_subclass = desc_buf_[5];
      num_configurations_ = desc_buf_[17];

      printk("Initialize phase 1: "
          "Device descriptor: CLASS=%d, SUBCLASS=%d, #CONFIGS=%d\n",
          dev_class, dev_subclass, num_configurations_);

      ++initialize_phase_;
      current_configuration_desc_index_ = 0;
      if (auto err = GetDescriptor(
            descriptor_type::kConfiguration,
            current_configuration_desc_index_); IsError(err))
      {
        on_completed_last_error_ = err;
        return;
      }
    }
    else if (initialize_phase_ == 2)
    {
      const int desc_len = desc_buf_[0];
      const int desc_type = desc_buf_[1];
      const int total_len = (static_cast<uint16_t>(desc_buf_[3]) << 8) | desc_buf_[2];
      const int num_interfaces = desc_buf_[4];

      printk("Initialize phase 2\n");

      if (desc_type != descriptor_type::kConfiguration || desc_len != 9)
      {
        on_completed_last_error_ = error::kInvalidDescriptor;
        return;
      }

      if (total_len > sizeof(desc_buf_))
      {
        on_completed_last_error_ = error::kTooLongDescriptor;
      }

      uint8_t* desc_ptr = &desc_buf_[desc_len];

      for (int interface = 0; interface < num_interfaces; ++interface)
      {
        uint8_t* interface_desc = desc_ptr;
        while (interface_desc[1] != usb::descriptor_type::kInterface)
        {
          interface_desc += interface_desc[0];
          if (interface_desc > desc_buf_ + sizeof(desc_buf_))
          {
            on_completed_last_error_ = error::kInvalidDescriptor;
            return;
          }
        }

        if (interface_desc[1] != usb::descriptor_type::kInterface)
        {
          on_completed_last_error_ = error::kInvalidDescriptor;
          return;
        }

        const int interface_class = interface_desc[5];
        const int interface_subclass = interface_desc[6];
        const int interface_protocol = interface_desc[7];
        const int num_endpoints = interface_desc[4];

        printk("Interface: CLASS=%d, SUBCLASS=%d, PROTOCOL=%d, #ENDPOINT=%d\n",
            interface_class, interface_subclass, interface_protocol,
            num_endpoints);

        auto class_driver = AllocDriver(
            epset_,
            interface_class, interface_subclass, interface_protocol);
        if (class_driver == nullptr)
        {
          printk("Unknown class. Finishing initialization process.\n");
          on_completed_last_error_ = error::kUnkwonClass;
          continue;
        }

        class_drivers[num_class_drivers] = class_driver;
        ++num_class_drivers;

        // Find endpoint descriptors
        uint8_t* endpoint_desc = interface_desc + interface_desc[0];
        while (endpoint_desc[1] != usb::descriptor_type::kEndpoint)
        {
          endpoint_desc += endpoint_desc[0];
          if (endpoint_desc > desc_buf_ + sizeof(desc_buf_))
          {
            on_completed_last_error_ = error::kInvalidDescriptor;
            return;
          }
        }

        EndpointConfig ep_configs[num_endpoints];

        for (int endpoint = 0; endpoint < num_endpoints; ++endpoint)
        {
          const auto ep_addr = endpoint_desc[2];
          const auto ep_attr = endpoint_desc[3];
          const int max_packet_size =
            (static_cast<uint16_t>(endpoint_desc[5]) << 8) | endpoint_desc[4];
          const auto interval = endpoint_desc[6];

          printk("Endpoint: ADDR=%u, ATTR=0x%02x, MaxPS=%d, INTERVAL=%d\n",
              ep_addr, ep_attr, max_packet_size, interval);

          const int ep_num = ep_addr & 0x0fu;
          const bool dir_in = (ep_addr & 0x80u);

          printk("Setting dispatch table %d with %08lx\n",
              ep_num, class_driver);
          dispatch_table[ep_num] = class_driver;

          ep_configs[endpoint].ep_num = ep_num;
          ep_configs[endpoint].dir_in = dir_in;
          ep_configs[endpoint].ep_type =
            static_cast<EndpointType>(ep_attr & 3);
          ep_configs[endpoint].max_packet_size = max_packet_size;
          ep_configs[endpoint].interval = interval;

          endpoint_desc += endpoint_desc[0];
        }

        desc_ptr = endpoint_desc;

        class_driver->SetEndpoints(ep_configs, num_endpoints);
        configure_endpoints_callback(
            ep_configs, num_endpoints, configure_endpoints_callback_arg);
      }

      printk("Incrementing current_configuration_desc_index %u\n",
          current_configuration_desc_index_);
      ++current_configuration_desc_index_;
      if (current_configuration_desc_index_ == num_configurations_)
      {
        ++initialize_phase_;
      }
      else if (auto err = GetDescriptor(
            descriptor_type::kConfiguration,
            current_configuration_desc_index_); IsError(err))
      {
        on_completed_last_error_ = err;
        return;
      }
    }
  }

  void Device::OnEndpointConfigured(unsigned int ep_nums)
  {
    printk("OnEndpointConfigured: Initialize phase %d\n", initialize_phase_);

    for (int i = 0; i < 16; ++i)
    {
      if (((ep_nums >> i) & 1) == 0)
      {
        continue;
      }

      if (dispatch_table[i]->IsInitialized())
      {
        continue;
      }
      dispatch_table[i]->Initialize();
    }

    /*
    for (int i = 0; i < num_class_drivers; ++i)
    {
      class_drivers[i]->Initialize();
    }
    */
  }

  Error Device::ConfigureEndpoints()
  {
    initialize_phase_ = 1;

    memset(desc_buf_, 0, sizeof(desc_buf_));
    if (auto err = GetDescriptor(descriptor_type::kDevice, 0); IsError(err))
    {
      return err;
    }

    return error::kSuccess;
  }
}
