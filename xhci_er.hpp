#pragma once

namespace bitnos::xhci::eventring
{
    class SegmentTableEntry
    {
    public:
        using ValueType = TRB;
        using Iterator = ValueType*;
        using ConstIterator = const ValueType*;

        SegmentTableEntry(uint64_t segment_base_addr, uint16_t segment_size)
            : segment_base_address_(segment_base_addr),
              segment_size_(segment_size)
        {}

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
        InterrupterRegSet& int_reg_set_;
        unsigned char producer_cycle_;
        SegmentTableEntry* erst_;
        size_t erst_size_;
        size_t erst_index_;

        TRB* ReadDequeuePointer()
        {
            return reinterpret_cast<TRB*>(
                bitutil::ClearBits(int_reg_set_.ERDP.Read(), 0xfu));
        }

        void WriteDequeuePointer(TRB* p)
        {
            int_reg_set_.ERDP.Write(reinterpret_cast<uint64_t>(p));
        }

    public:
        Manager(InterrupterRegSet& int_reg_set)
            : int_reg_set_(int_reg_set)
        {}

        SegmentTableEntry* SegmentTable() const { return erst_; }
        size_t SegmentTableSize() const { return erst_size_; }

        void Initialize()
        {
            const auto erstba = int_reg_set_.ERSTBA.Read();

            producer_cycle_ = 1;
            erst_ = reinterpret_cast<SegmentTableEntry*>(
                bitutil::ClearBits(erstba, 0x3fu));
            erst_size_ = int_reg_set_.ERSTSZ.Read() & 0xffffu;
            erst_index_ = 0;

            WriteDequeuePointer(erst_[0].begin());

            for (size_t i = 0; i < erst_size_; ++i)
            {
                auto& segment = erst_[i];
                memset(segment.begin(), 0, sizeof(TRB) * segment.Size());
            }

            // reload base address to reset the state of the ring
            int_reg_set_.ERSTBA.Write(erstba);
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
                if (erst_index_ == erst_size_)
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
