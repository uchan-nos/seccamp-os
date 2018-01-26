#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pci.hpp"
#include "register.hpp"
#include "bitutil.hpp"
#include "xhci_ctx.hpp"
#include "xhci_regs.hpp"
#include "xhci_cr.hpp"
#include "xhci_er.hpp"
#include "xhci_caps.hpp"

namespace bitnos::xhci
{
    class Controller
    {
        uintptr_t mmio_base_;
        CapabilityRegisters* cap_;
        OperationalRegisters* op_;

        static const size_t kDeviceContextCount = 8;
        alignas(64) uint64_t device_context_base_addresses_[256];
        alignas(64) DeviceContext device_contexts_[kDeviceContextCount];

        alignas(64) uint8_t cr_mgr_buf_[sizeof(commandring::Manager)];
        commandring::Manager* cr_mgr_;
        alignas(64) uint8_t er_mgr_buf_[sizeof(eventring::Manager)];
        eventring::Manager* er_mgr_;

    public:
        Controller(uintptr_t mmio_base);

        void Initialize();

        commandring::Manager& CommandRingManager() { return *cr_mgr_; }
        eventring::Manager& EventRingManager() { return *er_mgr_; }

        auto& CapabilityRegisters() { return *cap_; }
        const auto& CapabilityRegisters() const { return *cap_; }
        auto& OperationalRegisters() { return *op_; }
        const auto& OperationalRegisters() const { return *op_; }

        PortRegSetArray PortRegSets() const
        {
            return {
                reinterpret_cast<uintptr_t>(op_) + 0x400u,
                cap_->HCSPARAMS1.Read() >> 24
            };
        }

        InterrupterRegSetArray InterrupterRegSets() const
        {
            const auto rtsoff = bitutil::ClearBits(cap_->RTSOFF.Read().data, 0x1fu);
            return {
                mmio_base_ + rtsoff + 0x20u,
                cap_->HCSPARAMS1.Read() >> 24
            };
        }

        DoorbellRegisterArray DoorbellRegisters() const
        {
            return {
                mmio_base_ + bitutil::ClearBits(cap_->DBOFF.Read(), 0x3u),
                256
            };
        }

        DeviceContextAddressArray DeviceContextAddresses() const
        {
            return {
                bitutil::ClearBits(op_->DCBAAP.Read(), 0x3fu),
                op_->CONFIG.Read() & 0xffu
            };
        }

        caps::CapabilityList Capabilities() const
        {
            return {
                mmio_base_,
                static_cast<uint16_t>(cap_->HCCPARAMS1.Read() >> 16)
            };
        }

        void LoadDeviceContextAddress(size_t slot_id)
        {
            DeviceContextAddresses()[slot_id] = &device_contexts_[slot_id - 1];
        }

        void ClearDeviceContextAddress(size_t slot_id)
        {
            DeviceContextAddresses()[slot_id] = 0;
        }
    };

    /*
     * getters and setters for a host controller
     */

    inline uint8_t MaxSlots(const Controller& c)
    {
        return c.CapabilityRegisters().HCSPARAMS1.Read() & 0xffu;
    }

    inline uint8_t MaxSlotsEnabled(const Controller& c)
    {
        return c.OperationalRegisters().CONFIG.Read() & 0xffu;
    }

    inline uint8_t MaxPorts(const Controller& c)
    {
        return c.CapabilityRegisters().HCSPARAMS1.Read() >> 24;
    }

}
