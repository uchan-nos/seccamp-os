#ifndef DESCTABLE_HPP_
#define DESCTABLE_HPP_

#include <stddef.h>
#include <stdint.h>
#include "errorcode.hpp"

namespace bitnos
{

    struct DescriptorTableRegister
    {
        uint16_t limit;
        uint64_t base;
    };

    DescriptorTableRegister GetGDTR();
    DescriptorTableRegister GetIDTR();

    constexpr uint16_t MakeIDTAttr(
        uint8_t p, uint8_t dpl, uint8_t type, uint8_t ist)
    {
        return static_cast<uint16_t>(p) << 15
            | static_cast<uint16_t>(dpl) << 13
            | static_cast<uint16_t>(type) << 8
            | ist;
    }

    Error SetIDTEntry(
        DescriptorTableRegister idtr, size_t index,
        uint64_t offset, uint16_t attr, uint16_t selector);

}

#endif // DESCTABLE_HPP_
