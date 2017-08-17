#include "memory.hpp"

#include "bootparam.h"
#include "printk.hpp"

extern BootParam* kernel_boot_param;

namespace bitnos::memory
{
    PageFrame* frame_array;
    size_t frame_array_size; // the number of element
    uint64_t initial_stack_pointer;

    void SetFrameArray(
        PageFrame* frame_array,
        size_t frame_array_size,
        UefiMemoryDescriptorIterator desc_begin,
        UefiMemoryDescriptorIterator desc_end)
    {
        uintptr_t contiguous_free_area_start = 0;
        bool processing_contiguous_free_area = false;
        auto desc_prev = desc_end;

        auto MarkFreeArea = [&](uintptr_t start, uintptr_t end)
        {
            const auto frame_start = (start + kHostPageSize - 1) / kHostPageSize;
            const auto frame_end = end / kHostPageSize;
            for (auto i = frame_start; i < frame_end && i < frame_array_size; ++i)
            {
                frame_array[i].flags.SetUsed(false);
            }
        };

        for (size_t i = 0; i < frame_array_size; ++i)
        {
            frame_array[i].flags.SetUsed(true);
            frame_array[i].flags.SetType(PageFrameFlags::kType2M);
        }

        for (auto desc_it = desc_begin; desc_it != desc_end; ++desc_it)
        {
            if (desc_prev != desc_end)
            {
                if (processing_contiguous_free_area &&
                    !(IsContiguous(desc_prev, desc_it) && IsFree(desc_it)))
                {
                    processing_contiguous_free_area = false;
                    MarkFreeArea(contiguous_free_area_start, GetPhysicalEnd(desc_prev));
                }
            }

            if (IsFree(desc_it))
            {
                if (!processing_contiguous_free_area)
                {
                    processing_contiguous_free_area = true;
                    contiguous_free_area_start = desc_it->PhysicalStart;
                }
            }

            if (desc_it->PhysicalStart <= initial_stack_pointer
                && initial_stack_pointer < GetPhysicalEnd(desc_it))
            {
            }

            desc_prev = desc_it;
        }

        if (processing_contiguous_free_area)
        {
            MarkFreeArea(contiguous_free_area_start, GetPhysicalEnd(desc_prev));
        }
    }

    Error Init()
    {
        const auto desc_size = kernel_boot_param->memory_descriptor_size;
        const auto num_descs = kernel_boot_param->memory_map_size / desc_size;
        const UefiMemoryDescriptorIterator desc_begin{
            reinterpret_cast<uintptr_t>(kernel_boot_param->memory_map),
            desc_size
        };
        const UefiMemoryDescriptorIterator desc_end{
            reinterpret_cast<uintptr_t>(kernel_boot_param->memory_map) + desc_size * desc_size,
            desc_size
        };

        size_t frame_array_index = 0;

        auto last_free_desc = desc_end;
        for (auto desc_it = desc_begin; desc_it != desc_end; ++desc_it)
        {
            if (IsFree(desc_it))
            {
                last_free_desc = desc_it;
            }
        }

        if (last_free_desc == desc_end)
        {
            return errorcode::kNoEnoughMemory;
        }

        // End of physical free memory
        const auto physical_addr_end =
            last_free_desc->PhysicalStart + last_free_desc->NumberOfPages * kUefiPageSize;

        frame_array_size = (physical_addr_end + kHostPageSize - 1) / kHostPageSize;

        // find the first free area which is large enough to contain an array of pages.
        EFI_MEMORY_DESCRIPTOR* desc_for_array = nullptr;
        for (auto it = desc_begin; it != desc_end; ++it)
        {
            if (it->PhysicalStart > 0 && IsFree(it) &&
                sizeof(PageFrame) * frame_array_size <= it->NumberOfPages * kUefiPageSize)
            {
                desc_for_array = &*it;
                break;
            }
        }

        if (desc_for_array == nullptr)
        {
            return errorcode::kNoEnoughMemory;
        }

        const uintptr_t frame_array_start = desc_for_array->PhysicalStart;
        const uintptr_t frame_array_end = frame_array_start + sizeof(PageFrame) * frame_array_size;

        frame_array = reinterpret_cast<PageFrame*>(frame_array_start);
        SetFrameArray(frame_array, frame_array_size, desc_begin, desc_end);
        for (size_t i = frame_array_start / kHostPageSize;
            i < (frame_array_end + kHostPageSize - 1) / kHostPageSize;
            ++i)
        {
            frame_array[i].flags.SetUsed(true);
        }
        return errorcode::kSuccess;
    }
}
