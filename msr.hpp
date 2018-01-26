#pragma once

#include <stdint.h>

namespace bitnos::msr
{
    const uint32_t IA32_APIC_BASE = 0x1b;

    uint64_t Read(uint32_t msr_id)
    {
        uint32_t msr_low, msr_high;
        __asm__ volatile ("rdmsr"
            : "=a"(msr_low), "=d"(msr_high)
            : "c"(msr_id)
            );
        return static_cast<uint64_t>(msr_high) << 32 | msr_low;
    }
}
