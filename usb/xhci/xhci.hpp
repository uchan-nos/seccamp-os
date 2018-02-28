#pragma once

#include "usb/error.hpp"
#include "usb/xhci/registers.hpp"
#include "usb/xhci/context.hpp"
#include "usb/xhci/ring.hpp"
#include "usb/xhci/port.hpp"
#include "usb/xhci/devmgr.hpp"

namespace usb::xhci
{
  class Controller
  {
  public:
    virtual ~Controller() {}
    virtual Error Initialize() = 0;
    virtual Ring* CommandRing() = 0;
    virtual EventRing* PrimaryEventRing() = 0;
    virtual DoorbellRegister* DoorbellRegisterAt(uint8_t index) = 0;
    virtual Port PortAt(uint8_t port_num) = 0;
    virtual uint8_t MaxPorts() const = 0;
    virtual DeviceManager* DeviceManager() = 0;
  };

  class RealController : public Controller
  {
    static const size_t kDeviceSize = 8;

    const uintptr_t mmio_base_;
    CapabilityRegisters* const cap_;
    OperationalRegisters* const op_;
    const uint8_t max_ports_;

    class DeviceManager devmgr_;
    Ring cr_;
    EventRing er_;

    InterrupterRegisterSetArray InterrupterRegisterSets() const
    {
      return {mmio_base_ + cap_->RTSOFF.Read().Offset() + 0x20u, 1024};
    }

    PortRegisterSetArray PortRegisterSets() const
    {
      return {reinterpret_cast<uintptr_t>(op_) + 0x400u, max_ports_};
    }

    DoorbellRegisterArray DoorbellRegisters() const
    {
      return {mmio_base_ + cap_->DBOFF.Read().Offset(), 256};
    }

  public:
    RealController(uintptr_t mmio_base)
      : mmio_base_{mmio_base},
        cap_{reinterpret_cast<CapabilityRegisters*>(mmio_base)},
        op_{reinterpret_cast<OperationalRegisters*>(mmio_base + cap_->CAPLENGTH.Read())},
        max_ports_{static_cast<uint8_t>(cap_->HCSPARAMS1.Read().bits.max_ports)}
    {}

    Error Initialize();
    DoorbellRegister* DoorbellRegisterAt(uint8_t index);

    Ring* CommandRing() { return &cr_; }
    EventRing* PrimaryEventRing() { return &er_; }
    Port PortAt(uint8_t port_num) { return Port{*this, port_num, PortRegisterSets()[port_num - 1]}; }
    uint8_t MaxPorts() const { return max_ports_; }
    class DeviceManager* DeviceManager() { return &devmgr_; }

    void* operator new(size_t size) { return AllocObject<RealController>(); }
    void operator delete(void* ptr) { return FreeObject(ptr); }
  };
}
