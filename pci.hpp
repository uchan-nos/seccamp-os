#ifndef PCI_HPP_
#define PCI_HPP_

#include <stdint.h>
#include "errorcode.hpp"

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
}

#endif // PCI_HPP_
