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

    void InputContext::EnableEndpoint(
        size_t ep_index, bool dir_in, const EndpointContext& ctx)
    {
        // DCI: Device Context Index
        const size_t dci = 2 * ep_index + (ep_index == 0 ? 1 : dir_in);

        ep_contexts[dci - 1] = ctx;
        input_control_context.add_context_flags |= 1 << dci;
    }
}
