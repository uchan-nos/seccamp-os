#include <CppUTest/CommandLineTestRunner.h>
#include "usb/register.hpp"

#include <stdint.h>

TEST_GROUP(USBRegister) {
};

TEST(USBRegister, MemMapRegister)
{
  union RegBitmap
  {
    uint32_t data[5];
    struct
    {
      uint32_t field1 : 16;
      uint32_t field2 : 16;
      uint8_t byte1;
      uint8_t byte2;
      uint16_t word;
      uint32_t dword;
      uint64_t qword;
    } __attribute__((packed)) bits;
  } __attribute__((packed));

  uint32_t reg_raw[5] = {
    0x01020304u,
    0x11121314u,
    0x21222324u,
    0x31323334u,
    0x41424344u
  };

  CHECK_EQUAL(4 * 5, sizeof(RegBitmap));
  CHECK_EQUAL(4 * 5, sizeof(usb::MemMapRegister<RegBitmap>));

  auto reg = reinterpret_cast<usb::MemMapRegister<RegBitmap>*>(&reg_raw);

  CHECK_EQUAL(0x0304u, reg->Read().bits.field1);
  CHECK_EQUAL(0x0102u, reg->Read().bits.field2);
  CHECK_EQUAL(0x14u, reg->Read().bits.byte1);
  CHECK_EQUAL(0x13u, reg->Read().bits.byte2);
  CHECK_EQUAL(0x1112u, reg->Read().bits.word);
  CHECK_EQUAL(0x21222324u, reg->Read().bits.dword);
  CHECK_EQUAL(0x4142434431323334lu, reg->Read().bits.qword);

  auto reg_value = reg->Read();

  reg_value.bits.field1 = 0x5a5au;
  reg->Write(reg_value);
  CHECK_EQUAL(0x01025a5au, reg_raw[0]);

  reg_value.bits.word = 0xa5a5u;
  reg->Write(reg_value);
  CHECK_EQUAL(0xa5a51314u, reg_raw[1]);

  reg_value.bits.dword &= 0xffffff00u;
  reg_value.bits.dword |= 0x00ff0000u;
  reg->Write(reg_value);

  CHECK_EQUAL(0x21ff2300u, reg_raw[2]);
}
