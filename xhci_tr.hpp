#pragma once

#include <stddef.h>

#include "xhci_regs.hpp"
#include "printk.hpp"

namespace bitnos::xhci::transferring
{
    class Manager
    {
        static const size_t kDataSize = 8;
        bool cycle_bit_;
        size_t write_index_;
        size_t dci_;
        DoorbellRegisterArray db_regs_;
        uint8_t slot_id_;
        alignas(64) TRB data_[kDataSize];

    public:
        Manager(size_t dci, DoorbellRegisterArray db_regs, uint8_t slot_id)
            : cycle_bit_(1), write_index_(0), dci_(dci),
              db_regs_(db_regs), slot_id_(slot_id), data_{}
        {}

        template <typename TRBType>
        void Push(const TRBType& trb, bool ring = true)
        {
            for (int i = 0; i < 3; ++i)
            {
                // dwords[0..2] must be written prior to dwords[3].
                data_[write_index_].dwords[i] = trb.dwords[i];
            }
            data_[write_index_].dwords[3]
                = (trb.dwords[3] & 0xfffffffeu) | static_cast<uint32_t>(cycle_bit_);

            if (ring)
            {
                db_regs_[slot_id_].DB.Write(dci_);
            }

            ++write_index_;
            if (write_index_ == kDataSize - 1)
            {
                // TODO: place link TRB
                write_index_ = 0;
                cycle_bit_ = !cycle_bit_;
            }
        }

        TRB* Data() { return data_; }
        const TRB* Data() const { return data_; }

        TRB* Debug() { return data_ + write_index_; }
    };
}
