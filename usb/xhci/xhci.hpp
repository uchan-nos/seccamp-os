#pragma once

#include "usb/error.hpp"
#include "usb/xhci/registers.hpp"
#include "usb/xhci/context.hpp"
#include "usb/xhci/ring.hpp"
#include "usb/xhci/port.hpp"

namespace usb::xhci
{
  struct Controller
  {
    virtual ~Controller() {}
    virtual Error Initialize() = 0;
    virtual Ring* CommandRing() = 0;
    virtual EventRing* PrimaryEventRing() = 0;
    virtual void RingDoorbell(uint8_t target, uint16_t stream_id = 0) = 0;
    virtual Port PortAt(size_t index) = 0;
    virtual uint8_t MaxPorts() const = 0;
  };

  class RealController : public Controller
  {
    static const size_t kDeviceContextSize = 8;

    const uintptr_t mmio_base_;
    CapabilityRegisters* const cap_;
    OperationalRegisters* const op_;
    const uint8_t max_ports_;

    DeviceContext** device_context_pointers_;
    Ring cr_;
    EventRing er_;

    InterrupterRegisterSetArray InterrupterRegisterSets() const
    {
      return {
        mmio_base_ + cap_->RTSOFF.Read().Offset() + 0x20u,
        1024
      };
    }

    PortRegisterSetArray PortRegisterSets() const
    {
      return {
        reinterpret_cast<uintptr_t>(op_) + 0x400u,
        max_ports_
      };
    }

    DoorbellRegisterArray DoorbellRegisters() const
    {
      return {
        mmio_base_ + cap_->DBOFF.Read().Offset(),
        256
      };
    }

  public:
    RealController(uintptr_t mmio_base)
      : mmio_base_{mmio_base},
        cap_{reinterpret_cast<CapabilityRegisters*>(mmio_base)},
        op_{reinterpret_cast<OperationalRegisters*>(mmio_base + cap_->CAPLENGTH.Read())},
        max_ports_{static_cast<uint8_t>(cap_->HCSPARAMS1.Read().bits.max_ports)}
    {}

    Error Initialize()
    {
      device_context_pointers_ =
        AllocArray<DeviceContext*>(kDeviceContextSize);
      if (device_context_pointers_ == nullptr)
      {
        return error::kNoEnoughMemory;
      }

      // Host controller must be halted.
      if (!op_->USBSTS.Read().bits.host_controller_halted)
      {
        return error::kHostControllerNotHalted;
      }

      // Reset controller
      auto usbcmd = op_->USBCMD.Read();
      usbcmd.bits.host_controller_reset = true;
      op_->USBCMD.Write(usbcmd);

      while (op_->USBSTS.Read().bits.controller_not_ready);

      // Set "Max Slots Enabled" field in CONFIG.
      auto config = op_->CONFIG.Read();
      config.bits.max_device_slots_enabled = kDeviceContextSize;
      op_->CONFIG.Write(config);

      DCBAAP_Bitmap dcbaap{};
      dcbaap.SetPointer(reinterpret_cast<uint64_t>(device_context_pointers_));
      op_->DCBAAP.Write(dcbaap);

      auto primary_interrupter = &InterrupterRegisterSets()[0];
      if (auto err = cr_.Initialize(32); err)
      {
          return err;
      }
      if (auto err = RegisterCommandRing(&cr_, &op_->CRCR); err)
      {
          return err;
      }
      if (auto err = er_.Initialize(32, primary_interrupter); err)
      {
          return err;
      }

      // Enable interrupt for the primary interrupter
      auto iman = primary_interrupter->IMAN.Read();
      iman.bits.interrupt_pending = true;
      iman.bits.interrupt_enable = true;
      primary_interrupter->IMAN.Write(iman);

      // Set Run/Stop bit
      usbcmd = op_->USBCMD.Read();
      usbcmd.bits.run_stop = true;
      usbcmd.bits.interrupter_enable = true;
      op_->USBCMD.Write(usbcmd);

      return error::kSuccess;
    }

    Ring* CommandRing() { return &cr_; }
    EventRing* PrimaryEventRing() { return &er_; }

    void RingDoorbell(uint8_t target, uint16_t stream_id)
    {
      Doorbell_Bitmap value{};
      value.bits.db_target = target;
      value.bits.db_stream_id = stream_id;
      DoorbellRegisters()[target].Write(value);
    }

    Port PortAt(size_t index) { return Port{&PortRegisterSets()[index]}; }
    uint8_t MaxPorts() const { return max_ports_; }

    void* operator new(size_t size) { return AllocObject<Controller>(); }
    void operator delete(void* ptr) { return FreeObject(ptr); }
  };
}
