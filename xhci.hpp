#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pci.hpp"
#include "register.hpp"
#include "bitutil.hpp"

namespace bitnos::xhci
{
    struct CapabilityRegisters
    {
        MemMapRegister32 CAPLENGTH_HCIVERSION;
        MemMapRegister32 HCSPARAMS1;
        MemMapRegister32 HCSPARAMS2;
        MemMapRegister32 HCSPARAMS3;
        MemMapRegister32 HCCPARAMS1;
        MemMapRegister32 DBOFF;
        MemMapRegister32 RTSOFF;
        MemMapRegister32 HCCPARAMS2;

        uint8_t ReadCAPLENGTH() const
        {
            return CAPLENGTH_HCIVERSION.Read() & 0xffu;
        }

        uint16_t ReadHCIVERSION() const
        {
            return CAPLENGTH_HCIVERSION.Read() >> 16;
        }
    } __attribute__((__packed__));

    struct OperationalRegisters
    {
        MemMapRegister32 USBCMD;
        MemMapRegister32 USBSTS;
        MemMapRegister32 PAGESIZE;
        uint8_t reserved1_[8];
        MemMapRegister32 DNCTRL;
        MemMapRegister64Access32 CRCR;
        uint8_t reserved2_[16];
        MemMapRegister64Access32 DCBAAP;
        MemMapRegister32 CONFIG;
    } __attribute__((__packed__));

    /*
     * Design: container-like classes.
     *
     * Container-like classes, such as PortArray and DeviceContextArray,
     * should have Size() method and Iterator type.
     * Size() should return the number of elements, and iterators
     * of that type should iterate all elements.
     *
     * Each element may have a flag indicating availableness of the element.
     * For example each port has "Port Enabled/Disabled" bit.
     * Size() and iterators should not skip disabled elements.
     */

    template <typename T>
    class ArrayWrapper
    {
    public:
        using ValueType = T;
        using Iterator = ValueType*;
        using ConstIterator = const ValueType*;

        ArrayWrapper(uintptr_t array_base_addr, size_t size)
            : array_(reinterpret_cast<ValueType*>(array_base_addr)),
              size_(size)
        {}

        size_t Size() const { return size_; }

        Iterator begin() { return array_; }
        Iterator end() { return array_ + size_; }
        ConstIterator cbegin() const { return array_; }
        ConstIterator cend() const { return array_ + size_; }

        ValueType& operator [](size_t index) { return array_[index]; }

    private:
        ValueType* array_;
        size_t size_;
    };

    struct PortRegSet
    {
        MemMapRegister32 PORTSC;
        MemMapRegister32 PORTPMSC;
        MemMapRegister32 PORTLI;
        MemMapRegister32 PORTHLPMC;
    } __attribute__((__packed__));

    using PortRegSetArray = ArrayWrapper<PortRegSet>;

    struct InterrupterRegSet
    {
        MemMapRegister32 IMAN;
        MemMapRegister32 IMOD;
        MemMapRegister32 ERSTSZ;
        uint8_t reserved1_[4];
        MemMapRegister64 ERSTBA;
        MemMapRegister64 ERDP;
    };

    using InterrupterRegSetArray = ArrayWrapper<InterrupterRegSet>;

    struct DoorbellRegister
    {
        MemMapRegister32 DB;
    };

    using DoorbellRegisterArray = ArrayWrapper<DoorbellRegister>;

    union SlotContext
    {
        uint32_t dwords[8];
        struct
        {
            uint32_t route_string : 20;
            uint32_t speed : 4;
            uint32_t : 1; // reserved
            uint32_t mtt : 1;
            uint32_t hub : 1;
            uint32_t context_entries : 5;

            uint32_t max_exit_latency : 16;
            uint32_t root_hub_port_num : 8;
            uint32_t num_ports : 8;

            // TT : Transaction Translator
            uint32_t tt_hub_slot_id : 8;
            uint32_t tt_port_num : 8;
            uint32_t ttt: 2;
            uint32_t : 4; // reserved
            uint32_t interrupter_target : 10;

            uint32_t usb_device_address : 8;
            uint32_t : 19;
            uint32_t slot_state : 5;
        } bits;
    };

    union EndpointContext
    {
        uint32_t dwords[8];
        struct
        {
            uint32_t ep_state : 3;
            uint32_t : 5;
            uint32_t mult : 2;
            uint32_t max_primary_streams : 5;
            uint32_t linear_stream_array : 1;
            uint32_t interval : 8;
            uint32_t max_esit_paylod_hi : 8;

            uint32_t : 1;
            uint32_t error_count : 2;
            uint32_t ep_type : 3;
            uint32_t : 1;
            uint32_t host_initiate_disable : 1;
            uint32_t max_burst_size : 8;
            uint32_t max_packet_size : 16;

            uint32_t dequeue_cycle_state : 1;
            uint32_t : 3;
            uint64_t tr_dequeue_pointer_lo : 60;

            uint32_t average_trb_length : 16;
            uint32_t max_esit_payload_lo : 16;
        } bits;
    };

    struct DeviceContext
    {
        SlotContext slot_context;
        EndpointContext ep_contexts[31];
    };

    struct InputControlContext
    {
        uint32_t drop_context_flags;
        uint32_t add_context_flags;
        uint32_t reserved1[5];
        uint8_t configuration_value;
        uint8_t interface_number;
        uint8_t alternate_setting;
        uint8_t reserved2;
    };

    struct InputContext
    {
        InputControlContext input_control_context;
        SlotContext slot_context;
        EndpointContext ep_contexts[31];
    };

    using DeviceContextAddressArray = ArrayWrapper<DeviceContext*>;


    class Controller
    {
        uintptr_t mmio_base_;
        CapabilityRegisters* cap_;
        OperationalRegisters* op_;

    public:
        Controller(uintptr_t mmio_base);

        void Initialize();

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
            const auto rtsoff = bitutil::ClearBits(cap_->RTSOFF.Read(), 0x1fu);
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
