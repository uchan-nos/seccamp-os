#include "usb/xhci/port.hpp"

#include "usb/xhci/xhci.hpp"
#include "usb/xhci/registers.hpp"

namespace usb::xhci
{
  uint8_t Port::Number() const
  {
    return port_num_;
  }

  bool Port::IsConnected() const
  {
    return port_reg_set_.PORTSC.Read().bits.current_connect_status;
  }

  bool Port::IsEnabled() const
  {
    return port_reg_set_.PORTSC.Read().bits.port_enabled_disabled;
  }

  bool Port::IsConnectStatusChanged() const
  {
    return port_reg_set_.PORTSC.Read().bits.connect_status_change;
  }

  int Port::Speed() const
  {
    return port_reg_set_.PORTSC.Read().bits.port_speed;
  }

  Device* Port::Initialize()
  {
    return nullptr;
  }
}
