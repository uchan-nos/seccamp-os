#include "usb/xhci/interrupt.hpp"

namespace usb::xhci
{
  void InterruptHandler(const CallbackContext* ctx)
  {
    auto er = ctx->xhc->PrimaryEventRing();

    while (er->HasFront())
    {
      auto trb = er->Front();
      if (auto trb_ = TRBDynamicCast<TransferEventTRB>(trb); trb_)
      {
        InterruptMessage msg{};
        msg.type = InterruptMessageType::kXHCITransferEvent;
        msg.attr1 = trb_->bits.completion_code;
        msg.attr2 = trb_->bits.trb_transfer_length;
        msg.attr3 = trb_->bits.slot_id;
        msg.attr4 = trb_->bits.endpoint_id;
        msg.data = reinterpret_cast<void*>(trb_->bits.trb_pointer);
        ctx->EnqueueMessage(ctx, &msg);
      }
      else if (auto trb_ = TRBDynamicCast<CommandCompletionEventTRB>(trb); trb_)
      {
        InterruptMessage msg{};
        msg.type = InterruptMessageType::kXHCICommandCompletionEvent;
        msg.attr1 = trb_->bits.completion_code;
        msg.attr2 = trb_->bits.command_completion_parameter;
        msg.attr3 = trb_->bits.slot_id;
        msg.data = reinterpret_cast<void*>(trb_->bits.command_trb_pointer << 4);
        ctx->EnqueueMessage(ctx, &msg);
      }
      else if (auto trb_ = TRBDynamicCast<PortStatusChangeEventTRB>(trb); trb_)
      {
        InterruptMessage msg{};
        msg.type = InterruptMessageType::kXHCIPortStatusChangeEvent;
        msg.attr1 = trb_->bits.port_id;
        msg.data = nullptr;
        ctx->EnqueueMessage(ctx, &msg);
      }
      er->Pop();
    }
  }

  /** @brief Pointers to devices waiting for slot assignment.
   *
   * Indices are port numbers (1 .. 255).
   */
  Device* slot_assigning_devices[256] = {};

  void OnPortStatusChanged(const CallbackContext* ctx, const InterruptMessage& msg)
  {
    const uint8_t port_id = msg.attr1 & 0xffu;
    auto port = ctx->xhc->PortAt(port_id);

    auto devmgr = ctx->xhc->DeviceManager();
    auto dev = devmgr->FindByPort(port_id, 0);

    printk("OnPortStatusChanged: port %u, dev = %08lx, CSC = %u, PED = %u, IsConnected %u\n",
        port_id,
        dev,
        port.IsEnabled(),
        port.IsConnectStatusChanged(),
        port.IsConnected());

    if (!port.IsConnectStatusChanged())
    {
      return;
    }

    if (dev == nullptr && port.IsConnected() &&
        slot_assigning_devices[port_id] == nullptr)
    {
      dev = devmgr->FindByState(Device::State::kBlank);
      if (dev == nullptr)
      {
        printk("OnPortStatusChanged: No blank device\n");
        return;
      }

      printk("Resetting port\n");
      port.Reset();

      dev->SelectForSlotAssignment();
      slot_assigning_devices[port_id] = dev;

      EnableSlotCommandTRB cmd{};
      ctx->xhc->CommandRing()->Push(cmd);
      ctx->xhc->DoorbellRegisterAt(0)->Ring(0);
    }
    else if (dev != nullptr && !port.IsConnected())
    {
      devmgr->Remove(dev);
    }
  }

  int FindSlotAssigningDevice()
  {
    for (int port_id = 1; port_id < 256; ++port_id)
    {
      if (slot_assigning_devices[port_id] != nullptr)
      {
        return port_id;
      }
    }
    return -1;
  }

  struct ConfigureEndpointsArg
  {
    Controller* xhc;
    uint8_t slot_id;
  };

  Error ConfigureEndpoints(EndpointConfig* configs, int len, void* arg)
  {
    const auto arg_ = reinterpret_cast<ConfigureEndpointsArg*>(arg);
    auto dev = arg_->xhc->DeviceManager()->FindBySlot(arg_->slot_id);
    auto devctx = dev->DeviceContext();
    auto inpctx = dev->InputContext();

    memset(&inpctx->input_control_context, 0, sizeof(InputControlContext));
    memcpy(&inpctx->slot_context, &devctx->slot_context, sizeof(SlotContext));

    auto slot_ctx = inpctx->EnableSlotContext();
    slot_ctx->bits.context_entries = 31;

    for (int i = 0; i < len; ++i)
    {
      const DeviceContextIndex ep_dci(configs[i].ep_num, configs[i].dir_in);
      auto ep_ctx = inpctx->EnableEndpoint(ep_dci);
      switch (configs[i].ep_type)
      {
      case EndpointType::kControl:
        ep_ctx->bits.ep_type = 4;
        break;
      case EndpointType::kIsochronous:
        ep_ctx->bits.ep_type = configs[i].dir_in ? 5 : 1;
        break;
      case EndpointType::kBulk:
        ep_ctx->bits.ep_type = configs[i].dir_in ? 6 : 2;
        break;
      case EndpointType::kInterrupt:
        ep_ctx->bits.ep_type = configs[i].dir_in ? 7 : 3;
        break;
      }
      ep_ctx->bits.max_packet_size = configs[i].max_packet_size;
      ep_ctx->bits.interval = configs[i].interval;
      ep_ctx->bits.average_trb_length = 1;

      auto tr = AllocObject<Ring>();
      tr->Initialize(32);
      const auto tr_addr = reinterpret_cast<uintptr_t>(tr->Buffer());

      reinterpret_cast<usb::xhci::EndpointSet*>(dev->USBDevice()->EndpointSet())
        ->SetTransferRing(ep_dci, tr);

      ep_ctx->SetTransferRing(tr_addr);
      ep_ctx->bits.dequeue_cycle_state = 1;
      ep_ctx->bits.max_primary_streams = 0;
      ep_ctx->bits.mult = 0;
      ep_ctx->bits.error_count = 3;
    }

    ConfigureEndpointCommandTRB cmd{inpctx, arg_->slot_id};
    arg_->xhc->CommandRing()->Push(cmd);
    arg_->xhc->DoorbellRegisterAt(0)->Ring(0);

    return error::kSuccess;
  }

  Error AddressDevice(Controller* xhc, Device* dev, const Port& port, uint8_t slot_id)
  {
    const DeviceContextIndex dci_ep0{0, false};

    auto dbreg = xhc->DoorbellRegisterAt(slot_id);

    auto tr = AllocObject<Ring>();
    tr->Initialize(32);

    auto configure_endpoints_arg = AllocObject<ConfigureEndpointsArg>();
    configure_endpoints_arg->xhc = xhc;
    configure_endpoints_arg->slot_id = slot_id;

    auto epset = new EndpointSet(tr, dbreg);
    auto usb_dev =
      new usb::Device(epset, ConfigureEndpoints, configure_endpoints_arg);
    dev->SetUSBDevice(usb_dev);

    const auto tr_addr = reinterpret_cast<uintptr_t>(tr->Buffer());

    memset(dev->InputContext(), 0, sizeof(InputContext));
    auto slot_ctx = dev->InputContext()->EnableSlotContext();

    slot_ctx->bits.route_string = 0;
    slot_ctx->bits.root_hub_port_num = port.Number();
    slot_ctx->bits.context_entries = 1;
    slot_ctx->bits.speed = port.Speed();

    auto ep_ctx = dev->InputContext()->EnableEndpoint(dci_ep0);

    ep_ctx->bits.ep_type = 4; // Control Endpoint. Bidirectional.
    switch (slot_ctx->bits.speed)
    {
    case 4: // Super Speed
        ep_ctx->bits.max_packet_size = 512; // shall be set to 512 for control endpoints
        break;
    case 3: // High Speed
        ep_ctx->bits.max_packet_size = 64;
        break;
    default:
        ep_ctx->bits.max_packet_size = 8;
    }
    ep_ctx->bits.max_burst_size = 0;
    ep_ctx->SetTransferRing(tr_addr);
    ep_ctx->bits.dequeue_cycle_state = 1;
    ep_ctx->bits.interval = 0;
    ep_ctx->bits.max_primary_streams = 0;
    ep_ctx->bits.mult = 0;
    ep_ctx->bits.error_count = 3;

    memset(dev->DeviceContext(), 0, sizeof(DeviceContext));
    xhc->DeviceManager()->AssignSlot(dev, slot_id);

    AddressDeviceCommandTRB cmd{dev->InputContext(), slot_id};
    xhc->CommandRing()->Push(cmd);
    xhc->DoorbellRegisterAt(0)->Ring(0);

    return error::kSuccess;
  }

  //void OnConfigureDescriptorReceived(

  void OnCommandCompleted(const CallbackContext* ctx, const InterruptMessage& msg)
  {
    TRB* issue_trb = static_cast<TRB*>(msg.data);
    const uint8_t slot_id = msg.attr3;

    printk("OnCommandCompleted: %s, Code = %u, Param = %u, SlotID = %u\n",
        kTRBTypeToName[issue_trb->bits.trb_type],
        msg.attr1, msg.attr2, slot_id);

    if (auto t = TRBDynamicCast<EnableSlotCommandTRB>(issue_trb); t)
    {
      int port_id = FindSlotAssigningDevice();
      if (port_id == -1)
      {
        printk("OnCommandCompleted: no device waiting for slot assignment\n");
        return;
      }

      Device* dev = slot_assigning_devices[port_id];
      slot_assigning_devices[port_id] = nullptr;

      Port port = ctx->xhc->PortAt(port_id);

      if (auto err = AddressDevice(ctx->xhc, dev, port, slot_id); IsError(err))
      {
        printk("OnCommandCompleted: AddressDevice failed: %d\n", err);
      }
    }
    else if (auto t = TRBDynamicCast<AddressDeviceCommandTRB>(issue_trb); t)
    {
      Device* dev = ctx->xhc->DeviceManager()->FindBySlot(slot_id);
      if (dev == nullptr)
      {
        printk("No device with the SlotID = %u\n", slot_id);
        return;
      }

      // Now the device's default control pipe has been enabled.
      if (auto err = dev->USBDevice()->ConfigureEndpoints(); IsError(err))
      {
        printk("OnCommandCompleted: ConfigureEndpoints failed: %d\n", err);
      }
    }
    else if (auto t = TRBDynamicCast<ConfigureEndpointCommandTRB>(issue_trb); t)
    {
      Device* dev = ctx->xhc->DeviceManager()->FindBySlot(slot_id);
      if (dev == nullptr)
      {
        printk("No device with the SlotID = %u\n", slot_id);
        return;
      }

      const uint32_t add_context_flags =
        dev->InputContext()->input_control_context.add_context_flags;
      unsigned int configured_ep_nums = 0;
      for (unsigned int i = 1; i <= 15; ++i)
      {
        DeviceContextIndex dci{i, false};
        if ((add_context_flags >> dci.value) & 3)
        {
          configured_ep_nums |= (1u << i);
        }
      }

      printk("Calling OnEndpointConfigured\n");
      dev->USBDevice()->OnEndpointConfigured(configured_ep_nums);
    }
  }

  void OnTransferCompleted(const CallbackContext* ctx, const InterruptMessage& msg)
  {
    const auto slot_id = msg.attr3;
    const auto endpoint_id = msg.attr4;

    auto dev = ctx->xhc->DeviceManager()->FindBySlot(slot_id);
    if (dev == nullptr)
    {
      printk("No such device with SlotID = %u\n", slot_id);
      return;
    }

    dev->USBDevice()->OnCompleted(endpoint_id / 2);
  }

  void PostInterruptHandler(const CallbackContext* ctx, const InterruptMessage& msg)
  {
    switch (msg.type)
    {
    case InterruptMessageType::kXHCITransferEvent:
      OnTransferCompleted(ctx, msg);
      break;
    case InterruptMessageType::kXHCICommandCompletionEvent:
      OnCommandCompleted(ctx, msg);
      break;
    case InterruptMessageType::kXHCIPortStatusChangeEvent:
      OnPortStatusChanged(ctx, msg);
      break;
    }
  }
}
