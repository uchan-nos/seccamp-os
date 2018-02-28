#pragma once

#include <stdint.h>
#include "usb/error.hpp"

namespace usb::xhci
{
  class Controller;
  struct PortRegisterSet;
  class Device;

  class Port
  {
    Controller& xhc_;
    const uint8_t port_num_;
    PortRegisterSet& port_reg_set_;

  public:
    Port(Controller& xhc, uint8_t port_num, PortRegisterSet& port_reg_set)
      : xhc_{xhc}, port_num_{port_num}, port_reg_set_{port_reg_set}
    {}

    uint8_t Number() const;
    bool IsConnected() const;
    bool IsEnabled() const;
    bool IsConnectStatusChanged() const;
    int Speed() const;
    Device* Initialize();
  };
}
