#include "xhci.hpp"
#include <stdio.h>

namespace bitnos::xhci
{

    Controller::Controller(uintptr_t mmio_base)
        : mmio_base_(mmio_base),
          cap_(reinterpret_cast<struct CapabilityRegisters*>(mmio_base))
    {
        op_ = reinterpret_cast<struct OperationalRegisters*>(
            mmio_base_ + cap_->ReadCAPLENGTH());
    }

    void Controller::Initialize()
    {
        // Set Run/Stop bit
        auto usbcmd = op_->USBCMD.Read();
        usbcmd |= 1u;
        op_->USBCMD.Write(usbcmd);

    }

}
