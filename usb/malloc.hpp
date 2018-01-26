#pragma once

#include <stddef.h>

/**
 * The memory allocator used in this USB driver.
 */

namespace usb
{
  /** The size of the memory pool from which this allocater allocates blocks.
   */
  static const size_t kMemoryPoolSize = 4096 * 16;
  /** Allocate a memory block with specified alignment and boundary contraint.
   *
   * @param size  Size of a memory block in bytes to be allocated.
   * @param alignment  Alignment constraint in bytes. It must be the power of 2.
   * @param boundary  Boundary constraint in bytes which allocated memory block
   *  shall not span. 4096 is sufficient for all xHCI implementation.
   *
   * @return A pointer to the allocated memory block.
   */
  void* AllocMem(
      size_t size,
      unsigned int alignment=64u,
      unsigned int boundary=4096u);

  void FreeMem(void* p);
}
