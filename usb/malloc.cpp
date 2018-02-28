#include "usb/malloc.hpp"

#include <stdint.h>

namespace
{
  template <typename T>
  T Ceil(T value, unsigned int alignment)
  {
    return (value + alignment - 1) & ~static_cast<T>(alignment - 1);
  }
}

namespace usb
{
  alignas(64) uint8_t memory_pool[kMemoryPoolSize];
  uintptr_t alloc_ptr = reinterpret_cast<uintptr_t>(memory_pool);

  /** Very simple memory allocator implementation.
   *
   * This allocator just allocates memory blocks.
   * Allocated blocks are not free-able.
   */
  void* AllocMem(
      size_t size,
      unsigned int alignment,
      unsigned int boundary)
  {
    alloc_ptr = Ceil(alloc_ptr, alignment);
    auto next_boundary = Ceil(alloc_ptr, boundary);
    if (next_boundary < alloc_ptr + size)
    {
      alloc_ptr = next_boundary;
    }

    if (reinterpret_cast<uintptr_t>(memory_pool) + kMemoryPoolSize
        < alloc_ptr + size)
    {
      return nullptr;
    }

    auto p = alloc_ptr;
    alloc_ptr += size;
    return reinterpret_cast<void*>(p);
  }

  void FreeMem(void* p) {}

  void ResetAllocPointer()
  {
    alloc_ptr = reinterpret_cast<uintptr_t>(memory_pool);
  }
}
