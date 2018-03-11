#include "usb/malloc.hpp"
#include "usb/xhci/device.hpp"
#include "usb/xhci/ring.hpp"

#include <CppUTest/CommandLineTestRunner.h>

#if CPPUTEST_USE_NEW_MACROS
#undef new
#endif

inline SimpleString StringFrom(enum usb::xhci::Device::State state)
{
  return StringFrom(static_cast<int>(state));
}

namespace
{
  bool on_transferred_callback_called = false;

  void OnTransferredCallback(
      usb::xhci::Device* dev,
      usb::xhci::DeviceContextIndex dci,
      int completion_code,
      int trb_transfer_length,
      usb::xhci::TRB* issue_trb)
  {
    on_transferred_callback_called = true;
  }

  /*
  struct FakeXHCIController : public usb::xhci::Controller
  {
    FakeXHCIController()
    {}

    usb::Error Initialize() { return usb::error::kSuccess; }
    usb::xhci::Ring* CommandRing() { return nullptr; }
    usb::xhci::EventRing* PrimaryEventRing() { return nullptr; }
    void RingDoorbell(uint8_t index, uint8_t target, uint16_t stream_id) {};

    usb::xhci::Port PortAt(uint8_t port_num)
    {
      return usb::xhci::Port{*this, 0, nullptr};
    }

    uint8_t MaxPorts() const { return 255; }
    usb::xhci::DeviceManager* DeviceManager() { return nullptr; }
  };
  */
}

TEST_GROUP(XHCIEndpointSet)
{
  //FakeXHCIController xhc;
  usb::xhci::Ring rings[31];
  usb::xhci::DoorbellRegister dbreg;
  usb::xhci::EndpointSet* epset;

  usb::xhci::Ring* EnableTransferRing(usb::xhci::DeviceContextIndex dci)
  {
    rings[dci.value - 1].Initialize(8);
    epset->SetTransferRing(dci, &rings[dci.value - 1]);
    return &rings[dci.value - 1];
  }

  template <typename TRBType>
  TRBType* CheckTRB(usb::xhci::TRB* trb)
  {
    CHECK_TRUE(trb->bits.cycle_bit);
    auto trb_ = usb::xhci::TRBDynamicCast<TRBType>(trb);
    CHECK_TRUE(trb_);
    return trb_;
  }

  void CheckSetupStageTRB(usb::xhci::SetupStageTRB* trb,
                          uint8_t request_type, uint8_t request,
                          uint16_t value, uint16_t index, uint16_t length)
  {
    CHECK_EQUAL(request_type, trb->bits.request_type);
    CHECK_EQUAL(request, trb->bits.request);
    CHECK_EQUAL(value, trb->bits.value);
    CHECK_EQUAL(index, trb->bits.index);
    CHECK_EQUAL(length, trb->bits.length);
  }

  TEST_SETUP()
  {
    usb::ResetAllocPointer();
    rings[0].Initialize(8); // default control pipe
    epset = new usb::xhci::EndpointSet{&rings[0], &dbreg};
  }

  TEST_TEARDOWN()
  {
    delete epset;
  }
};

TEST(XHCIEndpointSet, ControlOut)
{
  char buf[16];
  auto tr = EnableTransferRing(usb::xhci::DeviceContextIndex(0, true));
  auto trb = tr->Buffer();

  CHECK_EQUAL(0, trb[0].bits.cycle_bit);

  CHECK_EQUAL(usb::error::kSuccess, epset->ControlOut(0, 0x0102030405060708ul, buf, 16));

  auto setup = CheckTRB<usb::xhci::SetupStageTRB>(&trb[0]);
  CheckSetupStageTRB(setup, 0x8, 0x7, 0x506, 0x304, 0x102);
  CHECK_EQUAL(2, setup->bits.transfer_type);

  auto data = CheckTRB<usb::xhci::DataStageTRB>(&trb[1]);
  CHECK_EQUAL(reinterpret_cast<uint64_t>(buf), data->bits.data_buffer_pointer);
  CHECK_FALSE(data->bits.direction);

  auto status = CheckTRB<usb::xhci::StatusStageTRB>(&trb[2]);
  CHECK_TRUE(status->bits.direction);
  CHECK_TRUE(status->bits.interrupt_on_completion);
}

TEST(XHCIEndpointSet, ControlOut_NoData)
{
  auto tr = EnableTransferRing(usb::xhci::DeviceContextIndex(0, true));
  auto trb = tr->Buffer();

  CHECK_EQUAL(0, trb[0].bits.cycle_bit);

  CHECK_EQUAL(usb::error::kSuccess, epset->ControlOut(0, 0, nullptr, 0));

  auto setup = CheckTRB<usb::xhci::SetupStageTRB>(&trb[0]);
  CHECK_EQUAL(0, setup->bits.transfer_type);

  auto status = CheckTRB<usb::xhci::StatusStageTRB>(&trb[1]);
  CHECK_TRUE(status->bits.direction);
}

TEST(XHCIEndpointSet, ControlIn)
{
  char buf[16];
  auto tr = EnableTransferRing(usb::xhci::DeviceContextIndex(0, true));
  auto trb = tr->Buffer();

  CHECK_EQUAL(usb::error::kSuccess, epset->ControlIn(0, 0x0123456789abcdeful, buf, 16));

  auto setup = CheckTRB<usb::xhci::SetupStageTRB>(&trb[0]);
  CheckSetupStageTRB(setup, 0xef, 0xcd, 0x89ab, 0x4567, 0x0123);
  CHECK_EQUAL(3, setup->bits.transfer_type);

  auto data = CheckTRB<usb::xhci::DataStageTRB>(&trb[1]);
  CHECK_EQUAL(reinterpret_cast<uint64_t>(buf), data->bits.data_buffer_pointer);
  CHECK_TRUE(data->bits.direction);
  CHECK_TRUE(data->bits.interrupt_on_completion);

  auto status = CheckTRB<usb::xhci::StatusStageTRB>(&trb[2]);
  CHECK_FALSE(status->bits.direction);
}

TEST(XHCIEndpointSet, TransferRing)
{
  const auto dci = usb::xhci::DeviceContextIndex(0, false);
  CHECK_TRUE(epset->TransferRing(dci));
}

TEST(XHCIEndpointSet, SetTransferRing)
{
  auto tr = usb::AllocObject<usb::xhci::Ring>();
  tr->Initialize(8);

  const auto dci = usb::xhci::DeviceContextIndex(1, false);
  CHECK_FALSE(epset->TransferRing(dci));
  epset->SetTransferRing(dci, tr);
  CHECK_TRUE(epset->TransferRing(dci));
}

TEST_GROUP(XHCIDevice)
{
  usb::xhci::Device* dev;

  TEST_SETUP()
  {
    dev = usb::AllocObject<usb::xhci::Device>();
    dev->Initialize();
  }

  TEST_TEARDOWN()
  {
  }
};

TEST(XHCIDevice, StateTransition)
{
  CHECK_EQUAL(usb::xhci::Device::State::kBlank, dev->State());
  dev->SelectForSlotAssignment();
  CHECK_EQUAL(usb::xhci::Device::State::kSlotAssigning, dev->State());
  dev->AssignSlot(2);
  CHECK_EQUAL(usb::xhci::Device::State::kSlotAssigned, dev->State());
}
