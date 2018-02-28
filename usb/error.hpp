#pragma once

/**
 * Error code definitions used by USB and host controller drivers.
 */

namespace usb::error
{
  using Type = int;

  const Type kSuccess = 0;
  const Type kNoEnoughMemory = 1;
  const Type kHostControllerNotHalted = 2;
  const Type kInvalidDeviceId = 3;
  const Type kAlreadyAllocated = 4;
  const Type kInvalidSlotId = 5;
  const Type kInvalidEndpointNumber = 6;
  const Type kTransferRingNotSet = 6;
}

namespace usb
{
  using Error = error::Type;

  constexpr bool IsError(Error code)
  {
    return code != error::kSuccess;
  }

  template <typename T>
  struct WithError
  {
    using ValueType = T;

    ValueType value;
    Error error;
  };
}
