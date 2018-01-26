#include "xhci.hpp"
#include <stdio.h>

#include "memory_op.hpp"

#include "printk.hpp"

namespace bitnos::xhci
{

    Controller::Controller(uintptr_t mmio_base)
        : mmio_base_(mmio_base),
          cap_(reinterpret_cast<struct CapabilityRegisters*>(mmio_base)),
          device_context_base_addresses_{},
          device_contexts_{}
    {
        op_ = reinterpret_cast<struct OperationalRegisters*>(
            mmio_base_ + cap_->ReadCAPLENGTH());
        cr_mgr_ = new(cr_mgr_buf_) commandring::Manager{*op_, DoorbellRegisters()};
        er_mgr_ = new(er_mgr_buf_) eventring::Manager{InterrupterRegSets()[0]};
    }

    void Controller::Initialize()
    {
        // Host controller must be halted.
        if ((op_->USBSTS.Read() & 1u) == 0)
        {
            printk("Host controller must be halted to reset.\n");
            return;
        }

        // Reset controller
        printk("Resetting host controller... ");
        op_->USBCMD.Write(op_->USBCMD.Read() | (1u << 1));
        //while (op_->USBCMD.Read() & (1u << 1));
        while (op_->USBSTS.Read() & (1u << 11)); // Controller Not Ready
        printk("finished.\n");

        // Set "Max Slots Enabled" field in CONFIG.
        auto config = op_->CONFIG.Read().data;
        config &= 0xffffff00u;
        config |= kDeviceContextCount;
        //config |= 1u << 9; // CIE
        op_->CONFIG.Write(config);

        op_->DCBAAP.Write(reinterpret_cast<uint64_t>(device_context_base_addresses_));

        printk("CRR, CA, CS, RCS = %u\n", op_->CRCR.Read() & 0xfu);
        printk("Initializing command ring manager... ");
        cr_mgr_->Initialize();
        printk("finished.\n");

        printk("Initializing event ring manager... ");
        er_mgr_->Initialize();
        printk("finished.\n");

        // Set Run/Stop bit
        auto usbcmd = op_->USBCMD.Read().data;
        usbcmd |= 1u;
        op_->USBCMD.Write(usbcmd);

        printk("Host controller has been started.\n");
        printk("CRR, CA, CS, RCS = %u\n", op_->CRCR.Read() & 0xfu);
    }
}
