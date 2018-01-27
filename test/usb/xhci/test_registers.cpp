#include <CppUTest/CommandLineTestRunner.h>
#include "usb/xhci/registers.hpp"

TEST_GROUP(XHCIRegisters) {
};

TEST(XHCIRegisters, CapabilityRegisters)
{
  uint32_t reg_raw[] = {
    0x01000040u, // HCIVERSION, CAPLENGTH
    0x20010f10u, // HCSPARAMS1
    0x00000000u, // HCSPARAMS2
    0x00000000u, // HCSPARAMS3
    0x00000000u, // HCCPARAMS1
    0x01000000u, // DBOFF
    0x02000000u, // RTSOFF
    0x00000001u, // HCCPARAMS2
  };

  auto cap_regs = reinterpret_cast<usb::xhci::CapabilityRegisters*>(reg_raw);

  CHECK_EQUAL(0x40, cap_regs->CAPLENGTH.Read());
  CHECK_EQUAL(0x0100, cap_regs->HCIVERSION.Read());
  CHECK_EQUAL(0x10, cap_regs->HCSPARAMS1.Read().bits.max_device_slots);
  CHECK_EQUAL(0x10f, cap_regs->HCSPARAMS1.Read().bits.max_interrupters);
  CHECK_EQUAL(0x20, cap_regs->HCSPARAMS1.Read().bits.max_ports);
  CHECK_EQUAL(0x01000000u, cap_regs->DBOFF.Read().Offset());
  CHECK_EQUAL(0x02000000u, cap_regs->RTSOFF.Read().Offset());
  CHECK_EQUAL(true, cap_regs->HCCPARAMS2.Read().bits.u3_entry_capability);
}

TEST(XHCIRegisters, OperationalRegisters)
{
  uint32_t reg_raw[0x3c / sizeof(uint32_t)] = {};
  auto op_regs = reinterpret_cast<usb::xhci::OperationalRegisters*>(reg_raw);

  usb::xhci::CRCR_Bitmap crcr{};
  crcr.SetPointer(0xa000);
  crcr.bits.ring_cycle_state = 1;
  op_regs->CRCR.Write(crcr);
  CHECK_EQUAL(0x0000a001u, reg_raw[6]);
  CHECK_EQUAL(0x280u, op_regs->CRCR.Read().bits.command_ring_pointer);
  CHECK_EQUAL(0xa000u, op_regs->CRCR.Read().Pointer());

  usb::xhci::DCBAAP_Bitmap dcbaap{};
  dcbaap.SetPointer(0x5000);
  op_regs->DCBAAP.Write(dcbaap);
  CHECK_EQUAL(0x00005000u, reg_raw[12]);
  CHECK_EQUAL(0x5000u, op_regs->DCBAAP.Read().Pointer());
}

TEST(XHCIRegisters, PortRegisterArray)
{
  uint32_t reg_raw[8] = {};
  usb::ArrayWrapper<usb::xhci::PortRegisterSet> port_reg_array{
    reinterpret_cast<uintptr_t>(reg_raw), // base addr
    2 // size
  };

  CHECK_EQUAL(2, port_reg_array.Size());
  POINTERS_EQUAL(&reg_raw[0], &port_reg_array[0]);
  POINTERS_EQUAL(&reg_raw[4], &port_reg_array[1]);

  auto p = &port_reg_array[1];
  auto portsc = p->PORTSC.Read();
  portsc.bits.port_enabled_disabled = true;
  p->PORTSC.Write(portsc);

  CHECK_EQUAL(0x00000002u, reg_raw[4]);
}

TEST(XHCIRegisters, InterrupterRegisterArray)
{
  uint32_t reg_raw[8] = {};
  auto int_reg_set =
    reinterpret_cast<usb::xhci::InterrupterRegisterSet*>(reg_raw);

  usb::xhci::ERSTBA_Bitmap erstba{};
  erstba.SetPointer(0x2000);
  int_reg_set->ERSTBA.Write(erstba);
  CHECK_EQUAL(0x00002000u, reg_raw[4]);

  usb::xhci::ERDP_Bitmap erdp{};
  erdp.SetPointer(0x3000);
  erdp.bits.event_handler_busy = 1;
  int_reg_set->ERDP.Write(erdp);
  CHECK_EQUAL(0x00003008u, reg_raw[6]);
}
