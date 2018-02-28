#pragma once

/**
 * The memory allocator used in this USB driver.
 */

#include <stddef.h>

namespace usb
{
  /** The size of the memory pool from which this allocater allocates blocks.
   */
  static const size_t kMemoryPoolSize = 4096 * 32;
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

  template <typename T>
  T* AllocArray(
      size_t num_object,
      unsigned int alignment=64u,
      unsigned int boundary=4096u)
  {
    return reinterpret_cast<T*>(
        AllocMem(sizeof(T) * num_object, alignment, boundary));
  }

  template <typename T>
  T* AllocObject(
      unsigned int alignment=64u,
      unsigned int boundary=4096u)
  {
    return AllocArray<T>(1, alignment, boundary);
  }

  void FreeMem(void* p);

  template <typename T>
  void FreeObject(const T* p)
  {}

  template <typename T>
  void FreeArray(const T* p)
  {}

  /** @brief ResetAllocPointer resets the pointer from which memory block will
   * be allocated. This function is for test purpuse.
   */
  void ResetAllocPointer();
}
