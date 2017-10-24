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
            return value_;
        }

        void Write(T value)
        {
            value_ = value;
        }
    };

    using MemMapRegister32 = MemMapRegister<uint32_t>;
    using MemMapRegister64 = MemMapRegister<uint64_t>;

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
}
