#pragma once

#include <stdint.h>

namespace bitnos
{

    template <typename T>
    class MemMapRegister
    {
        volatile T value_;

    public:
        T Read() const
        {
            auto tmp = value_.data;
            return T{tmp};
            //return value_;
        }

        void Write(T value)
        {
            value_.data = value.data;
        }
    };

    template <typename T>
    union DefaultBitmap
    {
        T data;

        DefaultBitmap(T data) : data(data) {}
        operator T() { return data; }
    };

    using MemMapRegister32 = MemMapRegister<DefaultBitmap<uint32_t>>;
    using MemMapRegister64 = MemMapRegister<DefaultBitmap<uint64_t>>;

    class MemMapRegister64Access32
    {
        volatile uint64_t value_;

        uint64_t Read32(int i) const
        {
            auto p = reinterpret_cast<const volatile uint32_t*>(&value_);
            return static_cast<uint64_t>(p[i]);
        }

    public:
        uint64_t Read() const
        {
            return Read32(0) | (Read32(1) << 32);
        }

        void Write(uint64_t value)
        {
            auto p = reinterpret_cast<volatile uint32_t*>(&value_);
            p[0] = value & 0xffffffffu;
            p[1] = value >> 32;
        }
    };

    template <typename T>
    struct BitMaskShift
    {
        T mask, shift;

        constexpr BitMaskShift(T mask)
            : mask(mask), shift(BitScanForwardConst(mask))
        {}

        constexpr BitMaskShift(T mask, T shift)
            : mask(mask), shift(shift)
        {}

        constexpr BitMaskShift<T> operator ~() const
        {
            return BitMaskShift{static_cast<T>(~mask), shift};
        }
    };

    template <typename U, typename T>
    U operator &(U value, BitMaskShift<T> ms)
    {
        return value & static_cast<U>(ms.mask);
    }

    template <typename U, typename T>
    U operator |(U value, BitMaskShift<T> ms)
    {
        return value | static_cast<U>(ms.mask);
    }

    template <typename U, typename T>
    U operator <<(U value, BitMaskShift<T> ms)
    {
        return value << ms.shift;
    }

    template <typename U, typename T>
    U operator >>(U value, BitMaskShift<T> ms)
    {
        return value >> ms.shift;
    }
}
