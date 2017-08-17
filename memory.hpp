#pragma once

#include <stdint.h>
#include "bitutil.hpp"
#include "errorcode.hpp"

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

namespace bitnos::memory
{
    class PageFrameFlags
    {
        uint32_t flags_;

        const static uint32_t kUsed = 0x00000001u;
        const static uint32_t kType = 0x0000000cu;

        inline unsigned int GetFlag(uint32_t mask) const
        {
            return (flags_ & mask) >> bitutil::BitScanForwardConst(mask);
        }

        inline void SetFlag(uint32_t mask, uint32_t flag)
        {
            flags_ = (flags_ & ~mask) | (flag << bitutil::BitScanForwardConst(mask));
        }

    public:
        const static uint32_t kType4K = 0;
        const static uint32_t kType2M = 1;
        const static uint32_t kType1G = 2;

        PageFrameFlags()
            : flags_(0)
        {}

        bool Used() const { return GetFlag(kUsed) == 1; }
        void SetUsed(bool value) { SetFlag(kUsed, value ? 1 : 0); }
        uint32_t Type() const { return GetFlag(kType); }
        void SetType(uint32_t value) { SetFlag(kType, value); }
    };

    struct PageFrame
    {
        PageFrameFlags flags;
    };

    extern PageFrame* frame_array;
    extern size_t frame_array_size; // the number of element

    const unsigned int kUefiPageSize = 0x1000;
    const unsigned int kHostPageSize = 0x200000;

    struct UefiMemoryDescriptorIterator
    {
        EFI_PHYSICAL_ADDRESS desc;
        UINTN desc_size;

        UefiMemoryDescriptorIterator& operator ++()
        {
            desc += desc_size;
            return *this;
        }

        EFI_MEMORY_DESCRIPTOR& operator *() const
        {
            return *reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(desc);
        }

        EFI_MEMORY_DESCRIPTOR* operator ->() const
        {
            return reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(desc);
        }

        EFI_MEMORY_DESCRIPTOR& operator [](int i) const
        {
            return *reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(desc + i * desc_size);
        }
    };

    inline bool operator ==(const UefiMemoryDescriptorIterator& lhs, const UefiMemoryDescriptorIterator& rhs)
    {
        return lhs.desc == rhs.desc && lhs.desc_size == rhs.desc_size;
    }

    inline bool operator !=(const UefiMemoryDescriptorIterator& lhs, const UefiMemoryDescriptorIterator& rhs)
    {
        return !(lhs == rhs);
    }

    void SetFrameArray(
        PageFrame* frame_array,
        size_t frame_array_size,
        UefiMemoryDescriptorIterator desc_begin,
        UefiMemoryDescriptorIterator desc_end);

    Error Init();
}
