#include "pci.hpp"

#include "asmfunc.h"
#include "bitutil.hpp"
#include <stdio.h>

namespace
{
    using namespace bitnos::pci;

    // common
    uint16_t GetVendorId(uint8_t bus, uint8_t dev, uint8_t func)
    {
        return ReadData(MakeAddress(bus, dev, func, 0x00)) & 0xffffu;
    }

    // common
    uint16_t GetDeviceId(uint8_t bus, uint8_t dev, uint8_t func)
    {
        return ReadData(MakeAddress(bus, dev, func, 0x00)) >> 16;
    }

    // common
    uint8_t GetHeaderType(uint8_t bus, uint8_t dev, uint8_t func)
    {
        return (ReadData(MakeAddress(bus, dev, func, 0x0c)) >> 16) & 0xffu;
    }

    // common
    uint32_t GetClassId(uint8_t bus, uint8_t dev, uint8_t func)
    {
        return ReadData(MakeAddress(bus, dev, func, 0x08));
    }

    // header type 1
    uint32_t GetBusNumbers(uint8_t bus, uint8_t dev, uint8_t func)
    {
        return ReadData(MakeAddress(bus, dev, func, 0x18));
    }

    bool IsSingleFunctionDevice(uint8_t header_type)
    {
        return (header_type & 0x80u) == 0;
    }
}

namespace bitnos::pci
{
    void WriteAddress(uint32_t address)
    {
        IoOut32(kConfigAddress, address);
    }

    void WriteData(uint32_t value)
    {
        IoOut32(kConfigData, value);
    }

    void WriteData(uint32_t address, uint32_t value)
    {
        WriteAddress(address);
        WriteData(value);
    }

    uint32_t ReadData()
    {
        return IoIn32(kConfigData);
    }

    uint32_t ReadData(uint32_t address)
    {
        WriteAddress(address);
        return ReadData();
    }

    class BusScanner
    {
        ScanCallbackType* callback_;

        void ScanDev(uint8_t bus, uint8_t dev)
        {
            auto vendor_id = GetVendorId(bus, dev, 0);
            if (vendor_id == 0xffffu)
            {
                return;
            }
            auto header_type = GetHeaderType(bus, dev, 0);
            ScanFunc(bus, dev, 0, header_type);

            if (IsSingleFunctionDevice(header_type))
            {
                return;
            }

            for (uint8_t func = 1; func < 8; ++func)
            {
                if (GetVendorId(bus, dev, func) != 0xffffu)
                {
                    auto header_type = GetHeaderType(bus, dev, func);
                    ScanFunc(bus, dev, func, header_type);
                }
            }
        }

        void ScanFunc(uint8_t bus, uint8_t dev, uint8_t func, uint8_t header_type)
        {
            auto class_id = GetClassId(bus, dev, func);
            uint8_t base = (class_id >> 24) & 0xffu;
            uint8_t sub = (class_id >> 16) & 0xffu;
            uint8_t interface = (class_id >> 8) & 0xffu;

            callback_(
                {
                    bus, dev, func,
                    GetDeviceId(bus, dev, func),
                    GetVendorId(bus, dev, func),
                    base, sub, interface, header_type
                });

            if ((header_type & 0x7fu) == 1 && base == 0x06 && sub == 0x04)
            {
                // PCI-PCI bridge
                auto bus_numbers = GetBusNumbers(bus, dev, func);
                uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
                Scan(secondary_bus);
            }
        }

    public:
        BusScanner(ScanCallbackType* callback)
            : callback_(callback)
        {}

        void Scan(uint8_t bus)
        {
            for (uint8_t dev = 0; dev < 32; ++dev)
            {
                ScanDev(bus, dev);
            }
        }
    };

    void ScanAllBus(ScanCallbackType* callback)
    {
        BusScanner scanner(callback);

        auto header_type = GetHeaderType(0, 0, 0);
        if (IsSingleFunctionDevice(header_type))
        {
            scanner.Scan(0);
            return;
        }

        for (uint8_t func = 0; func < 8; ++func)
        {
            if (GetVendorId(0, 0, func) != 0xffffu)
            {
                break;
            }
            scanner.Scan(func);
        }
    }

    uint32_t Device::ReadConfReg(uint8_t addr)
    {
        return ReadData(MakeAddress(bus_, dev_, func_, addr));
    }

    void Device::WriteConfReg(uint8_t addr, uint32_t value)
    {
        WriteData(MakeAddress(bus_, dev_, func_, addr), value);
    }

    Capability Device::ReadCapabilityStructure(uint8_t addr)
    {
        auto cap = ReadConfReg(addr);
        return {
            static_cast<uint8_t>(cap & 0xffu), // cap id
            static_cast<uint8_t>((cap >> 8) & 0xffu) // next ptr
        };
    }

    WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index)
    {
        if (bar_index >= 6)
        {
            return {0, errorcode::kIndexOutOfRange};
        }

        const auto addr = CalcBarAddress(bar_index);
        const auto bar = device.ReadConfReg(addr);

        if ((bar & 4u) == 0) // 32 bit address
        {
            return {bar, errorcode::kSuccess};
        }

        // 64 bit address
        if (bar_index >= 5)
        {
            return {0, errorcode::kIndexOutOfRange};
        }

        const auto bar_upper = device.ReadConfReg(addr + 4);
        return {
            bar | (static_cast<uint64_t>(bar_upper) << 32),
            errorcode::kSuccess
        };
    }

    Error WriteBar32(Device& device, unsigned int bar_index, uint32_t value)
    {
        if (bar_index >= 6)
        {
            return errorcode::kIndexOutOfRange;
        }

        device.WriteConfReg(CalcBarAddress(bar_index), value);
        return errorcode::kSuccess;
    }

    Error WriteBar64(Device& device, unsigned int bar_index, uint64_t value)
    {
        if (bar_index >= 5)
        {
            return errorcode::kIndexOutOfRange;
        }

        const auto addr = CalcBarAddress(bar_index);
        device.WriteConfReg(addr, value & 0xffffffffu);
        device.WriteConfReg(addr + 4, (value >> 32) & 0xffffffffu);
        return errorcode::kSuccess;
    }

    WithError<uint64_t> CalcBarMapSize(Device& device, unsigned int bar_index)
    {
        const auto bar = ReadBar(device, bar_index);
        if (IsError(bar.error))
        {
            return {0, bar.error};
        }

        const bool bar_is_64bits = (bar.value & 4u) != 0;

        bar_is_64bits ? WriteBar64(device, bar_index, 0xffffffffffffffffu)
                      : WriteBar32(device, bar_index, 0xffffffffu);

        const auto temp_bar = ReadBar(device, bar_index);
        const auto temp_bar_val = bitutil::ClearBits(temp_bar.value, 0xfu);

        // resume original value
        bar_is_64bits ? WriteBar64(device, bar_index, bar.value)
                      : WriteBar32(device, bar_index, bar.value);

        if (temp_bar_val == 0)
        {
            return {0, errorcode::kSuccess};
        }
        return {
            static_cast<uint64_t>(1) << bitutil::BitScanForward(temp_bar_val),
            errorcode::kSuccess
        };
    }

    PcieCapability ReadPcieCapabilityStructure(Device& device, uint8_t addr)
    {
        PcieCapability pcie_cap;
        uint32_t reg;

        reg = device.ReadConfReg(addr);
        pcie_cap.pcie_cap = (reg >> 16) & 0xffffu;

        pcie_cap.device_cap = device.ReadConfReg(addr + 4 * 1);
        reg = device.ReadConfReg(addr + 4 * 2);
        pcie_cap.device_control = reg & 0xffffu;
        pcie_cap.device_status = (reg >> 16) & 0xffffu;

        pcie_cap.link_cap = device.ReadConfReg(addr + 4 * 3);
        reg = device.ReadConfReg(addr + 4 * 4);
        pcie_cap.link_control = reg & 0xffffu;
        pcie_cap.link_status = (reg >> 16) & 0xffffu;

        pcie_cap.slot_cap = device.ReadConfReg(addr + 4 * 5);
        reg = device.ReadConfReg(addr + 4 * 6);
        pcie_cap.slot_control = reg & 0xffffu;
        pcie_cap.slot_status = (reg >> 16) & 0xffffu;

        reg = device.ReadConfReg(addr + 4 * 7);
        pcie_cap.root_control = reg & 0xffffu;
        pcie_cap.root_status = device.ReadConfReg(addr + 4 * 8);

        return pcie_cap;
    }

    MSICapability ReadMSICapabilityStructure(Device& device, uint8_t addr)
    {
        MSICapability msi_cap{};

        msi_cap.header.data = device.ReadConfReg(addr);
        msi_cap.msg_addr = device.ReadConfReg(addr + 4);

        uint8_t msg_data_addr = addr + 8;
        if (msi_cap.header.bits.addr_64_capable)
        {
            msi_cap.msg_upper_addr = device.ReadConfReg(addr + 8);
            msg_data_addr = addr + 12;
        }

        msi_cap.msg_data = device.ReadConfReg(msg_data_addr);

        if (msi_cap.header.bits.per_vector_mask_capable)
        {
            msi_cap.mask_bits = device.ReadConfReg(msg_data_addr + 4);
            msi_cap.pending_bits = device.ReadConfReg(msg_data_addr + 8);
        }

        return msi_cap;
    }

    void WriteMSICapabilityStructure(Device& device, uint8_t addr, const MSICapability& msi_cap)
    {
        device.WriteConfReg(addr, msi_cap.header.data);
        device.WriteConfReg(addr + 4, msi_cap.msg_addr);

        uint8_t msg_data_addr = addr + 8;
        if (msi_cap.header.bits.addr_64_capable)
        {
            device.WriteConfReg(addr + 8, msi_cap.msg_upper_addr);
            msg_data_addr = addr + 12;
        }

        device.WriteConfReg(msg_data_addr, msi_cap.msg_data);

        if (msi_cap.header.bits.per_vector_mask_capable)
        {
            device.WriteConfReg(msg_data_addr + 4, msi_cap.mask_bits);
            device.WriteConfReg(msg_data_addr + 8, msi_cap.pending_bits);
        }
    }

    MSIXCapability ReadMSIXCapabilityStructure(Device& device, uint8_t addr)
    {
        MSIXCapability msix_cap;
        msix_cap.header.data = device.ReadConfReg(addr);
        msix_cap.table.data = device.ReadConfReg(addr + 4);
        msix_cap.pba.data = device.ReadConfReg(addr + 8);
        return msix_cap;
    }

    Error ConfigureMSI(Device& device, uint8_t cap_addr, uint32_t msg_addr,
                       uint32_t msg_data, unsigned int num_vector_exponent)
    {
        auto msi_cap = ReadMSICapabilityStructure(device, cap_addr);

        if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent)
        {
            msi_cap.header.bits.multi_msg_enable =
                msi_cap.header.bits.multi_msg_capable;
        }
        else
        {
            msi_cap.header.bits.multi_msg_enable = num_vector_exponent;
        }

        msi_cap.header.bits.msi_enable = 1;
        msi_cap.msg_addr = msg_addr;
        msi_cap.msg_data = msg_data;

        pci::WriteMSICapabilityStructure(device, cap_addr, msi_cap);
        return errorcode::kSuccess;
    }

    Error ConfigureMSIX(Device& device, uint8_t cap_addr, uint32_t msg_addr,
                       uint32_t msg_data, unsigned int num_vector)
    {
        auto msix_cap = ReadMSIXCapabilityStructure(device, cap_addr);
        return errorcode::kNotImplemented;
    }

    Error ConfigureMSI(Device& device, uint32_t msg_addr,
                       uint32_t msg_data, unsigned int num_vector_exponent)
    {
        uint8_t msi_cap_addr = 0, msix_cap_addr = 0;

        uint8_t cap_addr = device.ReadCapabilityPointer();
        while (cap_addr != 0)
        {
            auto cap = device.ReadCapabilityStructure(cap_addr);
            if (cap.cap_id == 0x05) // MSI
            {
                msi_cap_addr = cap_addr;
            }
            else if (cap.cap_id == 0x11) // MSIX
            {
                msix_cap_addr = cap_addr;
            }
            cap_addr = cap.next_ptr;
        }

        if (msi_cap_addr != 0)
        {
            return ConfigureMSI(device, msi_cap_addr,
                                msg_addr, msg_data, num_vector_exponent);
        }
        if (msix_cap_addr != 0)
        {
            return ConfigureMSIX(device, msix_cap_addr,
                                 msg_addr, msg_data, num_vector_exponent);
        }
        return errorcode::kNoMSICapability;
    }
}
