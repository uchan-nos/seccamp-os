#include "xhci_trb.hpp"

namespace bitnos::xhci
{
    const char* trb_completion_code_names[37] = {
        "Invalid",
        "Success",
        "Data Buffer Error",
        "Babble Detected Error",
        "USB Transaction Error",
        "TRB Error",
        "Stall Error",
        "Resource Error",
        "Bandwidth Error",
        "No Slots Available Error",
        "Invalid Stream Type Error",
        "Slot Not Enabled Error",
        "Endpoint Not Enabled Error",
        "Short Packet",
        "Ring Underrun",
        "Ring Overrun",
        "VF Event Ring Full Error",
        "Parameter Error",
        "Bandwidth Overrun Error",
        "Context State Error",
        "No ping Response Error",
        "Event Ring Full Error",
        "Incompatible Device Error",
        "Missed Service Error",
        "Command Ring Stopped",
        "Command Aborted",
        "Stopped",
        "Stopped - Length Invalid",
        "Stopped - Short Packet",
        "Max Exit Latency Too Large Error",
        "Reserved",
        "Isoch Buffer Overrun",
        "Event Lost Error",
        "Undefined Error",
        "Invalid Stream ID Error",
        "Secondary Bandwidth Error",
        "Split Transaction Error",
    };

    const char* trb_type_names[64] = {
        "Reserved",                             // 0
        "Normal",
        "Setup Stage",
        "Data Stage",
        "Status Stage",
        "Isoch",
        "Link",
        "EventData",
        "No-Op",                                // 8
        "Enable Slot Command",
        "Disable Slot Command",
        "Address Device Command",
        "Configure Endpoint Command",
        "Evaluate Context Command",
        "Reset Endpoint Command",
        "Stop Endpoint Command",
        "Set TR Dequeue Pointer Command",       // 16
        "Reset Device Command",
        "Force Event Command",
        "Negotiate Bandwidth Command",
        "Set Latency Tolerance Value Command",
        "Get Port Bandwidth Command",
        "Force Header Command",
        "No Op Command",
        "Reserved",                             // 24
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Transfer Event",                       // 32
        "Command Completion Event",
        "Port Status Change Event",
        "Bandwidth Request Event",
        "Doorbell Event",
        "Host Controller Event",
        "Device Notification Event",
        "MFINDEX Wrap Event",
        "Reserved",                             // 40
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Vendor Defined",                       // 48
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",                       // 56
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
        "Vendor Defined",
    };

    EnableSlotCommandTRB::EnableSlotCommandTRB()
        : dwords{}
    {
        bits.trb_type = 9;
    }

    AddressDeviceCommandTRB::AddressDeviceCommandTRB(
        const InputContext* input_context, uint8_t slot_id)
        : dwords{}
    {
        bits.trb_type = 11;

        bits.input_context_pointer =
            reinterpret_cast<uint64_t>(input_context) >> 4;
        bits.slot_id = slot_id;
    }

    ConfigureEndpointCommandTRB::ConfigureEndpointCommandTRB(
        const InputContext* input_context, uint8_t slot_id)
        : dwords{}
    {
        bits.trb_type = 12;

        bits.input_context_pointer =
            reinterpret_cast<uint64_t>(input_context) >> 4;
        bits.slot_id = slot_id;
    }

    StopEndpointCommandTRB::StopEndpointCommandTRB(
        uint8_t endpoint_id, uint8_t slot_id)
        : dwords{}
    {
        bits.trb_type = 15;
        bits.endpoint_id = endpoint_id;
        bits.slot_id = slot_id;
    }

    NoOpCommandTRB::NoOpCommandTRB()
        : dwords{}
    {
        bits.trb_type = 23;
    }

    NormalTRB::NormalTRB()
        : dwords{}
    {
        bits.trb_type = 1;
    }

    SetupStageTRB::SetupStageTRB()
        : dwords{}
    {
        bits.trb_type = 2;
        bits.immediate_data = 1;
        bits.trb_transfer_length = 8;
    }

    DataStageTRB::DataStageTRB()
        : dwords{}
    {
        bits.trb_type = 3;
    }

    StatusStageTRB::StatusStageTRB()
        : dwords{}
    {
        bits.trb_type = 4;
    }

    LinkTRB::LinkTRB()
        : dwords{}
    {
        bits.trb_type = 6;
    }

    NoOpTRB::NoOpTRB()
        : dwords{}
    {
        bits.trb_type = 8;
    }
}
