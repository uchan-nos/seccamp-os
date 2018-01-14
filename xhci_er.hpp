#pragma once

#include <stddef.h>
#include <string.h>

#include "xhci_trb.hpp"

#include "printk.hpp"

namespace bitnos::xhci::eventring
{
    class SegmentTableEntry
    {
    public:
        using ValueType = TRB;
        using Iterator = ValueType*;
        using ConstIterator = const ValueType*;

        void SetAddressAndSize(TRB* segment_base, uint16_t segment_size)
        {
            segment_base_address_ = reinterpret_cast<uint64_t>(segment_base);
            segment_size_ = segment_size;
        }

        size_t Size() const { return segment_size_; }

        Iterator begin()
        {
            return reinterpret_cast<Iterator>(segment_base_address_);
        }
        Iterator end() { return begin() + segment_size_; }
        ConstIterator cbegin() const
        {
            return reinterpret_cast<ConstIterator>(segment_base_address_);
        }
        ConstIterator cend() const { return cbegin() + segment_size_; }

        ValueType& operator [](size_t index) { return begin()[index]; }

    public:
        uint64_t segment_base_address_;
        uint16_t segment_size_;
        uint16_t reserved1_;
        uint32_t reserved2_;
    };

    class Manager
    {
        static const size_t kNumTables = 1; // the number of tables
        static const size_t kTableSize = 16; // the number of elements in a table

        unsigned char producer_cycle_;
        InterrupterRegSet& int_reg_set_;
        size_t erst_index_;
        alignas(64) SegmentTableEntry erst_[kNumTables];
        alignas(64) TRB data_[kNumTables][kTableSize];

        TRB* ReadDequeuePointer()
        {
            return reinterpret_cast<TRB*>(
                bitutil::ClearBits(int_reg_set_.ERDP.Read().data, 0xfu));
        }

        void WriteDequeuePointer(TRB* p)
        {
            int_reg_set_.ERDP.Write(reinterpret_cast<uint64_t>(p));
        }

    public:
        Manager(InterrupterRegSet& int_reg_set)
            : int_reg_set_(int_reg_set)
        {}

        void Initialize()
        {
            producer_cycle_ = 1;
            erst_index_ = 0;

            memset(data_, 0, sizeof(data_));
            for (size_t i = 0; i < kNumTables; ++i)
            {
                erst_[i].SetAddressAndSize(data_[i], kTableSize);
            }

            int_reg_set_.ERSTSZ.Write(kNumTables);

            WriteDequeuePointer(data_[0]);

            // reload base address to reset the state of the ring
            int_reg_set_.ERSTBA.Write(reinterpret_cast<uint64_t>(erst_));
            // 仕様書にはこうある
            // Write the ERST Base Address (ERSTBA) register with the value of ERST(0).BaseAddress.
            // つまり erst_[0].segment_base_address_ を ERSTBA に書き込めと言っている．
            // しかし，これは間違いだと思う…
        }

        bool HasFront()
        {
            return Front().bits.cycle_bit == producer_cycle_;
        }

        TRB& Front()
        {
            return *ReadDequeuePointer();
        }

        void Pop()
        {
            auto p = ReadDequeuePointer();
            ++p;

            const auto& current_segment = erst_[erst_index_];
            if (p == current_segment.cend())
            {
                ++erst_index_;
                if (erst_index_ == kNumTables)
                {
                    erst_index_ = 0;
                    producer_cycle_ = 1 - producer_cycle_;
                }
                p = erst_[erst_index_].begin();
            }

            WriteDequeuePointer(p);
        }
    };
}
