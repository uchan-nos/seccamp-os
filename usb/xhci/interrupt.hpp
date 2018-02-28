#pragma once

#include <stdint.h>

#include "usb/interrupt.hpp"
#include "usb/malloc.hpp"
#include "usb/error.hpp"
#include "usb/xhci/xhci.hpp"

#include "printk.hpp"

namespace usb::xhci
{
  struct CallbackContext
  {
    /** @brief Host controller object.
     */
    Controller* xhc;

    /** @brief A data structure given by the operating system.
     *
     * If the OS needs some information to process a message inside its
     * interrupt handler, this field can be used to store a data structure.
     */
    void* data;

    /** @brief EnqueueMessage is a function to enqueue a message to somewhere,
     * typically main queue of the operating system.
     * It is usually called inside an interrupt handler.
     *
     * @param ctx  A context which will be used duing enqueuing a message.
     * @param msg  A message to be enqueued. Because it may be local object,
     *  this function may NOT leak the pointer to it.
     */
    Error (*EnqueueMessage)(const CallbackContext* ctx, const InterruptMessage* msg);
  };

  /** @brief InterruptHandler is a callback function to handle interrupts to
   * this USB library. It will call ctx->EnqueueMessage with a message.
   */
  void InterruptHandler(const CallbackContext* ctx);

  void OnPortStatusChanged(const CallbackContext* ctx, const InterruptMessage& msg);
  void OnCommandCompleted(const CallbackContext* ctx, const InterruptMessage& msg);
  void PostInterruptHandler(const CallbackContext* ctx, const InterruptMessage& msg);
}
