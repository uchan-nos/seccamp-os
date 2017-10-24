#pragma once

#include <stdint.h>

#include "xhci_ctx.hpp"

namespace bitnos::xhci
{
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
            const InputContext& input_context, bool cycle_bit, uint8_t slot_id);
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
}
