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

    printk("OnPortStatusChanged: port %u, dev = %08lx, CSC = %u, IsConnected %u\n",
        port_id,
        dev,
        port.IsConnectStatusChanged(),
        port.IsConnected());

    if (dev == nullptr && port.IsConnected() &&
        slot_assigning_devices[port_id] == nullptr)
    {
      dev = devmgr->FindByState(Device::State::kBlank);
      if (dev == nullptr)
      {
        printk("OnPortStatusChanged: No blank device\n");
        return;
      }

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

  Error AddressDevice(Controller* xhc, Device* dev, const Port& port, uint8_t slot_id)
  {
    const DeviceContextIndex dci_ep0{0, false};

    if (auto err = dev->AllocTransferRing(dci_ep0, 32); IsError(err))
    {
      return err;
    }
    const auto tr_addr = reinterpret_cast<uintptr_t>(
        dev->EndpointSet()->TransferRing(dci_ep0)->Buffer());

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

    dev->AssignSlot(slot_id);
    AddressDeviceCommandTRB cmd{dev->InputContext(), slot_id};
    xhc->CommandRing()->Push(cmd);
    xhc->DoorbellRegisterAt(0)->Ring(0);

    return error::kSuccess;
  }

  //void OnConfigureDescriptorReceived(

  void OnDeviceDescriptorReceived(
      Device* dev,
      DeviceContextIndex dci,
      int completion_code,
      int trb_transfer_length,
      TRB* issue_trb)
  {
    printk("OnDeviceDescriptorReceived: slot %u, dci %u, cc %d, len %d, issue trb %s\n",
        dev->SlotID(), dci.value, completion_code, trb_transfer_length,
        kTRBTypeToName[issue_trb->bits.trb_type]);

    // TODO USB クラスドライバ登録票を作る
  }

  void GetDescriptor(Controller* xhc, Device* dev,
                     uint8_t descriptor_type, uint8_t descriptor_index,
                     size_t len, uint8_t* buf,
                     Device::OnTransferredCallbackType* callback)
  {
    SetupStageTRB setup{};
    setup.bits.request_type = 0b10000000;
    setup.bits.request = 6; // get descriptor
    setup.bits.value = (static_cast<uint16_t>(descriptor_type) << 8) | descriptor_index;
    setup.bits.index = 0;
    setup.bits.length = len;
    setup.bits.transfer_type = 3; // IN Data Stage

    DataStageTRB data{};
    data.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(buf);
    data.bits.trb_transfer_length = len;
    data.bits.td_size = 0;
    data.bits.direction = 1; // 1: IN
    data.bits.interrupt_on_completion = 1;

    StatusStageTRB status{};

    const DeviceContextIndex dci_ep0(0, false);

    dev->SetOnTransferred(dci_ep0, callback);

    auto tr = dev->EndpointSet()->TransferRing(dci_ep0);
    tr->Push(setup);
    tr->Push(data);
    tr->Push(status);
    printk("Issuing Control Stage for slot %u, dci %u\n",
        dev->SlotID(), dci_ep0.value);
    xhc->DoorbellRegisterAt(dev->SlotID())->Ring(dci_ep0.value);
  }

  Error ConfigureEndpoints(Controller* xhc, Device* dev)
  {
    uint8_t* buf = AllocArray<uint8_t>(128);
    GetDescriptor(xhc, dev, 1, 0, 128, buf, OnDeviceDescriptorReceived);
    return error::kSuccess;
  }

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

      if (auto err = ConfigureEndpoints(ctx->xhc, dev); IsError(err))
      {
        printk("OnCommandCompleted: ConfigureEndpoints failed: %d\n", err);
      }
    }
  }

  void OnTransferred(const CallbackContext* ctx, const InterruptMessage& msg)
  {
    const auto slot_id = msg.attr3;
    const auto endpoint_id = msg.attr4;

    auto dev = ctx->xhc->DeviceManager()->FindBySlot(slot_id);
    if (dev == nullptr)
    {
      printk("No such device with SlotID = %u\n", slot_id);
      return;
    }

    dev->OnTransferred(DeviceContextIndex(endpoint_id),
                       msg.attr1,
                       msg.attr2,
                       reinterpret_cast<TRB*>(msg.data));
  }

  void PostInterruptHandler(const CallbackContext* ctx, const InterruptMessage& msg)
  {
    switch (msg.type)
    {
    case InterruptMessageType::kXHCITransferEvent:
      OnTransferred(ctx, msg);
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
