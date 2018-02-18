#pragma once

#include "usb/error.hpp"

namespace usb::xhci
{
  class Port
  {
    PortRegisterSet* port_reg_set_;

  public:
    Port(PortRegisterSet* port_reg_set)
      : port_reg_set_{port_reg_set}
    {}

    bool IsConnected() const
    {
      return port_reg_set_->PORTSC.Read().bits.current_connect_status;
    }

    bool IsEnabled() const
    {
      return port_reg_set_->PORTSC.Read().bits.port_enabled_disabled;
    }

    bool IsConnectStatusChanged() const
    {
      return port_reg_set_->PORTSC.Read().bits.connect_status_change;
    }
  };
}
