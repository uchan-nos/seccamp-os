#include "usb/xhci/xhci.hpp"

namespace usb::xhci
{
  Error RealController::Initialize()
  {
    if (auto err = devmgr_.Initialize(kDeviceSize); IsError(err))
    {
      return err;
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
    while (op_->USBCMD.Read().bits.host_controller_reset);
    while (op_->USBSTS.Read().bits.controller_not_ready);

    printk("MaxSlots: %u\n", cap_->HCSPARAMS1.Read().bits.max_device_slots);
    // Set "Max Slots Enabled" field in CONFIG.
    auto config = op_->CONFIG.Read();
    config.bits.max_device_slots_enabled = kDeviceSize;
    op_->CONFIG.Write(config);

    DCBAAP_Bitmap dcbaap{};
    dcbaap.SetPointer(reinterpret_cast<uint64_t>(devmgr_.DeviceContexts()));
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

    // Enable interrupt for the controller
    usbcmd = op_->USBCMD.Read();
    usbcmd.bits.interrupter_enable = true;
    op_->USBCMD.Write(usbcmd);

    return error::kSuccess;
  }

  Error RealController::Run()
  {
    // Run the controller
    auto usbcmd = op_->USBCMD.Read();
    usbcmd.bits.run_stop = true;
    op_->USBCMD.Write(usbcmd);
    op_->USBCMD.Read();

    while (op_->USBSTS.Read().bits.host_controller_halted);

    return error::kSuccess;
  }

  DoorbellRegister* RealController::DoorbellRegisterAt(uint8_t index)
  {
    return &DoorbellRegisters()[index];
  }
}
