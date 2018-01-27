#pragma once

/**
 * Transfer Request Block definitions.
 */

#include <stdint.h>
#include "usb/xhci/context.hpp"

namespace usb::xhci
{
  union TRB
  {
    uint32_t data[4];
    struct
    {
      uint64_t parameter;
      uint32_t status;
      uint32_t cycle_bit : 1;
      uint32_t evaluate_next_trb : 1;
      uint32_t : 8;
      uint32_t trb_type : 6;
      uint32_t control : 16;
    } __attribute__((packed)) bits;
  };

  union NormalTRB
  {
    static const unsigned int Type = 1;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    NormalTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union SetupStageTRB
  {
    static const unsigned int Type = 2;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    SetupStageTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union DataStageTRB
  {
    static const unsigned int Type = 3;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    DataStageTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union StatusStageTRB
  {
    static const unsigned int Type = 4;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    StatusStageTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union LinkTRB
  {
    static const unsigned int Type = 6;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    uint64_t Pointer() const
    {
      return bits.ring_segment_pointer << 4;
    }

    void SetPointer(uint64_t value)
    {
      bits.ring_segment_pointer = value >> 4;
    }

    LinkTRB(void* ring_segment_pointer)
      : data{}
    {
      bits.trb_type = Type;
      SetPointer(reinterpret_cast<uint64_t>(ring_segment_pointer));
    }
  };

  union NoOpTRB
  {
    static const unsigned int Type = 8;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    NoOpTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union EnableSlotCommandTRB
  {
    static const unsigned int Type = 9;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    EnableSlotCommandTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union AddressDeviceCommandTRB
  {
    static const unsigned int Type = 11;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    uint64_t Pointer() const
    {
      return bits.input_context_pointer << 4;
    }

    void SetPointer(uint64_t value)
    {
      bits.input_context_pointer = value >> 4;
    }

    AddressDeviceCommandTRB(const InputContext* input_context, uint8_t slot_id)
      : data{}
    {
      bits.trb_type = Type;
      bits.slot_id = slot_id;
      SetPointer(reinterpret_cast<uint64_t>(input_context));
    }
  };

  union ConfigureEndpointCommandTRB
  {
    static const unsigned int Type = 12;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    uint64_t Pointer() const
    {
      return bits.input_context_pointer << 4;
    }

    void SetPointer(uint64_t value)
    {
      bits.input_context_pointer = value >> 4;
    }

    ConfigureEndpointCommandTRB(const InputContext* input_context, uint8_t slot_id)
      : data{}
    {
      bits.trb_type = Type;
      bits.slot_id = slot_id;
      SetPointer(reinterpret_cast<uint64_t>(input_context));
    }
  };

  union StopEndpointCommandTRB
  {
    static const unsigned int Type = 15;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    StopEndpointCommandTRB(DeviceContextIndex endpoint_id, uint8_t slot_id)
      : data{}
    {
      bits.trb_type = Type;
      bits.endpoint_id = endpoint_id.value;
      bits.slot_id = slot_id;
    }
  };

  union NoOpCommandTRB
  {
    static const unsigned int Type = 23;
    uint32_t data[4];
    struct
    {
      uint32_t : 32;

      uint32_t : 32;

      uint32_t : 32;

      uint32_t cycle_bit : 1;
      uint32_t : 9;
      uint32_t trb_type : 6;
      uint32_t : 16;
    } __attribute__((packed)) bits;

    NoOpCommandTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union TransferEventTRB
  {
    static const unsigned int Type = 32;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    TransferEventTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };

  union CommandCompletionEventTRB
  {
    static const unsigned int Type = 33;
    uint32_t data[4];
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
    } __attribute__((packed)) bits;

    CommandCompletionEventTRB()
      : data{}
    {
      bits.trb_type = Type;
    }
  };
}
