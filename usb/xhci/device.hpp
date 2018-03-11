#pragma once

#include <stdint.h>

#include "usb/error.hpp"
#include "usb/endpoint.hpp"
#include "usb/busdriver.hpp"
#include "usb/xhci/context.hpp"
#include "usb/xhci/ring.hpp"
#include "usb/xhci/registers.hpp"

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
    Ring* rings_[31];
    DoorbellRegister* dbreg_;

  public:
    EndpointSet(Ring* default_control_pipe, DoorbellRegister* dbreg)
      : rings_{default_control_pipe,}, dbreg_{dbreg}
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

      Ring* tr = rings_[dci.value - 1];

      if (tr == nullptr)
      {
        return error::kTransferRingNotSet;
      }

      auto status = StatusStageTRB{};
      //status.bits.interrupt_on_completion = true;

      if (buf)
      {
        tr->Push(MakeSetupStageTRB(setup_data, SetupStageTRB::kInDataStage));
        //tr->Push(MakeDataStageTRB(buf, len, true));
        auto data = MakeDataStageTRB(buf, len, true);
        data.bits.interrupt_on_completion = true;
        tr->Push(data);
      }
      else
      {
        tr->Push(MakeSetupStageTRB(setup_data, SetupStageTRB::kNoDataStage));
        status.bits.direction = true;
      }

      tr->Push(status);

      dbreg_->Ring(dci.value);

      return error::kSuccess;
    }

    virtual Error Send(int ep_num, const void* buf, int len) override
    {
      return error::kSuccess;
    }

    virtual Error Receive(int ep_num, void* buf, int len) override
    {
      if (ep_num < 0 || 15 < ep_num)
      {
        return error::kInvalidEndpointNumber;
      }

      const DeviceContextIndex dci(ep_num, true);
      Ring* tr = rings_[dci.value - 1];

      if (tr == nullptr)
      {
        return error::kTransferRingNotSet;
      }

      NormalTRB normal{};
      normal.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(buf);
      normal.bits.trb_transfer_length = len;
      normal.bits.td_size = 0;
      normal.bits.interrupter_target = 0;
      normal.bits.interrupt_on_completion = true;
      tr->Push(normal);

      dbreg_->Ring(dci.value);

      return error::kSuccess;
    }

    void* operator new(size_t size) { return AllocObject<Device>(); }
    void operator delete(void* ptr) { return FreeObject(ptr); }
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
        //on_transferred_callbacks_[i] = nullptr;
      }
      return error::kSuccess;
    }

    DeviceContext* DeviceContext() { return &ctx_; }
    InputContext* InputContext() { return &input_ctx_; }
    usb::Device* USBDevice() { return usb_device_; }
    void SetUSBDevice(usb::Device* value) { usb_device_ = value; }

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
      reinterpret_cast<usb::xhci::EndpointSet*>(usb_device_->EndpointSet())
        ->SetTransferRing(index, tr);
      return error::kSuccess;
    }

  private:
    alignas(64) struct DeviceContext ctx_;
    alignas(64) struct InputContext input_ctx_;

    enum State state_;
    uint8_t slot_id_;

    usb::Device* usb_device_;
  };
}
