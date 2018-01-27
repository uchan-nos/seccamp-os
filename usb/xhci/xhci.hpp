#pragma once

namespace usb::xhci
{
  class Controller
  {
    const uintptr_t mmio_base_;

  public:
    Controller(uintptr_t mmio_base);

    Error Initialize();
  };
}
