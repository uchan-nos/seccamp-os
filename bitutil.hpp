#ifndef BITUTIL_HPP_
#define BITUTIL_HPP_

/** @file bitutil.hpp provides utilities for bit and byte operations.
 */

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

namespace bitutil
{
    inline int BitScanForward(uint64_t value)
    {
        if (value == 0)
        {
            return -1;
        }

        int64_t result;
        __asm__("bsfq %1, %0  \n\t"
                : "=r"(result)
                : "m"(value)
                );
        return result;
    }

    template <typename T>
    constexpr int BitScanForwardConst(T value)
    {
        int i = 0;
        for (; i < sizeof(T) * CHAR_BIT; ++i)
        {
            if (value & (static_cast<T>(1) << i))
            {
                return i;
            }
        }
        return -1;
    }

    /** ClearBits clears the specified bits of value and returns the result.
     *
     * example: ClearBits(0xdeadbeaf, 0xf0f0) == 0xdead0e0f
     *
     * @param value  Integral value.
     * @param bits_cleared  Bit mask which specify bits to be cleared.
     *
     * @return Masked value.
     */
    template <typename T, typename U>
    constexpr T ClearBits(T value, U bits_cleared)
    {
        return value & ~static_cast<T>(bits_cleared);
    }

    template <typename T, typename U>
    constexpr T GetValueWithMask(T value, U mask)
    {
        return (value & mask) >> BitScanForwardConst(mask);
    }

    /** ReadByByte reads/writes a multi-bytes object from/to a byte array.
     */
    template <typename T>
    T ReadByByte(const void* buf)
    {
        auto p = reinterpret_cast<const uint8_t*>(buf);
        T result = 0;
        for (size_t i = 0; i < sizeof(T); i++)
        {
            result <<= 8;
            result |= p[sizeof(T) - 1 - i];
        }
        return result;
    }

    inline uint32_t Read32(uintptr_t addr)
    {
        return *reinterpret_cast<uint32_t*>(addr);
    }

    inline uint64_t Read64(uintptr_t addr)
    {
        return *reinterpret_cast<uint64_t*>(addr);
    }

    inline uint64_t Read64By32(uintptr_t addr)
    {
        return static_cast<uint64_t>(Read32(addr))
            | static_cast<uint64_t>(Read32(addr + 4));
    }
}

#endif // BITUTIL_HPP_
