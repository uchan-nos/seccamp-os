#pragma once

#include <stddef.h>

#include "xhci_regs.hpp"
#include "xhci_trb.hpp"

namespace bitnos::xhci::commandring
{
    class Manager
    {
        static const size_t kDataSize = 8;
        bool cr_cycle_bit_ = 1;
        size_t write_index_ = 0;
        OperationalRegisters& op_regs_;
        DoorbellRegisterArray db_regs_;
        alignas(64) TRB data_[kDataSize];

    public:
        Manager(OperationalRegisters& op_regs, DoorbellRegisterArray db_regs)
            : op_regs_{op_regs}, db_regs_{db_regs}, data_{}
        {}

        void Initialize()
        {
            cr_cycle_bit_ = 1;
            write_index_ = 0;
            op_regs_.CRCR.Write(reinterpret_cast<uint64_t>(&data_[0]) | cr_cycle_bit_);
        }

        template <typename TRBType>
        void Push(const TRBType& command_trb)
        {
            for (int i = 0; i < 3; ++i)
            {
                // dwords[0..2] must be written prior to dwords[3].
                data_[write_index_].dwords[i] = command_trb.dwords[i];
            }
            data_[write_index_].dwords[3]
                = (command_trb.dwords[3] & 0xfffffffeu) | static_cast<uint32_t>(cr_cycle_bit_);

            db_regs_[0].DB.Write(0);

            ++write_index_;
            if (write_index_ == kDataSize - 1)
            {
                // TODO: place link TRB
                write_index_ = 0;
                cr_cycle_bit_ = !cr_cycle_bit_;
            }
        }

        auto write_index() const
        {
            return write_index_;
        }
    };
}
