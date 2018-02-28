#pragma once

#include "usb/malloc.hpp"

namespace usb
{
  enum class InterruptMessageType
  {
    kReserved,
    kXHCITransferEvent,
    kXHCICommandCompletionEvent,
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

    /** @brief General purpose 32 bit field.
     *
     * Meaning is dependent on this message's type.
     */
    uint32_t attr1;
    uint32_t attr2;
    uint32_t attr3;
    uint32_t attr4;

    /** @brief A pointer to an additional data. Null if no additional data.
     */
    void* data;
  };
}
