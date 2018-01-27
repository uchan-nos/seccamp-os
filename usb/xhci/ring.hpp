#pragma once

/**
 * Command and transfer ring implementation.
 */

#include "usb/xhci/trb.hpp"

namespace usb::xhci
{
  class Ring
  {
    bool cycle_bit_ = true;
    size_t write_index_ = 0;
    TRB* buf_;
    size_t buf_size_;

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
    Ring(TRB* buf, size_t buf_size)
      : buf_{buf}, buf_size_{buf_size}
    {}

    template <typename TRBType>
    void Push(const TRBType& trb)
    {
      Push(trb.data);
    }
  };
}
