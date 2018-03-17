#include "command.hpp"

#include <stdio.h>
#include <string.h>

#include "memory_op.hpp"
#include "bootparam.h"
#include "pci.hpp"
#include "msr.hpp"
#include "xhci.hpp"
#include "xhci_trb.hpp"
#include "xhci_tr.hpp"
#include "xhci_er.hpp"
#include "xhci_cr.hpp"
#include "bitutil.hpp"
#include "printk.hpp"
#include "mutex.hpp"

#include "usb/xhci/xhci.hpp"

extern BootParam* kernel_boot_param;

//namespace
//{
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

        bool only_used = false;
        for (int i = 0; i < argc; ++i)
        {
            if (strcmp("--used", argv[i]) == 0)
            {
                only_used = true;
            }
        }

        decltype(EFI_MEMORY_DESCRIPTOR::PhysicalStart) last_addr = 0;
        for (size_t it = 0; it < mmap_size; it += desc_size)
        {
            auto desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(base + it);

            const bool free = desc->Type == EfiBootServicesCode
                || desc->Type == EfiBootServicesData
                || desc->Type == EfiConventionalMemory;

            if (only_used && !free)
            {
                printk("%c %08lx - %08lx: %05lx pages %s (%s)\n",
                    last_addr > 0 && last_addr == desc->PhysicalStart ? '*' : ' ',
                    desc->PhysicalStart,
                    desc->PhysicalStart + desc->NumberOfPages * 4096 - 1,
                    desc->NumberOfPages,
                    free ? "Free" : "    ",
                    GetMemoryType(static_cast<EFI_MEMORY_TYPE>(desc->Type))
                    );
            }

            last_addr = desc->PhysicalStart + desc->NumberOfPages * 4096;
        }
    }

    void ShowEventTRB(xhci::TRB& trb)
    {
        if (trb.bits.trb_type == 32) // Transfer Event TRB
        {
            auto te_trb = reinterpret_cast<xhci::TransferEventTRB&>(trb);
            if (te_trb.bits.event_data)
            {
                printk("Event Data TRB Completed: "
                    "Code %u (%s), TRB transfer len %u, EP %u, Slot %u, Data %08x:08x\n",
                    te_trb.bits.completion_code,
                    xhci::trb_completion_code_names[te_trb.bits.completion_code],
                    te_trb.bits.trb_transfer_length,
                    te_trb.bits.endpoint_id, te_trb.bits.slot_id,
                    te_trb.bits.trb_pointer >> 32,
                    te_trb.bits.trb_pointer & 0xffffffffu);
            }
            else
            {
                auto& issue_trb = *reinterpret_cast<xhci::TRB*>(
                    te_trb.bits.trb_pointer);
                printk("Transfer Completed: "
                    "Code %u (%s), TRB transfer len %u, EP %u, Slot %u, TRB %016lx (%s)\n",
                    te_trb.bits.completion_code,
                    xhci::trb_completion_code_names[te_trb.bits.completion_code],
                    te_trb.bits.trb_transfer_length,
                    te_trb.bits.endpoint_id, te_trb.bits.slot_id,
                    te_trb.bits.trb_pointer,
                    xhci::trb_type_names[issue_trb.bits.trb_type]);
            }
        }
        else if (trb.bits.trb_type == 33) // Command Completion TRB
        {
            auto cmd_trb = reinterpret_cast<xhci::CommandCompletionEventTRB&>(trb);
            auto& issue_trb = *reinterpret_cast<xhci::TRB*>(
                cmd_trb.bits.command_trb_pointer << 4);
            printk("Command Completed: "
                "Code %u (%s), Param %u, Slot ID %u, TRB %016lx (%s)\n",
                cmd_trb.bits.completion_code,
                xhci::trb_completion_code_names[cmd_trb.bits.completion_code],
                cmd_trb.bits.command_completion_parameter,
                cmd_trb.bits.slot_id, &issue_trb,
                xhci::trb_type_names[issue_trb.bits.trb_type]);
        }
        else
        {
            printk("unknown Event TRB: type %s\n",
                xhci::trb_type_names[trb.bits.trb_type]);
        }
    }

    void DiscardAllEvents(xhci::Controller* xhc)
    {
        while (xhc->EventRingManager().HasFront())
        {
            auto trb = xhc->EventRingManager().Front();
            xhc->EventRingManager().Pop();
        }
    }

    void ShowPORTSC(xhci::PortRegSet& port_reg)
    {
        auto portsc = port_reg.PORTSC.Read();
        auto bits = portsc.bits;
        printk("  PORTSC: %08x, (PP,CCS,PED,PR)=(%u,%u,%u,%u)\n",
            portsc.data,
            bits.port_power, bits.current_connect_status,
            bits.port_enabled_disabled, bits.port_reset);
    }

    alignas(64) uint8_t tr_for_ep0_buf[sizeof(xhci::transferring::Manager) * 16];
    xhci::transferring::Manager* tr_mgr_for_ep0[16];

    const size_t kTRBufSize = 32;
    alignas(64) uint8_t tr_buf[sizeof(xhci::transferring::Manager) * kTRBufSize];
    bool tr_buf_assigned[kTRBufSize] = {};

    xhci::transferring::Manager* tr_mgr_for_keyboard;

    template <typename... Args>
    xhci::transferring::Manager* AllocateTRManager(Args... args)
    {
        for (size_t i = 0; i < kTRBufSize; ++i)
        {
            if (tr_buf_assigned[i] == false)
            {
                auto buf = tr_buf + sizeof(xhci::transferring::Manager) * i;
                return new(buf) xhci::transferring::Manager(args...);
            }
        }
        return nullptr;
    }

    void InitializeDeviceSlot(xhci::Controller* xhc, size_t slot_id, size_t port_index)
    {
        //printk("Initializing a device slot: %llu\n", slot_id);

        alignas(64) xhci::InputContext input_context{};
        input_context.EnableSlotContext();
        input_context.EnableEndpoint(0, false);

        auto& slot = input_context.GetSlotContext();
        auto& ep0 = input_context.GetEndpointContext(0, false);

        // route string について https://news.mynavi.jp/article/sopinion-303/
        slot.bits.route_string = 0;
        slot.bits.root_hub_port_num = port_index + 1; // port number is 1..255, port_index is 0 - 254.
        slot.bits.context_entries = 1;
        slot.bits.speed = xhc->PortRegSets()[port_index].PORTSC.Read().bits.port_speed;
        printk("Slot %llu: port %llu, speed %llu\n", slot_id, port_index + 1, slot.bits.speed);

        auto buf = tr_for_ep0_buf + sizeof(xhci::transferring::Manager) * slot_id;
        auto tr_mgr = new(buf) xhci::transferring::Manager(1, xhc->DoorbellRegisters(), slot_id);
        tr_mgr_for_ep0[slot_id] = tr_mgr;

        ep0.bits.ep_type = 4; // Control Endpoint. Bidirectional.
        switch (slot.bits.speed)
        {
        case 4: // Super Speed
            ep0.bits.max_packet_size = 512; // shall be set to 512 for control endpoints
            break;
        case 3: // High Speed
            ep0.bits.max_packet_size = 64;
            break;
        default:
            ep0.bits.max_packet_size = 8;
        }
        ep0.bits.max_burst_size = 0;
        ep0.bits.tr_dequeue_pointer = reinterpret_cast<uint64_t>(tr_mgr->Data()) >> 4;
        ep0.bits.dequeue_cycle_state = 1;
        ep0.bits.interval = 0;
        ep0.bits.max_primary_streams = 0;
        ep0.bits.mult = 0;
        ep0.bits.error_count = 3;

        xhc->LoadDeviceContextAddress(slot_id);

        xhci::AddressDeviceCommandTRB address_device_cmd{&input_context, static_cast<uint8_t>(slot_id)};

        //printk("pussing Address Device Command to command ring\n");
        xhc->CommandRingManager().Push(address_device_cmd);

        while (!xhc->EventRingManager().HasFront());

        while (xhc->EventRingManager().HasFront())
        {
            auto trb = xhc->EventRingManager().Front();
            xhc->EventRingManager().Pop();
            ShowEventTRB(trb);
        }
    }

    void InitializePort(xhci::Controller* xhc, size_t port_index)
    {
        auto& cr_mgr = xhc->CommandRingManager();
        auto& er_mgr = xhc->EventRingManager();

        auto& port_regs = xhc->PortRegSets()[port_index];
        auto& portsc = port_regs.PORTSC;

        if (portsc.Read().bits.current_connect_status == 0)
        {
            printk("port %lu is disconnected\n", port_index);
            return;
        }

        //if (portsc().port_power == 0)
        //{
        //    xhc->Port
        //}

        //printk("waiting port gets to be enabled\n");
        //while (portsc().port_enabled_disabled == 0);

        //ShowPORTSC(port_regs);

        while (portsc.Read().bits.port_enabled_disabled == 0);

        //ShowPORTSC(port_regs);

        DiscardAllEvents(xhc);

        //printk("port %lu is enabled\n", port_index);
        xhci::EnableSlotCommandTRB enable_slot_cmd{};

        //printk("pussing Enable Slot Command to command ring\n");
        cr_mgr.Push(enable_slot_cmd);
        //printk("Command Ring write index: %lu\n", cr_mgr.write_index());

        while (!er_mgr.HasFront());

        while (er_mgr.HasFront())
        {
            auto trb = er_mgr.Front();
            er_mgr.Pop();
            ShowEventTRB(trb);

            if (trb.bits.trb_type == 33) // Command Completion TRB
            {
                auto cmd_trb = reinterpret_cast<xhci::CommandCompletionEventTRB&>(trb);
                auto& issue_trb = *reinterpret_cast<xhci::TRB*>(
                    cmd_trb.bits.command_trb_pointer << 4);
                if (issue_trb.bits.trb_type == 9) // Enable Slot Command
                {
                    InitializeDeviceSlot(xhc, cmd_trb.bits.slot_id, port_index);
                }
            }
        }
    }

    void GetDescriptor(
        xhci::Controller* xhc,
        uint8_t slot_id,
        uint8_t descriptor_type, uint8_t descriptor_index,
        size_t len, uint8_t* buf)
    {
        xhci::SetupStageTRB setup{};
        // Direction=Dev2Host, Type=Standard, Recipient=Dev
        setup.bits.request_type = 0b10000000;
        setup.bits.request = 6; // get descriptor
        setup.bits.value = (static_cast<uint16_t>(descriptor_type) << 8) | descriptor_index;
        setup.bits.index = 0;
        setup.bits.length = len;
        setup.bits.transfer_type = 3; // IN Data Stage

        //auto& epc = ctx_to_send.ep_contexts[0];

        xhci::DataStageTRB data{};
        data.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(buf);
        data.bits.trb_transfer_length = len;
        //data.bits.td_size = (len + epc.bits.max_packet_size - 1) / epc.bits.max_packet_size;
        data.bits.td_size = 0;
        data.bits.direction = 1; // 1: IN
        data.bits.interrupt_on_completion = 1;

        xhci::StatusStageTRB status{};

        /*
         * SetupStage  01000680 00800000 00000008 00030840
         *  bmRequestType : 0x80 = 10000000B
         *  bRequest      : 0x06
         *  wValue        : 0x0100
         *    High byte is desc type : 1 = DEVICE descriptor
         *    Low byte is desc index : 0
         *  wIndex        : 0x0000
         *  wLength       : 0x0080
         *
         *  TRB Transfer Length : 0x00008
         *  Flags         : 00030840 = 00000000 00000011 00001000 01000000
         *    IOC, IDT    : 0, 1  (no interrupt on completion, immediate data)
         *    TRBType     : 2
         *    TRT         : 3  IN Data Stage (Device to host)
         *
         * DataStage   792c17a0 00000000 00040080 00010c00
         *  Data Buf Lo   : 0x792c17a0
         *  Data Buf Hi   : 0x00000000
         *  TRB Transfer Length : 0x00080
         *  TD Size       : 2
         *  Flags         : 00010c00 = 00000000 00000001 00001100 00000000
         *    ENT         : 0  (Not evaluate next TRB)
         *    ISP         : 0  (No interrupt on short packet)
         *    NS, CH      : 0, 0  (not permit to set no-snoop, not chained)
         *    IOC, IDT    : 0, 0  (no interrupt on completion, no immediate data)
         *    TRBType     : 3
         *    DIR         : 1  (IN)
         *
         * StatusStage 00000000 00000000 00000000 00001000
         *  Flags         : 00001000 = 00000000 00000000 00010000 00000000
         *    ENT         : 0
         *    CH          : 0
         *    IOC         : 0
         *    TRBType     : 4
         *    DIR         : 0  (OUT)
         */

        /*
        printk("Dump memory of 3 trbs.\n");
        uint32_t* p = &setup.dwords[0];
        printk("%016lx: %08x %08x %08x %08x\n", p, p[0], p[1], p[2], p[3]);
        p = &data.dwords[0];
        printk("%016lx: %08x %08x %08x %08x\n", p, p[0], p[1], p[2], p[3]);
        p = &status.dwords[0];
        printk("%016lx: %08x %08x %08x %08x\n", p, p[0], p[1], p[2], p[3]);
        */

        auto er_mgr = xhc->EventRingManager();

        auto dev_ctx = xhc->DeviceContextAddresses()[slot_id];
        auto ep0_ctx = &dev_ctx->ep_contexts[0];
        auto tr_mgr = tr_mgr_for_ep0[slot_id];

        DiscardAllEvents(xhc);

        /*
        xhci::NoOpTRB noop{};
        noop.bits.interrupt_on_completion = 1;
        printk("Pussing NoOp to TR of slot_id = %lu, tr front = %p\n",
            slot_id, tr_mgr->Debug());
        tr_mgr->Push(noop);

        printk("Waiting an event\n");
        while (!er_mgr.HasFront());

        while (er_mgr.HasFront())
        {
            auto trb = er_mgr.Front();
            er_mgr.Pop();
            ShowEventTRB(trb);
        }
        */

        //printk("Pussing to TR of slot_id = %lu, tr front = %p\n",
        //    slot_id, tr_mgr->Debug());
        tr_mgr->Push(setup, false);
        tr_mgr->Push(data, false);
        tr_mgr->Push(status);

        //printk("Waiting an event\n");
        while (!er_mgr.HasFront());

        while (er_mgr.HasFront())
        {
            auto trb = er_mgr.Front();
            er_mgr.Pop();
            //ShowEventTRB(trb);
        }

    }

    void SetConfiguration(
        xhci::Controller* xhc,
        uint8_t slot_id,
        uint8_t configuration_value)
    {
        xhci::SetupStageTRB setup{};
        setup.bits.request_type = 0b00000000;
        setup.bits.request = 9; // set configuration
        setup.bits.value = configuration_value;
        setup.bits.index = 0;
        setup.bits.length = 0;
        setup.bits.transfer_type = 0; // No Data Stage

        xhci::StatusStageTRB status{};
        status.bits.direction = 1;
        status.bits.interrupt_on_completion = 1;

        auto er_mgr = xhc->EventRingManager();

        auto dev_ctx = xhc->DeviceContextAddresses()[slot_id];
        auto ep0_ctx = &dev_ctx->ep_contexts[0];
        auto tr_mgr = tr_mgr_for_ep0[slot_id];

        //printk("Pussing to TR of slot_id = %lu, tr front = %p\n",
        //    slot_id, tr_mgr->Debug());
        tr_mgr->Push(setup, false);
        tr_mgr->Push(status);

        //printk("Waiting an event\n");
        while (!er_mgr.HasFront());

        while (er_mgr.HasFront())
        {
            auto trb = er_mgr.Front();
            er_mgr.Pop();
            //ShowEventTRB(trb);
        }

    }

    uint8_t GetConfiguration(
        xhci::Controller* xhc,
        uint8_t slot_id)
    {
        xhci::SetupStageTRB setup{};
        setup.bits.request_type = 0b10000000;
        setup.bits.request = 8; // get configuration
        setup.bits.value = 0;
        setup.bits.index = 0;
        setup.bits.length = 1;
        setup.bits.transfer_type = 3; // IN Data Stage

        uint8_t configuration_value;
        xhci::DataStageTRB data{};
        data.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(&configuration_value);
        data.bits.trb_transfer_length = 1;
        data.bits.td_size = 0;
        data.bits.direction = 1; // 1: IN

        xhci::StatusStageTRB status{};
        status.bits.interrupt_on_completion = 1;

        auto er_mgr = xhc->EventRingManager();

        auto dev_ctx = xhc->DeviceContextAddresses()[slot_id];
        auto ep0_ctx = &dev_ctx->ep_contexts[0];
        auto tr_mgr = tr_mgr_for_ep0[slot_id];

        //printk("Pussing to TR of slot_id = %lu, tr front = %p\n",
        //    slot_id, tr_mgr->Debug());
        tr_mgr->Push(setup, false);
        tr_mgr->Push(data, false);
        tr_mgr->Push(status);

        //printk("Waiting an event\n");
        while (!er_mgr.HasFront());

        while (er_mgr.HasFront())
        {
            auto trb = er_mgr.Front();
            er_mgr.Pop();
            //ShowEventTRB(trb);
        }

        return configuration_value;
    }


#if 0
    alignas(64) uint8_t xhc_buf[sizeof(xhci::Controller)];
    xhci::Controller* xhc;
#endif
    usb::xhci::Controller* xhc;

    unsigned long long GetCSCBits(xhci::PortRegSetArray port_reg_sets)
    {
        unsigned long long csc = 0;
        size_t i = 0;
        for (auto& port_reg : port_reg_sets)
        {
            const auto port_status = port_reg.PORTSC.Read();
            csc |= port_status.bits.connect_status_change << i;
            ++i;
        }
        return csc;
    }

    const char* transfer_types[] = {"Ctrl", "Isoc", "Bulk", "Intr"};
    const char* usage_type_interrupt[] = {"Periodic", "Notification", "Reserved", "Reserved"};
    const char* sync_type_isochronous[] = {"No-Sync", "Async", "Adaptive", "Sync"};
    const char* usage_type_isochronous[] = {"Data", "Feedback", "Implicit", "Reserved"};

    xhci::transferring::Manager* ConfigureEndpoint(
        xhci::Controller* xhc,
        uint8_t slot_id,
        uint8_t ep_index, bool dir_in)
    {
        alignas(64) xhci::InputContext input_context{};
        //input_context.EnableSlotContext();
        input_context.EnableSlotContext();
        auto& dc = *xhc->DeviceContextAddresses()[slot_id];
        auto& sc = input_context.GetSlotContext();
        memcpy(&sc, &dc.slot_context, sizeof(xhci::SlotContext));

        sc.bits.context_entries = 2 * ep_index + dir_in;

        input_context.EnableEndpoint(ep_index, dir_in);
        auto& icc = input_context.input_control_context;
        //icc.configuration_value = 1;
        //icc.interface_number = 0;
        //icc.alternate_setting = 0;
        printk("add context flags: %08x, calculated dci = %u\n",
            icc.add_context_flags, 2 * ep_index + dir_in);

        auto& ep = input_context.GetEndpointContext(ep_index, dir_in);

        const auto dci = 2 * ep_index + dir_in;
        auto tr_mgr = AllocateTRManager(dci, xhc->DoorbellRegisters(), slot_id);

        ep.bits.ep_type = 7; // Interrupt In
        ep.bits.max_packet_size = 8;
        ep.bits.max_burst_size = 0;
        ep.bits.tr_dequeue_pointer = reinterpret_cast<uint64_t>(tr_mgr->Data()) >> 4;
        ep.bits.dequeue_cycle_state = 1;
        ep.bits.interval = 0;
        ep.bits.max_primary_streams = 0;
        ep.bits.mult = 0;
        ep.bits.error_count = 3;
        ep.bits.average_trb_length = 1;

        xhci::ConfigureEndpointCommandTRB conf_ep{&input_context, slot_id};
        xhc->CommandRingManager().Push(conf_ep);

        auto& er_mgr = xhc->EventRingManager();
        while (!er_mgr.HasFront());

        while (er_mgr.HasFront())
        {
            auto trb = er_mgr.Front();
            er_mgr.Pop();
            ShowEventTRB(trb);
        }

        return tr_mgr;
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
                break;
            }
        }

        if (xhci_dev_index == num_pci_devices)
        {
            printk("no xHCI device\n");
            return;
        }

        const auto& dev_param = pci_devices[xhci_dev_index];
        printk("xHCI device: index = %lu, addr = %02x:%02x.%02x\n",
            xhci_dev_index, dev_param.bus, dev_param.dev, dev_param.func);

        pci::NormalDevice xhci_dev(dev_param.bus, dev_param.dev, dev_param.func);

        const auto bar = pci::ReadBar(xhci_dev, 0);
        const auto mmio_base = bitutil::ClearBits(bar.value, 0xf);
        printk("xhci mmio_base = %08lx\n", mmio_base);
        xhc = new usb::xhci::RealController{mmio_base};

        if (auto err = xhc->Initialize(); err != usb::error::kSuccess)
        {
            delete xhc;
            printk("failed to initialize xHCI controller: %d\n", err);
            return;
        }

        // Configure MSI
        const uint8_t bsp_local_apic_id =
            *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;

        const uint32_t msi_msg_addr = 0xfee00000u | (bsp_local_apic_id << 12);
        const uint32_t msi_msg_data = 0xc000u | 0x40u;
        if (auto err = pci::ConfigureMSI(xhci_dev, msi_msg_addr, msi_msg_data, 0);
            err != errorcode::kSuccess)
        {
            printk("failed to configure xHCI MSI capability: %d\n", err);
            return;
        }

        // Run the controller
        if (auto err = xhc->Run(); err != usb::error::kSuccess)
        {
            delete xhc;
            printk("failed to run xHCI controller: %d\n", err);
            return;
        }
        return;

#if 0
        xhc = new(xhc_buf) xhci::Controller{mmio_base};
        auto& cap_reg = xhc->CapabilityRegisters();
        auto& op_reg = xhc->OperationalRegisters();
        auto& cr_mgr = xhc->CommandRingManager();
        auto& er_mgr = xhc->EventRingManager();

        const auto max_ports = xhci::MaxPorts(*xhc);
        /*
        for (uint8_t port = 0; port < max_ports; ++port)
        {
            auto& port_reg_set = xhc.PortRegSets()[port];
            const bool current_connect_status = port_reg_set.PORTSC & 1;
            if (current_connect_status)
            {
                printk("Port %u is connected\n");
            }
        }
        */

        xhc->Initialize();

        auto ia32_apic_base = msr::Read(msr::IA32_APIC_BASE);
        printk("IA32_APIC_BASE %08x (APIC is %s)\n", ia32_apic_base,
            (ia32_apic_base & (1u << 11)) ? "enabled" : "disabled");

        const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;

        uint64_t cap_addr = xhci_dev.ReadCapabilityPointer();
        auto cap = xhci_dev.ReadCapabilityStructure(cap_addr);
        while (true)
        {
            if (cap.cap_id == 0x11)
            {
                auto msix_cap = pci::ReadMSIXCapabilityStructure(xhci_dev, cap_addr);
                auto header = msix_cap.header;
                auto table = msix_cap.table;
                auto pba = msix_cap.pba;

                const auto table_bar = pci::ReadBar(xhci_dev, table.bits.bir);
                const auto pba_bar = pci::ReadBar(xhci_dev, pba.bits.bir);
                const auto table_addr = table_bar.value + table.bits.offset << 3;
                const auto pba_addr = pba_bar.value + pba.bits.offset << 3;

                printk("MSI-X Conf Cap:"
                    " table size %u, func mask %u, msix enable %u",
                    ", table %u %08x, pba %u %08x\n",
                    header.bits.table_size,
                    header.bits.function_mask,
                    header.bits.msix_enable,
                    table.bits.bir, table_addr,
                    pba.bits.bir, pba_addr);

                auto* msix_table = reinterpret_cast<pci::MSIXTableEntry*>(table_addr);

                for (size_t i = 0; i <= header.bits.table_size; ++i)
                {
                    printk(" Table[%lu]: MsgAddr %08x, Upper %08x"
                        ", MsgData %08x, VectCtrl %08x\n",
                        i,
                        msix_table[i].msg_addr.Read(),
                        msix_table[i].msg_upper_addr.Read(),
                        msix_table[i].msg_data.Read(),
                        msix_table[i].vector_control.Read());
                }
            }
            else if (cap.cap_id == 0x05)
            {
                auto msi_cap = pci::ReadMSICapabilityStructure(xhci_dev, cap_addr);

                printk("MSI Conf Cap:"
                    " msi enable %u, multi msg cap %u, enable %u, addr64 %u, pvmask %u"
                    ", addr %08x, upper %08x, data %08x, mask %08x, pending %08x\n",
                    msi_cap.header.bits.msi_enable,
                    msi_cap.header.bits.multi_msg_capable,
                    msi_cap.header.bits.multi_msg_enable,
                    msi_cap.header.bits.addr_64_capable,
                    msi_cap.header.bits.per_vector_mask_capable,
                    msi_cap.msg_addr,
                    msi_cap.msg_upper_addr,
                    msi_cap.msg_data,
                    msi_cap.mask_bits,
                    msi_cap.pending_bits);

                msi_cap.header.bits.msi_enable = 1;
                msi_cap.msg_addr = 0xfee00000u | (bsp_local_apic_id << 12);
                msi_cap.msg_data = 0xc000u | 0x40u;

                pci::WriteMSICapabilityStructure(xhci_dev, cap_addr, msi_cap);

                msi_cap = pci::ReadMSICapabilityStructure(xhci_dev, cap_addr);

                printk("MSI Conf Cap:"
                    " msi enable %u, multi msg cap %u, enable %u, addr64 %u, pvmask %u"
                    ", addr %08x, upper %08x, data %08x, mask %08x, pending %08x\n",
                    msi_cap.header.bits.msi_enable,
                    msi_cap.header.bits.multi_msg_capable,
                    msi_cap.header.bits.multi_msg_enable,
                    msi_cap.header.bits.addr_64_capable,
                    msi_cap.header.bits.per_vector_mask_capable,
                    msi_cap.msg_addr,
                    msi_cap.msg_upper_addr,
                    msi_cap.msg_data,
                    msi_cap.mask_bits,
                    msi_cap.pending_bits);
            }
            else
            {
                printk("Capability %u\n", cap.cap_id);
            }

            if (cap.next_ptr == 0)
            {
                break;
            }

            cap_addr = cap.next_ptr;
            cap = xhci_dev.ReadCapabilityStructure(cap_addr);
        }

        //xhci::NoOpCommandTRB no_op_cmd{};

        //printk("pussing No Op Command to command ring\n");
        //cr_mgr.Push(no_op_cmd);
        //printk("Command Ring write index: %lu\n", cr_mgr.write_index());
        //printk("CRR, CA, CS, RCS = %u\n", xhc->OperationalRegisters().CRCR.Read() & 0xfu);

        // wait for Port Status Change Event
        while (!er_mgr.HasFront());

        while (er_mgr.HasFront())
        {
            auto trb = er_mgr.Front();
            er_mgr.Pop();
            ShowEventTRB(trb);
        }

        //printk("CAPLENGTH=%02x HCIVERSION=%04x"
        //    " DBOFF=%08x RTSOFF=%08x"
        //    " HCSPARAMS1=%08x HCCPARAMS1=%08x\n",
        //    cap_reg.ReadCAPLENGTH(), cap_reg.ReadHCIVERSION(),
        //    cap_reg.DBOFF.Read(), cap_reg.RTSOFF.Read(),
        //    cap_reg.HCSPARAMS1.Read(), cap_reg.HCCPARAMS1.Read());

        //printk("USBCMD=%08x USBSTS=%08x DCBAAP=%016lx CONFIG=%08x\n",
        //    op_reg.USBCMD.Read(), op_reg.USBSTS.Read(),
        //    op_reg.DCBAAP.Read(), op_reg.CONFIG.Read());

        for (size_t i = 0; i < 100000; ++i)
        {
            const auto csc = GetCSCBits(xhc->PortRegSets());
            if (csc == 0) continue;

            printk("CSC: %llu (i = %lu)\n", csc, i);
            size_t port_index = 0;
            for (auto& port_reg : xhc->PortRegSets())
            {
                if ((csc >> port_index) & 1)
                {
                    //ShowPORTSC(port_reg);

                    auto portsc = port_reg.PORTSC.Read();
                    // clear following bits:
                    //  WPR, DR, CAS, CEC, PLC, PRC, OCC, WRC, PEC, CSC, LWS,
                    //  Port Speed, PR, OCA, PED, CCS
                    // Read-Only: CCS, OCA, Port Speed, CAS, DR
                    // RW1CS: PED, CSC, PEC, WRC, OCC, PRC, PLC, CEC
                    // RW1S: PR, WPR
                    // RW: LWS
                    portsc.data &= 0x0e00c3e0u;
                    portsc.data |= 0x00020010u; // Write 1 to PR and CSC
                    port_reg.PORTSC.Write(portsc);

                    //ShowPORTSC(port_reg);

                    printk("Initializing port index %lu\n", port_index);
                    InitializePort(xhc, port_index);
                }
                ++port_index;
            }
        }

        //uint64_t rtsbase = mmio_base + cap_reg.RTSOFF.Read();
        //uint32_t* ir0 = reinterpret_cast<uint32_t*>(rtsbase + 0x20u);
        //printk("ERSTSZ=%u ERSTBA=%016lx ERDP=%016lx\n",
        //    ir0[2],
        //    ir0[4] | (static_cast<uint64_t>(ir0[5]) << 32),
        //    ir0[6] | (static_cast<uint64_t>(ir0[7]) << 32));

        /*
        if (er_mgr.HasFront())
        {
            printk("ER has at least one element (2)\n");
        }
        else
        {
            printk("ER has no element (2)\n");
        }
        */

        for (unsigned int slot_id = 1; slot_id <= 255; ++slot_id)
        {
            auto dcaddr = xhc->DeviceContextAddresses()[slot_id];
            if (dcaddr == nullptr)
            {
                continue;
            }

            const auto& sc = dcaddr->slot_context;
            printk("DevCtx %u: Hub=%d, #EP=%d, slot state %02x, usb dev addr %02x, route string",
                slot_id, sc.bits.hub, sc.bits.context_entries,
                sc.bits.slot_state, sc.bits.usb_device_address);

            uint32_t route_string = sc.bits.route_string;
            printk(" Route String:");
            for (int i = 0; i < 5; ++i)
            {
                printk(" %x", (route_string >> 16) & 0xfu);
                route_string <<= 4;
            }
            printk("\n");

            uint8_t buf[128];
            GetDescriptor(xhc, slot_id, 1, 0, sizeof(buf), buf);
            const auto dev_class = buf[4];
            const auto dev_subclass = buf[5];
            const auto num_configs = buf[17];
            printk("DEVICE: class = %u, subclass = %u, num configs = %u\n",
                dev_class, dev_subclass, num_configs);

            int configuration_value = -1;
            bool is_keyboard = false;
            for (size_t conf_index = 0; conf_index < num_configs; ++conf_index)
            {
                memset(buf, 0, sizeof(buf));
                GetDescriptor(xhc, slot_id, 2, conf_index, sizeof(buf), buf);
                const auto num_interfaces = buf[4];
                printk("CONFIGURATION[%lu]: Length = %u, DescriptorType = %u, Value = %u, NumInterfaces = %u\n",
                    conf_index, buf[0], buf[1], buf[5], num_interfaces);
                if (configuration_value == -1)
                {
                    configuration_value = buf[5];
                }

                uint8_t* p = buf + buf[0];
                //for (size_t interface = 0; interface < num_interfaces; ++interface)
                while (p[0])
                {
                    const char* desc_type_name = "UnknownType";
                    if (p[1] == 4) desc_type_name = "INTERFACE";
                    if (p[1] == 5) desc_type_name = "ENDPOINT";
                    if (p[1] == 33) desc_type_name = "HID";
                    if (p[1] == 34) desc_type_name = "REPORT";
                    printk("  Length = %u, DesciptorType = %u (%s)\n",
                        p[0], p[1], desc_type_name);
                    if (p[1] == 4) // INTERFACE
                    {
                        printk("    Class = %u, SubClass = %u, Protocol = %u, "
                            "INum = %u, AltSet = %u",
                            p[5], p[6], p[7], p[2], p[3]);
                        if (p[5] == 3) // HID class
                        {
                            if (p[6] == 1)
                            {
                                const char* protocol_name = "Reserved";
                                if (p[7] == 0) protocol_name = "None";
                                if (p[7] == 1) protocol_name = "Keyboard";
                                if (p[7] == 2) protocol_name = "Mouse";
                                printk(", HID BootInterface %s\n", protocol_name);
                                if (p[7] == 1)
                                {
                                    is_keyboard = true;
                                }
                            }
                            else
                            {
                                printk(", HID NotBootInterface\n");
                            }
                        }
                        else
                        {
                            printk(", Unknown Class\n");
                        }
                    }
                    else if (p[1] == 5)
                    {
                        const auto ep_num = p[2] & 0xfu;
                        const char* dir = p[2] & 0x80u ? "IN" : "OUT";

                        const char* transfer_type = transfer_types[p[3] & 0x3u];
                        const char* sync_type = "Reserved";
                        const char* usage_type = "Reserved";
                        switch (p[3] & 0x3u)
                        {
                        case 1:
                            sync_type = sync_type_isochronous[(p[3] >> 2) & 0x3u];
                            usage_type = usage_type_isochronous[(p[3] >> 4) & 0x3u];
                            break;
                        case 3:
                            usage_type = usage_type_interrupt[(p[3] >> 4) & 0x3u];
                            break;
                        }
                        printk("    EP Num %u (%s), Attr %s %s %s, "
                            "Max Packet Size %u, Interval %u\n",
                            ep_num, dir, transfer_type, sync_type,usage_type,
                            static_cast<uint16_t>(p[5]) << 8 | p[4], p[6]);
                    }
                    p += p[0];
                }

            }

            if (!is_keyboard)
            {
                continue;
            }

            printk("Current Configuration %u\n", GetConfiguration(xhc, slot_id));
            printk("Setting configuration: slot %lu, value %u\n",
                slot_id, configuration_value);
            SetConfiguration(xhc, slot_id, configuration_value);
            printk("Current Configuration %u\n", GetConfiguration(xhc, slot_id));
            auto tr_mgr = ConfigureEndpoint(xhc, slot_id, 1, true);
            tr_mgr_for_keyboard = tr_mgr;
            printk("TR Manager for keyboard %08x\n", tr_mgr_for_keyboard);

            printk("Pussing NoOp\n");
            xhci::NoOpTRB noop{};
            noop.bits.interrupt_on_completion = 1;
            tr_mgr->Push(noop);

            while (!xhc->EventRingManager().HasFront());

            while (xhc->EventRingManager().HasFront())
            {
                auto trb = xhc->EventRingManager().Front();
                xhc->EventRingManager().Pop();
                ShowEventTRB(trb);
            }

            auto& int_reg_set = xhc->InterrupterRegSets()[0];

            //auto imod = int_reg_set.IMOD.Read();
            //imod.data = 4000;
            //int_reg_set.IMOD.Write(imod);

            auto iman = int_reg_set.IMAN.Read();
            iman.data |= 3u;
            printk("Setting IMAN %u\n", iman.data);
            int_reg_set.IMAN.Write(iman);

            /*
            while (true)
            {
                memset(buf, 0, sizeof(buf));
                xhci::NormalTRB normal{};
                normal.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(buf);
                normal.bits.trb_transfer_length = 8;
                normal.bits.interrupt_on_completion = 1;
                tr_mgr->Push(normal);

                while (!xhc->EventRingManager().HasFront());

                while (xhc->EventRingManager().HasFront())
                {
                    auto trb = xhc->EventRingManager().Front();
                    xhc->EventRingManager().Pop();
                }

                for (size_t i = 2; i < 8; ++i)
                {
                    if (buf[i] == 0) continue;

                    char ascii = keycode_map[buf[i]];
                    if (ascii)
                    {
                        printk("%c", ascii);
                    }
                }
            }
            */

            printk("DevCtx %u: Hub=%d, #EP=%d, slot state %02x, usb dev addr %02x, route string\n",
                slot_id, sc.bits.hub, sc.bits.context_entries,
                sc.bits.slot_state, sc.bits.usb_device_address);
        }

#endif
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
//}

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
