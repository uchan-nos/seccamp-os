#pragma once

/**
 * Command, transfer, event ring implementation.
 */

#include "usb/error.hpp"
#include "usb/malloc.hpp"
#include "usb/xhci/registers.hpp"
#include "usb/xhci/trb.hpp"

namespace usb::xhci
{
  /**
   * Command and transfer ring
   */
  class Ring
  {
    TRB* buf_ = nullptr;
    size_t buf_size_ = 0;

    bool cycle_bit_;
    size_t write_index_;

    void CopyToLast(const uint32_t* data)
    {
      for (int i = 0; i < 3; ++i)
      {
        // data[0..2] must be written prior to data[3].
        buf_[write_index_].data[i] = data[i];
      }
      buf_[write_index_].data[3]
        = (data[3] & 0xfffffffeu) | static_cast<uint32_t>(cycle_bit_);
    }

    void Push(const uint32_t* data)
    {
      CopyToLast(data);

      ++write_index_;
      if (write_index_ == buf_size_ - 1)
      {
        LinkTRB link{buf_};
        link.bits.toggle_cycle = true;
        CopyToLast(link.data);

        write_index_ = 0;
        cycle_bit_ = !cycle_bit_;
      }
    }

  public:
    Ring() {}

    Error Initialize(size_t buf_size)
    {
      cycle_bit_ = true;
      write_index_ = 0;
      buf_size_ = buf_size;

      buf_ = usb::AllocArray<TRB>(buf_size_, 64, 64 * 1024);
      if (buf_ == nullptr)
      {
        return error::kNoEnoughMemory;
      }

      return error::kSuccess;
    }

    Error Initialize(TRB* buf, size_t buf_size)
    {
      cycle_bit_ = true;
      write_index_ = 0;
      buf_ = buf;
      buf_size_ = buf_size;

      return error::kSuccess;
    }

    template <typename TRBType>
    void Push(const TRBType& trb)
    {
      Push(trb.data);
    }

    TRB* Buffer() const { return buf_; }
  };

  Error RegisterCommandRing(Ring* ring, MemMapRegister<CRCR_Bitmap>* crcr);

  union EventRingSegmentTableEntry
  {
    uint32_t data[4];
    struct
    {
      uint64_t : 6;
      uint64_t ring_segment_base_address : 58;

      uint32_t ring_segment_size : 16;
      uint32_t : 16;

      uint32_t : 32;
    } __attribute__((packed)) bits;

    uint64_t Pointer() const
    {
      return bits.ring_segment_base_address << 6;
    }

    uint16_t Size() const
    {
      return bits.ring_segment_size;
    }

    void SetPointerAndSize(TRB* segment_base, uint16_t segment_size)
    {
        bits.ring_segment_base_address =
          reinterpret_cast<uint64_t>(segment_base) >> 6;
        bits.ring_segment_size = segment_size;
    }
  } __attribute__((packed));

  class EventRing
  {
    EventRingSegmentTableEntry* erst_;
    bool cycle_bit_;
    TRB* buf_;
    size_t buf_size_;
    InterrupterRegisterSet* interrupter_;

  public:
    TRB* ReadDequeuePointer() const
    {
      return reinterpret_cast<TRB*>(interrupter_->ERDP.Read().Pointer());
    }

    void WriteDequeuePointer(TRB* p)
    {
      auto erdp = interrupter_->ERDP.Read();
      erdp.SetPointer(reinterpret_cast<uint64_t>(p));
      interrupter_->ERDP.Write(erdp);
    }

  public:
    EventRing()
    {}

    Error Initialize(size_t buf_size, InterrupterRegisterSet* interrupter)
    {
      cycle_bit_ = true;
      buf_size_ = buf_size;
      interrupter_ = interrupter;

      buf_ = usb::AllocArray<TRB>(buf_size_, 64, 64 * 1024);
      if (buf_ == nullptr)
      {
        return error::kNoEnoughMemory;
      }

      erst_ = usb::AllocArray<EventRingSegmentTableEntry>(1);
      if (erst_ == nullptr)
      {
        usb::FreeArray(buf_);
        return error::kNoEnoughMemory;
      }

      erst_[0].SetPointerAndSize(buf_, buf_size_);

      ERSTSZ_Bitmap erstsz{};
      erstsz.SetSize(1);
      interrupter_->ERSTSZ.Write(erstsz);

      WriteDequeuePointer(&buf_[0]);

      ERSTBA_Bitmap erstba{};
      erstba.SetPointer(reinterpret_cast<uint64_t>(erst_));
      interrupter_->ERSTBA.Write(erstba);

      return error::kSuccess;
    }

    bool HasFront() const
    {
      return Front()->bits.cycle_bit == cycle_bit_;
    }

    TRB* Front() const
    {
      return ReadDequeuePointer();
    }

    void Pop()
    {
      auto p = ReadDequeuePointer();
      ++p;

      if (p == reinterpret_cast<TRB*>(erst_[0].Pointer()) + erst_[0].Size())
      {
        p = reinterpret_cast<TRB*>(erst_[0].Pointer());
      }

      WriteDequeuePointer(p);
    }
  };
}
