#include "command.hpp"

#include <stdio.h>
#include <string.h>

#include "bootparam.h"
#include "pci.hpp"
#include "xhci.hpp"
#include "xhci_trb.hpp"
#include "xhci_er.hpp"
#include "bitutil.hpp"
#include "printk.hpp"

extern BootParam* kernel_boot_param;

namespace
{
    using namespace bitnos;

    void Echo(int argc, char* argv[])
    {
        if (argc > 1)
        {
            printk("%s\n", argv[1]);
        }
        for (int i = 2; i < argc; ++i)
        {
            printk(" %s\n", argv[i]);
        }
        printk("\n");
    }

    pci::ScanCallbackParam pci_devices[32];
    size_t num_pci_devices = 0;

    void Lspci(int argc, char* argv[])
    {
        if (num_pci_devices == 0)
        {
            pci::ScanAllBus([](const pci::ScanCallbackParam& param)
                {
                    pci_devices[num_pci_devices++] = param;
                });
        }

        if (argc == 1)
        {
            for (size_t i = 0; i < num_pci_devices; ++i)
            {
                const auto& param = pci_devices[i];
                printk("%02x:%02x.%02x"
                    " DEVICE %04x, VENDOR %04x"
                    " CLASS %02x.%02x.%02x HT %02x\n",
                    param.bus, param.dev, param.func,
                    param.device_id, param.vendor_id,
                    param.base_class, param.sub_class,
                    param.interface, param.header_type);
            }
        }
        else if (argc == 2)
        {
            unsigned int bus, dev, func;
            auto res = sscanf(argv[1], "%02x:%02x.%02x", &bus, &dev, &func);
            if (res != 3)
            {
                printk("Invalid device specifier: %s\n", argv[1]);
                return;
            }

            int device_index = -1;
            for (int i = 0; i < num_pci_devices; ++i)
            {
                if (pci_devices[i].bus == bus
                        && pci_devices[i].dev == dev
                        && pci_devices[i].func == func)
                {
                    device_index = i;
                    break;
                }
            }

            if (device_index < 0)
            {
                printk("No such device: %s\n", argv[1]);
                return;
            }

            const auto& param = pci_devices[device_index];
            printk("%02x:%02x.%02x"
                " DEVICE %04x, VENDOR %04x"
                " CLASS %02x.%02x.%02x HT %02x\n",
                param.bus, param.dev, param.func,
                param.device_id, param.vendor_id,
                param.base_class, param.sub_class,
                param.interface, param.header_type);

            auto device = pci::Device(bus, dev, func, param.header_type);

            auto status_command = device.ReadConfReg(0x04);
            printk("status %04x, command %04x\n",
                (status_command >> 16) & 0xffffu,
                status_command & 0xffffu);

            for (int i = 0; i < 6; ++i)
            {
                auto bar = device.ReadConfReg(0x10 + 4 * i);
                printk("bar %d: %08x\n", i, bar);
            }

            printk("bar map size: %lx\n", CalcBarMapSize(device, 0).value);

            uint8_t cap_ptr = device.ReadCapabilityPointer();
            while (cap_ptr != 0)
            {
                auto cap = device.ReadCapabilityStructure(cap_ptr);
                if (cap.cap_id == 0x10)
                {
                    printk("Cap ID 0x10 (PCI Express)\n");
                    auto pcie_cap = pci::ReadPcieCapabilityStructure(device, cap_ptr);
                    printk(" PCIeCap: %04x, DevCap: %08x, DevCon: %04x, DevStat: %04x\n",
                        pcie_cap.pcie_cap, pcie_cap.device_cap,
                        pcie_cap.device_control, pcie_cap.device_status);
                }
                else
                {
                    printk("Cap ID %02x\n", cap.cap_id);
                }
                cap_ptr = cap.next_ptr;
            }
        }
    }

    const char* GetMemoryType(EFI_MEMORY_TYPE Type) {
        switch (Type) {
            case EfiReservedMemoryType: return "EfiReservedMemoryType";
            case EfiLoaderCode: return "EfiLoaderCode";
            case EfiLoaderData: return "EfiLoaderData";
            case EfiBootServicesCode: return "EfiBootServicesCode";
            case EfiBootServicesData: return "EfiBootServicesData";
            case EfiRuntimeServicesCode: return "EfiRuntimeServicesCode";
            case EfiRuntimeServicesData: return "EfiRuntimeServicesData";
            case EfiConventionalMemory: return "EfiConventionalMemory";
            case EfiUnusableMemory: return "EfiUnusableMemory";
            case EfiACPIReclaimMemory: return "EfiACPIReclaimMemory";
            case EfiACPIMemoryNVS: return "EfiACPIMemoryNVS";
            case EfiMemoryMappedIO: return "EfiMemoryMappedIO";
            case EfiMemoryMappedIOPortSpace: return "EfiMemoryMappedIOPortSpace";
            case EfiPalCode: return "EfiPalCode";
            case EfiPersistentMemory: return "EfiPersistentMemory";
            case EfiMaxMemoryType: return "EfiMaxMemoryType";
            default: return "InvalidMemoryType";
        }
    }

    void Mmap(int argc, char* argv[])
    {
        const auto base =
            reinterpret_cast<uintptr_t>(kernel_boot_param->memory_map);
        const auto mmap_size = kernel_boot_param->memory_map_size;
        const auto desc_size = kernel_boot_param->memory_descriptor_size;

        decltype(EFI_MEMORY_DESCRIPTOR::PhysicalStart) last_addr = 0;
        for (size_t it = 0; it < mmap_size; it += desc_size)
        {
            auto desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(base + it);

            const bool free = desc->Type == EfiBootServicesCode
                || desc->Type == EfiBootServicesData
                || desc->Type == EfiConventionalMemory;

            printk("%c %08lx - %08lx: %05lx pages %s (%s)\n",
                last_addr > 0 && last_addr == desc->PhysicalStart ? '*' : ' ',
                desc->PhysicalStart,
                desc->PhysicalStart + desc->NumberOfPages * 4096 - 1,
                desc->NumberOfPages,
                free ? "Free" : "    ",
                GetMemoryType(static_cast<EFI_MEMORY_TYPE>(desc->Type))
                );

            last_addr = desc->PhysicalStart + desc->NumberOfPages * 4096;
        }
    }

    void Xhci(int argc, char* argv[])
    {
        if (num_pci_devices == 0)
        {
            pci::ScanAllBus([](const pci::ScanCallbackParam& param)
                {
                    pci_devices[num_pci_devices++] = param;
                });
        }

        size_t xhci_dev_index = 0;
        for (xhci_dev_index = 0; xhci_dev_index < num_pci_devices; ++xhci_dev_index)
        {
            const auto& p = pci_devices[xhci_dev_index];
            if (p.base_class == 0x0c && p.sub_class == 0x03 && p.interface == 0x30)
            {
                printk("found an xHCI device at %02x:%02x.%02x\n",
                    p.bus, p.dev, p.func);
                break;
            }
        }

        if (xhci_dev_index == num_pci_devices)
        {
            printk("no xHCI device\n");
            return;
        }

        const auto& dev_param = pci_devices[xhci_dev_index];
        pci::NormalDevice xhci_dev(dev_param.bus, dev_param.dev, dev_param.func);
        const auto bar = pci::ReadBar(xhci_dev, 0);
        const auto mmio_base = bitutil::ClearBits(bar.value, 0xf);

        xhci::Controller xhc(mmio_base);
        auto& cap_reg = xhc.CapabilityRegisters();
        auto& op_reg = xhc.OperationalRegisters();

        printk("CAPLENGTH=%02x HCIVERSION=%04x"
            " DBOFF=%08x RTSOFF=%08x"
            " HCSPARAMS1=%08x HCCPARAMS1=%08x\n",
            cap_reg.ReadCAPLENGTH(), cap_reg.ReadHCIVERSION(),
            cap_reg.DBOFF.Read(), cap_reg.RTSOFF.Read(),
            cap_reg.HCSPARAMS1.Read(), cap_reg.HCCPARAMS1.Read());

        printk("USBCMD=%08x USBSTS=%08x DCBAAP=%016lx CONFIG=%08x\n",
            op_reg.USBCMD.Read(), op_reg.USBSTS.Read(),
            op_reg.DCBAAP.Read(), op_reg.CONFIG.Read());

        const auto max_ports = xhci::MaxPorts(xhc);
        const auto max_slots_enabled = xhci::MaxSlotsEnabled(xhc);

        alignas(64) xhci::TRB cr_buf[8];
        size_t cr_enqueue_ptr = 0;
        unsigned char cr_cycle_bit = 1;
        memset(cr_buf, 0, sizeof(cr_buf));
        op_reg.CRCR.Write(reinterpret_cast<uint64_t>(&cr_buf[0]) | cr_cycle_bit);
        printk("Write to CRCR cr_buf=%016lx\n", reinterpret_cast<uint64_t>(&cr_buf[0]));


        auto interrupter_reg_sets = xhc.InterrupterRegSets();
        auto doorbell_registers = xhc.DoorbellRegisters();
        auto& int0_reg = interrupter_reg_sets[0];
        xhci::eventring::Manager er_mgr(int0_reg);
        er_mgr.Initialize();

        if (er_mgr.HasFront())
        {
            printk("ER has at least one element\n");
        }
        else
        {
            printk("ER has no element\n");
        }

        while (er_mgr.HasFront())
        {
            auto& trb = er_mgr.Front();
            printk("%08x %08x %08x %08x (type=%u)\n",
                trb.dwords[0], trb.dwords[1], trb.dwords[2], trb.dwords[3], trb.bits.trb_type);
        }

        xhc.Initialize();

        printk("CCS+CSC: ");
        for (auto& port_reg : xhc.PortRegSets())
        {
            const auto port_status = port_reg.PORTSC.Read();
            printk("%c", '0' + (port_status & 1u) + 2 * ((port_status >> 17) & 1));
        }
        printk("\n");

        printk("SEGM TABLE Base=%016lx Size=%lu\n",
            reinterpret_cast<uint64_t>(er_mgr.SegmentTable()),
            er_mgr.SegmentTableSize());
        for (size_t st_index = 0; st_index < er_mgr.SegmentTableSize(); st_index++)
        {
            const auto& seg_table = er_mgr.SegmentTable()[st_index];
            printk("SEGM TABLE %u: Base=%016lx Size=%u\n",
                st_index,
                seg_table.segment_base_address_,
                seg_table.segment_size_);
        }

        uint64_t rtsbase = mmio_base + cap_reg.RTSOFF.Read();
        uint32_t* ir0 = reinterpret_cast<uint32_t*>(rtsbase + 0x20u);
        printk("ERSTSZ=%u ERSTBA=%016lx ERDP=%016lx\n",
            ir0[2],
            ir0[4] | (static_cast<uint64_t>(ir0[5]) << 32),
            ir0[6] | (static_cast<uint64_t>(ir0[7]) << 32));

        int slot_id = -1;
        for (auto dcaddr : xhc.DeviceContextAddresses())
        {
            ++slot_id;
            if (dcaddr == nullptr)
            {
                continue;
            }

            const auto& sc = dcaddr->slot_context;
            printk("DevCtx %u: Hub=%d, #EP=%d, slot state %02x, usb dev addr %02x\n",
                slot_id, sc.bits.hub, sc.bits.context_entries,
                sc.bits.slot_state, sc.bits.usb_device_address);

            for (size_t ep_index = 0; ep_index < sc.bits.context_entries; ++ep_index)
            {
                const auto& ep = dcaddr->ep_contexts[ep_index];
                printk("EP %u: state %u, type %u, MaxBSiz %u, MaxPSiz %u\n",
                    ep_index, ep.bits.ep_state, ep.bits.ep_type,
                    ep.bits.max_burst_size, ep.bits.max_packet_size);
            }

            xhci::InputContext input_context;
            memset(&input_context, 0, sizeof(input_context));
            const unsigned int ep_enabling = 1;
            input_context.input_control_context.add_context_flags
                |= (1 << (2 * ep_enabling));
            input_context.ep_contexts[ep_enabling] = dcaddr->ep_contexts[ep_enabling];

            xhci::ConfigureEndpointCommandTRB cmd{};
            cmd.bits.cycle_bit = cr_cycle_bit;
            cmd.bits.input_context_pointer = (
                reinterpret_cast<uint64_t>(&input_context) >> 4);
            cmd.bits.trb_type = 12;
            cmd.bits.slot_id = slot_id;

            for (int i = 0; i < 4; ++i)
            {
                cr_buf[0].dwords[i] = cmd.dwords[i];
            }

            printk("Put a command to %p\n", cr_buf[0].dwords);

            printk("writing doorbell. R/S=%u, CRR=%u\n",
                op_reg.USBCMD.Read() & 1u, (op_reg.CRCR.Read() >> 3) & 1u);

            doorbell_registers[0].DB.Write(0);

            printk("doorbell written. R/S=%u, CRR=%u\n",
                op_reg.USBCMD.Read() & 1u, (op_reg.CRCR.Read() >> 3) & 1u);

            while ((op_reg.CRCR.Read() & 8) == 0);

            printk("CRCR %016lx\n", op_reg.CRCR.Read());

            printk("CRCR.CRR got to be 1. R/S=%u, CRR=%u\n",
                op_reg.USBCMD.Read() & 1u, (op_reg.CRCR.Read() >> 3) & 1u);

            printk("waiting completion event (c=%d)\n",
                er_mgr.Front().bits.cycle_bit);

            while (!er_mgr.HasFront());

            auto trb = er_mgr.Front();
            er_mgr.Pop();

            printk("completed! TRB type=%u ptr=%016lx\n",
                trb.bits.trb_type, &trb);

        }

        /*
        const uint64_t* dcbaap =
            reinterpret_cast<uint64_t*>(op_reg.Read64(op_reg.DCBAAP));
        for (size_t i = 0; i < max_slots_enabled; ++i)
        {
            const auto dc_base_addr = dcbaap[i];
            if (!dc_base_addr)
            {
                continue;
            }

            const auto slot_info =
                *reinterpret_cast<uint32_t*>(dc_base_addr + 0x00);
            const auto slot_state_dev_addr =
                *reinterpret_cast<uint32_t*>(dc_base_addr + 0x0c);
            const auto num_endpoint = slot_info >> 27;
            printk("DevCtx %u: Hub=%d, #EP=%d, slot state %02x, usb dev addr %02x\n",
                i, (slot_info >> 26) & 1u, num_endpoint,
                slot_state_dev_addr >> 27, slot_state_dev_addr & 0xffu);

            for (size_t ep = 1; ep <= num_endpoint; ++ep)
            {
                const auto ep_addr = dc_base_addr + 0x20u * ep;
                const auto ep_state =
                    *reinterpret_cast<uint32_t*>(ep_addr + 0x00);
                const auto ep_type =
                    *reinterpret_cast<uint32_t*>(ep_addr + 0x04);
                printk("EP %u: state %u, type %u, MaxBSiz %u, MaxPSiz %u\n",
                    ep, ep_state & 0x7u, (ep_type >> 3) & 0x7u,
                    (ep_type >> 8) & 0xffu, ep_type >> 16);
            }

            uint32_t input_context[8 * 33];
            const unsigned int ep_enabling = 2;
            memset(input_context, 0, sizeof(input_context));
            input_context[8 * 0 + 1] = 1 << ep_enabling;
            memcpy(input_context + 8 * (1 + ep_enabling),
                reinterpret_cast<void*>(dc_base_addr + 32 * ep_enabling), 32);

            volatile uint32_t* cr_base =
                reinterpret_cast<uint32_t*>(bitutil::ClearBits(crcr, 0x3fu));

            auto unsigned int cycle_state = crcr & 1;
            const auto input_context_base =
                reinterpret_cast<uint64_t>(input_context);
            cr_base[0] = input_context_base & 0xffffffffu;
            cr_base[1] = input_context_base >> 32;
            cr_base[3] =
                (i << 24)    // Slot ID
                | (12 << 10) // TRB Type
                | cycle_state;

            printk("cr_base %016lx, input_context %016lx\n",
                reinterpret_cast<uint64_t>(cr_base), input_context_base);

            const uint64_t ers0 = reinterpret_cast<uint64_t*>(erst_base)[0];
            const uint16_t ers0_size = reinterpret_cast<uint64_t*>(erst_base)[1] & 0xffffu;
            const uint64_t erdp =
                bitutil::ClearBits(interrupter0_registers[6], 0xfu) |
                (static_cast<uint64_t>(interrupter0_registers[7]) << 32);
            const volatile uint32_t* er_entry = reinterpret_cast<uint32_t*>(erdp);
            printk("erst[0] = %016lx, sizeof erst[0] = %u, erdp = %016lx\n",
                ers0, ers0_size, erdp);
            printk("waiting completion event (c=%d)\n", er_entry[3] & 1);

            volatile uint32_t* doorbell = reinterpret_cast<uint32_t*>(
                bitutil::ClearBits(cap_reg.Read(cap_reg.DBOFF), 3u));
            doorbell[0] = 0;

            crcr = op_reg.Read64(op_reg.CRCR);
            printk("CRCR %016lx, USBCMD %08x\n", crcr, op_reg.Read32(op_reg.USBCMD));

            while ((er_entry[3] & 1) == 0);

            const uint64_t command_trb =
                bitutil::ClearBits(er_entry[0], 0xfu) |
                (static_cast<uint64_t>(er_entry[1]) << 32);

            printk("completed! (c=%d) Command TRB %016lx\n",
                er_entry[3] & 1, command_trb);
        }
    */
    }

    alignas(0x1000) uint64_t page_dir_ptr_table[512];
    alignas(0x1000) uint64_t page_directory[512];

    void Pages(int argc, char* argv[])
    {
        memset(page_dir_ptr_table, 0, sizeof(page_dir_ptr_table));
        memset(page_directory, 0, sizeof(page_directory));

        uint64_t cr3;
        __asm__("mov %%cr3, %0\n\t"
                : "=r"(cr3));
        printk("CR3=%016lx\n", cr3);

        const uint32_t* addr = reinterpret_cast<uint32_t*>(0x7e60000u);
        const uint32_t* addr_virt = reinterpret_cast<uint32_t*>(0xfffff00000060000lu);
        printk("%08x\n", *addr);
        //printk("%08x\n", *addr_virt);

        uint64_t* pml4 = reinterpret_cast<uint64_t*>(bitutil::ClearBits(cr3, 0xfffu));
        page_dir_ptr_table[0] = reinterpret_cast<uint64_t>(page_directory) | 0x03u;
        page_directory[0] = bitutil::ClearBits(0x7e60000u, 0x1fffffu) | 0x83u;

        pml4[0x1e0] = reinterpret_cast<uint64_t>(page_dir_ptr_table) | 0x03u;

        printk("%08x\n", *addr);
        printk("accessing to page\n");

        printk("%08x\n", *addr_virt);
        printk("successed!\n");
    }
}

namespace bitnos::command
{
    Command table[5] = {
        {"echo", Echo},
        {"lspci", Lspci},
        {"mmap", Mmap},
        {"xhci", Xhci},
        {"pages", Pages},
    };
}
