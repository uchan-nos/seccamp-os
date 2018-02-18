#pragma once

#include "usb/malloc.hpp"

namespace usb
{
  enum class InterruptMessageType
  {
    kReserved,
    kXHCIPortStatusChangeEvent
  };

  /**
   * Generic message structure which will be issued by interrupt handlers.
   */
  struct InterruptMessage
  {
    /** @brief Message type.
     */
    InterruptMessageType type;

    /** @brief General purpose 64 bit field.
     *
     * Meaning is dependent on this message's type.
     */
    uint64_t attr;

    /** @brief A pointer to an additional data. Null if no additional data.
     */
    void* data;
  };
}
