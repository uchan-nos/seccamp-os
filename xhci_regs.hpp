#pragma once

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

    union PORTSC_Bitmap
    {
        uint32_t data;
        struct
        {
            uint32_t current_connect_status : 1;
            uint32_t port_enabled_disabled : 1;
            uint32_t : 1;
            uint32_t over_current_active : 1;
            uint32_t port_reset : 1;
            uint32_t port_link_state : 4;
            uint32_t port_power : 1;
            uint32_t port_speed : 4;
            uint32_t port_indicator_control : 2;
            uint32_t port_link_state_write_strobe : 1;
            uint32_t connect_status_change : 1;
            uint32_t port_enabled_disabled_change : 1;
            uint32_t warm_port_reset_change : 1;
            uint32_t over_current_change : 1;
            uint32_t port_reset_change : 1;
            uint32_t port_link_state_change : 1;
            uint32_t port_config_error_change : 1;
            uint32_t cold_attach_status : 1;
            uint32_t wake_on_connect_enable : 1;
            uint32_t wake_on_disconnect_enable : 1;
            uint32_t wake_on_over_current_enable : 1;
            uint32_t : 2;
            uint32_t device_removable : 1;
            uint32_t warm_port_reset : 1;
        } bits;
    };

    struct PortRegSet
    {
        MemMapRegister<PORTSC_Bitmap> PORTSC;
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
    } __attribute__((__packed__));

    using InterrupterRegSetArray = ArrayWrapper<InterrupterRegSet>;

    struct DoorbellRegister
    {
        MemMapRegister32 DB;
    };

    using DoorbellRegisterArray = ArrayWrapper<DoorbellRegister>;

    using DeviceContextAddressArray = ArrayWrapper<DeviceContext*>;
}
