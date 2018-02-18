#include "usb/xhci/ring.hpp"
#include "usb/xhci/trb.hpp"

class EventRingProducer
{
  usb::xhci::InterrupterRegisterSet* intr_;
  bool cycle_bit_ = 1;

  size_t erst_index_ = 0;
  size_t enqueue_index_ = 0;

  usb::xhci::EventRingSegmentTableEntry* erst_;
  size_t erst_size_;

  usb::xhci::TRB* GetEnqueuePointer() const
  {
    return reinterpret_cast<usb::xhci::TRB*>(
        erst_[erst_index_].Pointer()) + enqueue_index_;
  }

public:
  void Initialize(usb::xhci::InterrupterRegisterSet* intr)
  {
    intr_ = intr;
    erst_ = reinterpret_cast<usb::xhci::EventRingSegmentTableEntry*>(
        intr_->ERSTBA.Read().Pointer());
    erst_size_ = intr_->ERSTSZ.Read().Size();
  }

  template <typename TRB>
  void Push(TRB trb)
  {
    trb.bits.cycle_bit = cycle_bit_;
    for (size_t i = 0; i < 4; ++i)
    {
      GetEnqueuePointer()->data[i] = trb.data[i];
    }

    ++enqueue_index_;
    if (enqueue_index_ == erst_[erst_index_].Size())
    {
      enqueue_index_ = 0;
      ++erst_index_;
      if (erst_index_ == erst_size_)
      {
        erst_index_ = 0;
      }
    }
  }
};
