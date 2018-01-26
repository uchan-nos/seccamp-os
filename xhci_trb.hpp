#pragma once

#include <stdint.h>

#include "xhci_ctx.hpp"

namespace bitnos::xhci
{
    extern const char* trb_completion_code_names[37];
    extern const char* trb_type_names[64];

    union TRB
    {
        uint32_t dwords[4];
        struct
        {
            uint64_t parameter : 64;
            uint32_t status : 32;
            uint32_t cycle_bit : 1;
            uint32_t evaluate_next_trb : 1;
            uint32_t : 8;
            uint32_t trb_type : 6;
            uint32_t : 16;
        } bits;
    };

    union EnableSlotCommandTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint32_t : 32;

            uint32_t : 32;

            uint32_t : 32;

            uint32_t cycle_bit : 1;
            uint32_t : 9;
            uint32_t trb_type : 6;
            uint32_t slot_type : 5;
            uint32_t : 11;
        } bits;

        EnableSlotCommandTRB();
    };

    union AddressDeviceCommandTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint32_t : 4;
            uint64_t input_context_pointer : 60;

            uint32_t : 32;

            uint32_t cycle_bit : 1;
            uint32_t : 8;
            uint32_t deconfigure : 1;
            uint32_t trb_type : 6;
            uint32_t : 8;
            uint32_t slot_id : 8;
        } bits;

        AddressDeviceCommandTRB(
            const InputContext* input_context, uint8_t slot_id);
    };

    union ConfigureEndpointCommandTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint32_t : 4;
            uint64_t input_context_pointer : 60;

            uint32_t : 32;

            uint32_t cycle_bit : 1;
            uint32_t : 8;
            uint32_t deconfigure : 1;
            uint32_t trb_type : 6;
            uint32_t : 8;
            uint32_t slot_id : 8;
        } bits;

        ConfigureEndpointCommandTRB(
            const InputContext* input_context, uint8_t slot_id);
    };

    union StopEndpointCommandTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint32_t : 32;

            uint32_t : 32;

            uint32_t : 32;

            uint32_t cycle_bit : 1;
            uint32_t : 9;
            uint32_t trb_type : 6;
            uint32_t endpoint_id : 5;
            uint32_t : 2;
            uint32_t suspend : 1;
            uint32_t slot_id : 8;
        } bits;

        StopEndpointCommandTRB(uint8_t endpoint_id, uint8_t slot_id);
    };

    union NoOpCommandTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint32_t : 32;

            uint32_t : 32;

            uint32_t : 32;

            uint32_t cycle_bit : 1;
            uint32_t : 9;
            uint32_t trb_type : 6;
            uint32_t : 16;
        } bits;

        NoOpCommandTRB();
    };

    union TransferEventTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint64_t trb_pointer : 64;

            uint32_t trb_transfer_length : 24;
            uint32_t completion_code : 8;

            uint32_t cycle_bit : 1;
            uint32_t : 1;
            uint32_t event_data : 1;
            uint32_t : 7;
            uint32_t trb_type : 6;
            uint32_t endpoint_id : 5;
            uint32_t : 3;
            uint32_t slot_id : 8;
        } bits;
    };

    union CommandCompletionEventTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint32_t : 4;
            uint64_t command_trb_pointer : 60;

            uint32_t command_completion_parameter : 24;
            uint32_t completion_code : 8;

            uint32_t cycle_bit : 1;
            uint32_t : 9;
            uint32_t trb_type : 6;
            uint32_t vf_id : 8;
            uint32_t slot_id : 8;
        } bits;
    };

    union NormalTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint64_t data_buffer_pointer;

            uint32_t trb_transfer_length : 17;
            uint32_t td_size : 5;
            uint32_t interrupter_target : 10;

            uint32_t cycle_bit : 1;
            uint32_t evaluate_next_trb : 1;
            uint32_t interrupt_on_short_packet : 1;
            uint32_t no_snoop : 1;
            uint32_t chain_bit : 1;
            uint32_t interrupt_on_completion : 1;
            uint32_t immediate_data : 1;
            uint32_t : 2;
            uint32_t block_event_interrupt : 1;
            uint32_t trb_type : 6;
            uint32_t : 16;
        } bits;

        NormalTRB();
    };

    union SetupStageTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint32_t request_type : 8;
            uint32_t request : 8;
            uint32_t value : 16;

            uint32_t index : 16;
            uint32_t length : 16;

            uint32_t trb_transfer_length : 17;
            uint32_t : 5;
            uint32_t interrupter_target : 10;

            uint32_t cycle_bit : 1;
            uint32_t : 4;
            uint32_t interrupt_on_completion : 1;
            uint32_t immediate_data : 1;
            uint32_t : 3;
            uint32_t trb_type : 6;
            uint32_t transfer_type : 2;
            uint32_t slot_id : 14;
        } bits;

        SetupStageTRB();
    };

    union DataStageTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint64_t data_buffer_pointer;

            uint32_t trb_transfer_length : 17;
            uint32_t td_size : 5;
            uint32_t interrupter_target : 10;

            uint32_t cycle_bit : 1;
            uint32_t evaluate_next_trb : 1;
            uint32_t interrupt_on_short_packet : 1;
            uint32_t no_snoop : 1;
            uint32_t chain_bit : 1;
            uint32_t interrupt_on_completion : 1;
            uint32_t immediate_data : 1;
            uint32_t : 3;
            uint32_t trb_type : 6;
            uint32_t direction : 1;
            uint32_t : 15;
        } bits;

        DataStageTRB();
    };

    union StatusStageTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint64_t : 64;

            uint32_t : 22;
            uint32_t interrupter_target : 10;

            uint32_t cycle_bit : 1;
            uint32_t evaluate_next_trb : 1;
            uint32_t : 2;
            uint32_t chain_bit : 1;
            uint32_t interrupt_on_completion : 1;
            uint32_t : 4;
            uint32_t trb_type : 6;
            uint32_t direction : 1;
            uint32_t : 15;
        } bits;

        StatusStageTRB();
    };

    union LinkTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint64_t : 4;
            uint64_t ring_segment_pointer : 60;

            uint32_t : 22;
            uint32_t interrupter_target : 10;

            uint32_t cycle_bit : 1;
            uint32_t toggle_cycle : 1;
            uint32_t : 2;
            uint32_t chain_bit : 1;
            uint32_t interrupt_on_completion : 1;
            uint32_t : 4;
            uint32_t trb_type : 6;
            uint32_t : 16;
        } bits;

        LinkTRB();
    };

    union NoOpTRB
    {
        uint32_t dwords[4];
        struct
        {
            uint64_t : 64;

            uint32_t : 22;
            uint32_t interrupter_target : 10;

            uint32_t cycle_bit : 1;
            uint32_t evaluate_next_trb : 1;
            uint32_t : 2;
            uint32_t chain_bit : 1;
            uint32_t interrupt_on_completion : 1;
            uint32_t : 4;
            uint32_t trb_type : 6;
            uint32_t : 16;
        } bits;

        NoOpTRB();
    };
}
