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
  const Type kTransferRingNotSet = 7;
  const Type kInvalidDescriptor = 8;
  const Type kTooLongDescriptor = 9;
  const Type kNoSuitableEndpoint = 10;
  const Type kUnkwonClass = 11;
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
