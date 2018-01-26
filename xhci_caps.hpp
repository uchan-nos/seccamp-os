#pragma once

namespace bitnos::xhci::caps
{
    union ExtendedCapabilityPointerRegister_Bitmap
    {
        uint32_t data;
        struct
        {
            uint32_t capability_id : 8;
            uint32_t next_pointer : 8;
            uint32_t capability_specific : 16;
        } bits;
    };

    struct USBLegacySupport
    {
        union USBLEGSUP_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t capability_id : 8;
                uint32_t next_pointer : 8;
                uint32_t hc_bios_owned_semaphore : 1;
                uint32_t : 7;
                uint32_t hc_os_owned_semaphore : 1;
                uint32_t : 7;
            } bits;
        };

        union USBLEGCTLSTS_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t usb_smi_enable : 1;
                uint32_t : 3;
                uint32_t smi_on_host_system_error_enable : 1;
                uint32_t : 8;
                uint32_t smi_on_os_ownership_enable : 1;
                uint32_t smi_on_pci_command_enable : 1;
                uint32_t smi_on_bar_enable : 1;
                uint32_t smi_on_event_interrupt : 1;
                uint32_t : 3;
                uint32_t smi_on_host_system_error : 1;
                uint32_t : 8;
                uint32_t smi_on_os_ownership_change : 1;
                uint32_t smi_on_pci_command : 1;
                uint32_t smi_on_bar : 1;
            } bits;
        };

        MemMapRegister<USBLEGSUP_Bitmap> USBLEGSUP;
        MemMapRegister<USBLEGCTLSTS_Bitmap> USBLEGCTLSTS;
    } __attribute__((__packed__));

    struct SupportedProtocol
    {
        union Offset00_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t capability_id : 8;
                uint32_t next_pointer : 8;
                uint32_t revision_minor : 8;
                uint32_t revision_major : 8;
            } bits;
        };

        union Offset08_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t compatible_port_offset : 8;
                uint32_t compatible_port_count : 8;
                uint32_t protocol_defined : 12;
                uint32_t protocol_speed_id_count : 4;
            } bits;
        };

        union Offset0c_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t protocol_slot_type : 5;
                uint32_t : 27;
            } bits;
        };

        MemMapRegister<Offset00_Bitmap> offset00;
        MemMapRegister32 name_string;
        MemMapRegister<Offset08_Bitmap> offset08;
        MemMapRegister<Offset0c_Bitmap> offset0c;
    } __attribute__((__packed__));

    union SupportedProtocol_Bitmap
    {
        uint32_t data[4];
        struct
        {
            uint32_t capability_id : 8;
            uint32_t next_pointer : 8;
            uint32_t revision_minor : 8;
            uint32_t revision_major : 8;

            uint32_t name_string;

            uint32_t compatible_port_offset : 8;
            uint32_t compatible_port_count : 8;
            uint32_t protocol_defined : 12;
            uint32_t protocol_speed_id_count : 4;

            uint32_t protocol_slot_type : 5;
            uint32_t : 27;
        } bits;
    };

    union ProtocolSpeedID_Bitmap
    {
        uint32_t data;
        struct
        {
            uint32_t value : 4;
            uint32_t exponent : 2;
            uint32_t type : 2;
            uint32_t full_duplex : 1;
            uint32_t : 7;
            uint32_t mantissa : 16;
        } bits;
    };

    class CapabilityIterator
    {
    public:
        using ValueType = MemMapRegister<ExtendedCapabilityPointerRegister_Bitmap>;
        CapabilityIterator(ValueType* capability_register)
            : capability_register_(capability_register)
        {}

        CapabilityIterator& operator++()
        {
            auto next_pointer = (*this)->Read().bits.next_pointer;
            if (next_pointer > 0)
            {
                capability_register_ += next_pointer;
            }
            else
            {
                capability_register_ = nullptr;
            }
            return *this;
        }

        ValueType* operator->()
        {
            return capability_register_;
        }

        ValueType& operator*()
        {
            return *capability_register_;
        }

        bool operator!=(const CapabilityIterator& rhs) const
        {
            return capability_register_ != rhs.capability_register_;
        }

    private:
        ValueType* capability_register_;
    };

    class CapabilityList
    {
    public:
        using Iterator = CapabilityIterator;
        using ConstIterator = const CapabilityIterator;

        CapabilityList(uintptr_t mmio_base_addr, uint16_t offset)
            : begin_{reinterpret_cast<CapabilityIterator::ValueType*>(mmio_base_addr)
                + offset},
              end_{nullptr}
        {}

        Iterator begin() { return begin_; }
        Iterator end() { return end_; }
        ConstIterator cbegin() const { return begin_; }
        ConstIterator cend() const { return end_; }

    private:
        const CapabilityIterator begin_;
        const CapabilityIterator end_;
    };
}
