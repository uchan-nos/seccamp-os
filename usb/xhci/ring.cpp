#include "usb/xhci/ring.hpp"

namespace usb::xhci
{
  Error RegisterCommandRing(Ring* ring, MemMapRegister<CRCR_Bitmap>* crcr)
  {
    CRCR_Bitmap value{};
    value.SetPointer(reinterpret_cast<uint64_t>(ring->Buffer()));
    crcr->Write(value);
    return error::kSuccess;
  }
}
