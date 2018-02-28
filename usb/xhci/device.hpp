#pragma once

#include <stdint.h>

#include "usb/error.hpp"
#include "usb/endpoint.hpp"
#include "usb/xhci/context.hpp"
#include "usb/xhci/ring.hpp"

namespace usb::xhci
{
  inline SetupStageTRB MakeSetupStageTRB(uint64_t setup_data, int transfer_type)
  {
    SetupStageTRB setup{};
    setup.bits.request_type = setup_data & 0xffu;
    setup.bits.request = (setup_data >> 8) & 0xffu;
    setup.bits.value = (setup_data >> 16) & 0xffffu;
    setup.bits.index = (setup_data >> 32) & 0xffffu;
    setup.bits.length = (setup_data >> 48) & 0xffffu;
    setup.bits.transfer_type = transfer_type;
    return setup;
  }

  inline DataStageTRB MakeDataStageTRB(const void* buf, int len, bool dir_in)
  {
    DataStageTRB data{};
    data.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(buf);
    data.bits.trb_transfer_length = len;
    data.bits.td_size = 0;
    data.bits.direction = dir_in;
    return data;
  }

  class EndpointSet : public usb::EndpointSet
  {
    DeviceContext* devctx_;
    Ring* rings_[31];

  public:
    EndpointSet(DeviceContext* devctx)
      : devctx_{devctx}, rings_{}
    {}

    void SetTransferRing(DeviceContextIndex dci, Ring* ring)
    {
      rings_[dci.value - 1] = ring;
    }

    Ring* TransferRing(DeviceContextIndex dci) const
    {
      return rings_[dci.value - 1];
    }

    virtual Error ControlOut(int ep_num, uint64_t setup_data,
                             const void* buf, int len) override
    {
      if (ep_num < 0 || 15 < ep_num)
      {
        return error::kInvalidEndpointNumber;
      }

      // control endpoint must be dir_in=true
      const DeviceContextIndex dci(ep_num, true);

      EndpointContext* epctx = &devctx_->ep_contexts[dci.value - 1];
      Ring* tr = rings_[dci.value - 1];

      if (tr == nullptr)
      {
        return error::kTransferRingNotSet;
      }

      if (buf)
      {
        tr->Push(MakeSetupStageTRB(setup_data, SetupStageTRB::kOutDataStage));
        tr->Push(MakeDataStageTRB(buf, len, false));
      }
      else
      {
        tr->Push(MakeSetupStageTRB(setup_data, SetupStageTRB::kNoDataStage));
      }

      auto status = StatusStageTRB{};
      status.bits.direction = true;
      status.bits.interrupt_on_completion = true;
      tr->Push(status);

      return error::kSuccess;
    }

    virtual Error ControlIn(int ep_num, uint64_t setup_data,
                            void* buf, int len) override
    {
      if (ep_num < 0 || 15 < ep_num)
      {
        return error::kInvalidEndpointNumber;
      }

      // control endpoint must be dir_in=true
      const DeviceContextIndex dci(ep_num, true);

      EndpointContext* epctx = &devctx_->ep_contexts[dci.value - 1];
      Ring* tr = rings_[dci.value - 1];

      if (tr == nullptr)
      {
        return error::kTransferRingNotSet;
      }

      auto status = StatusStageTRB{};
      status.bits.interrupt_on_completion = true;

      if (buf)
      {
        tr->Push(MakeSetupStageTRB(setup_data, SetupStageTRB::kInDataStage));
        tr->Push(MakeDataStageTRB(buf, len, true));
      }
      else
      {
        tr->Push(MakeSetupStageTRB(setup_data, SetupStageTRB::kNoDataStage));
        status.bits.direction = true;
      }

      tr->Push(status);

      return error::kSuccess;
    }

    virtual Error Send(int ep_num, const void* buf, int len) override
    {
      return error::kSuccess;
    }

    virtual Error Receive(int ep_num, void* buf, int len) override
    {
      return error::kSuccess;
    }
  };

  class Device
  {
  public:
    enum class State
    {
      kInvalid,
      kBlank,
      kSlotAssigning,
      kSlotAssigned
    };

    using OnTransferredCallbackType = void (
        Device* dev,
        DeviceContextIndex dci,
        int completion_code,
        int trb_transfer_length,
        TRB* issue_trb);

    Error Initialize()
    {
      state_ = State::kBlank;
      for (size_t i = 0; i < 31; ++i)
      {
        const DeviceContextIndex dci(i + 1);
        FreeObject(epset_.TransferRing(dci));
        epset_.SetTransferRing(dci, nullptr);
        on_transferred_callbacks_[i] = nullptr;
      }
      return error::kSuccess;
    }

    DeviceContext* DeviceContext() { return &ctx_; }
    InputContext* InputContext() { return &input_ctx_; }
    class EndpointSet* EndpointSet() { return &epset_; }

    State State() const { return state_; }
    uint8_t SlotID() const { return slot_id_; }

    void SelectForSlotAssignment()
    {
      state_ = State::kSlotAssigning;
    }

    void AssignSlot(uint8_t slot_id)
    {
      slot_id_ = slot_id;
      state_ = State::kSlotAssigned;
    }

    Error AllocTransferRing(DeviceContextIndex index, size_t buf_size)
    {
      int i = index.value - 1;
      auto tr = AllocObject<Ring>();
      tr->Initialize(buf_size);
      epset_.SetTransferRing(index, tr);
      return error::kSuccess;
    }

    void SetOnTransferred(
        DeviceContextIndex dci, OnTransferredCallbackType* callback)
    {
      on_transferred_callbacks_[dci.value - 1] = callback;
    }

    /** @brief OnTransferred calls a callback function if exist.
     * This function will be called when a Transfer Event for this device
     * (slot) has been received.
     *
     * @param dci  An endpoint to which the event targets.
     * @param completion_code  A completion code of the event.
     * @param trb_transfer_length  The residual number of bytes not transferred.
     * @param issue_trb  Pointer to the TRB which issued the event.
     */
    void OnTransferred(
        DeviceContextIndex dci,
        int completion_code,
        int trb_transfer_length,
        TRB* issue_trb)
    {
      auto callback = on_transferred_callbacks_[dci.value - 1];
      if (callback)
      {
        callback(this, dci, completion_code, trb_transfer_length, issue_trb);
      }
    }

    template <typename TRBType>
    void OnTransferred(
        DeviceContextIndex dci,
        int completion_code,
        int trb_transfer_length,
        TRBType* issue_trb)
    {
      OnTransferred(dci, completion_code, trb_transfer_length,
                    reinterpret_cast<TRB*>(issue_trb));
    }

  private:
    alignas(64) struct DeviceContext ctx_;
    alignas(64) struct InputContext input_ctx_;
    class EndpointSet epset_;
    OnTransferredCallbackType* on_transferred_callbacks_[31];

    enum State state_;
    uint8_t slot_id_;
  };
}
