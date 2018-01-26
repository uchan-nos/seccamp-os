#ifndef PCI_HPP_
#define PCI_HPP_

#include <stdint.h>
#include "errorcode.hpp"
#include "register.hpp"

namespace bitnos::pci
{
    const uint16_t kConfigAddress = 0x0cf8;
    const uint16_t kConfigData = 0x0cfc;

    struct ScanCallbackParam
    {
        uint8_t bus, dev, func;
        uint16_t device_id, vendor_id;
        uint8_t base_class, sub_class, interface, header_type;
    };

    using ScanCallbackType = void (const ScanCallbackParam& param);

    constexpr uint32_t MakeAddress(
        uint8_t bus, uint8_t dev, uint8_t func, uint8_t addr)
    {
        auto Shl = [](uint32_t x, unsigned int bits)
        {
            return static_cast<uint32_t>(x) << bits;
        };

        return 0x80000000u // enable bit
            | Shl(bus, 16)
            | Shl(dev, 11)
            | Shl(func, 8)
            | (addr & 0xfcu);
    }

    void WriteAddress(uint32_t address);
    void WriteData(uint32_t value);
    void WriteData(uint32_t address, uint32_t value);
    uint32_t ReadData();
    uint32_t ReadData(uint32_t address);

    void ScanAllBus(ScanCallbackType* callback);

    struct Capability
    {
        uint8_t cap_id;
        uint8_t next_ptr;
    };

    class Device
    {
        const uint8_t bus_, dev_, func_, header_type_;
    public:
        Device(uint8_t bus, uint8_t dev, uint8_t func, uint8_t header_type)
            : bus_(bus), dev_(dev), func_(func), header_type_(header_type)
        {}

        virtual ~Device() = default;
        Device(const Device&) = default;
        Device& operator =(const Device&) = default;

        uint32_t ReadConfReg(uint8_t addr);
        void WriteConfReg(uint8_t addr, uint32_t value);

        uint8_t ReadCapabilityPointer()
        {
            return ReadConfReg(0x34);
        }

        Capability ReadCapabilityStructure(uint8_t addr);
    };

    class NormalDevice : public Device
    {
    public:
        NormalDevice(uint8_t bus, uint8_t dev, uint8_t func)
            : Device(bus, dev, func, 0)
        {}
    };

    class PciPciBridge : public Device
    {
        PciPciBridge(uint8_t bus, uint8_t dev, uint8_t func)
            : Device(bus, dev, func, 1)
        {}
    };

    constexpr uint8_t CalcBarAddress(unsigned int bar_index)
    {
        return 0x10 + 4 * bar_index;
    }

    WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index);
    Error WriteBar32(Device& device, unsigned int bar_index, uint32_t value);
    Error WriteBar64(Device& device, unsigned int bar_index, uint64_t value);

    WithError<uint64_t> CalcBarMapSize(Device& device, unsigned int bar_index);

    struct PcieCapability : public Capability
    {
        uint16_t pcie_cap;
        uint32_t device_cap;
        uint16_t device_control;
        uint16_t device_status;
        uint32_t link_cap;
        uint16_t link_control;
        uint16_t link_status;
        uint32_t slot_cap;
        uint16_t slot_control;
        uint16_t slot_status;
        uint16_t root_control;
        uint32_t root_status;
    };

    PcieCapability ReadPcieCapabilityStructure(Device& device, uint8_t addr);

    struct MSICapability
    {
        union Header_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t capability_id : 8;
                uint32_t next_pointer : 8;
                uint32_t msi_enable : 1;
                uint32_t multi_msg_capable : 3;
                uint32_t multi_msg_enable : 3;
                uint32_t addr_64_capable : 1;
                uint32_t per_vector_mask_capable : 1;
                uint32_t : 7;
            } bits;
        };

        Header_Bitmap header;
        uint32_t msg_addr;
        uint32_t msg_upper_addr;
        uint32_t msg_data;
        uint32_t mask_bits;
        uint32_t pending_bits;
    };

    MSICapability ReadMSICapabilityStructure(Device& device, uint8_t addr);
    void WriteMSICapabilityStructure(Device& device, uint8_t addr, const MSICapability& msi_cap);

    struct MSIXTableEntry
    {
        MemMapRegister32 msg_addr;
        MemMapRegister32 msg_upper_addr;
        MemMapRegister32 msg_data;
        MemMapRegister32 vector_control;
    };

    struct MSIXCapability
    {
        union Header_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t capability_id : 8;
                uint32_t next_pointer : 8;
                uint32_t table_size : 11;
                uint32_t : 3;
                uint32_t function_mask : 1;
                uint32_t msix_enable : 1;
            } bits;
        };

        union Offset_Bitmap
        {
            uint32_t data;
            struct
            {
                uint32_t bir : 3;
                uint32_t offset : 29;
            } bits;
        };

        Header_Bitmap header;
        Offset_Bitmap table;
        Offset_Bitmap pba;
    };

    MSIXCapability ReadMSIXCapabilityStructure(Device& device, uint8_t addr);
}

#endif // PCI_HPP_
