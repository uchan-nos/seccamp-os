#include "desctable.hpp"

#include "bitutil.hpp"

namespace bitnos
{
    namespace
    {
        DescriptorTableRegister MakeDTR(const uint8_t *dtr)
        {
            using T = DescriptorTableRegister;
            return {
                bitutil::ReadByByte<decltype(T::limit)>(&dtr[0]),
                bitutil::ReadByByte<decltype(T::base)>(&dtr[2])
            };
        }

        constexpr size_t kDTRBufSize =
            sizeof(DescriptorTableRegister::limit)
            + sizeof(DescriptorTableRegister::base);
    }

    DescriptorTableRegister GetGDTR()
    {
        uint8_t gdtr[kDTRBufSize];
        __asm__("sgdt %0" : "=m"(gdtr));
        return MakeDTR(gdtr);
    }

    DescriptorTableRegister GetIDTR()
    {
        uint8_t idtr[kDTRBufSize];
        __asm__("sidt %0" : "=m"(idtr));
        return MakeDTR(idtr);
    }

    Error SetIDTEntry(
        DescriptorTableRegister idtr, size_t index,
        uint64_t offset, uint16_t attr, uint16_t selector)
    {
        if (16 * index + 15 > idtr.limit)
        {
            return errorcode::kIndexOutOfRange;
        }

        auto p = reinterpret_cast<uint32_t*>(idtr.base) + 4 * index;
        p[0] = static_cast<uint32_t>(selector) << 16 | (offset & 0xffffu);
        p[1] = (offset & 0xffff0000u) | attr;
        p[2] = offset >> 32;

        return errorcode::kSuccess;
    }

}
