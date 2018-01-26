#include <CppUTest/CommandLineTestRunner.h>
#include "usb/malloc.hpp"

TEST_GROUP(USBMalloc) {
};

TEST(USBMalloc, AllocMem)
{
  uint8_t* p1 = reinterpret_cast<uint8_t*>(usb::AllocMem(1, 4096, 4096));
  uint8_t* p2 = reinterpret_cast<uint8_t*>(usb::AllocMem(1, 8, 4096));
  uint8_t* p3 = reinterpret_cast<uint8_t*>(usb::AllocMem(1, 64, 4096));
  uint8_t* p4 = reinterpret_cast<uint8_t*>(usb::AllocMem(4000, 64, 4096));
  CHECK_EQUAL(8, p2 - p1);
  CHECK_EQUAL(64, p3 - p1);
  CHECK_EQUAL(4096, p4 - p1);
}

TEST(USBMalloc, AllocMemTooLarge)
{
  auto p = usb::AllocMem(usb::kMemoryPoolSize + 1);
  POINTERS_EQUAL(nullptr, p);
}
