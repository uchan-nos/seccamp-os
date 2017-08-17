#include <CppUTest/CommandLineTestRunner.h>
#include "memory.hpp"

#include "bootparam.h"
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <iostream>

using namespace bitnos::memory;

using namespace std;

BootParam* kernel_boot_param;



TEST_GROUP(PageFrame) {
    TEST_SETUP()
    {
    }

    TEST_TEARDOWN()
    {
    }
};

TEST(PageFrame, flags)
{
    PageFrameFlags f;
    CHECK_FALSE(f.Used());
    CHECK_EQUAL(0, f.Type());

    f.SetUsed(true);
    CHECK_TRUE(f.Used());

    f.SetType(f.kType1G);
    CHECK_EQUAL(2, f.Type());
}

unsigned long long operator"" _MiB(unsigned long long value)
{
    return value * 1024 * 1024;
}

unsigned long long operator"" _KiB(unsigned long long value)
{
    return value * 1024;
}

EFI_MEMORY_DESCRIPTOR MakeDesc(
    EFI_MEMORY_TYPE type, EFI_PHYSICAL_ADDRESS physical_start, EFI_PHYSICAL_ADDRESS physical_end)
{
    const auto num_pages = (physical_end - physical_start) / 4_KiB;
    return { static_cast<UINT32>(type), physical_start, 0, num_pages, 0 };
}

template <size_t N, size_t M>
void SetFrameArray(PageFrame (&frame_array)[N], EFI_MEMORY_DESCRIPTOR (&mem_map)[M])
{
    UefiMemoryDescriptorIterator desc_begin{
        reinterpret_cast<EFI_PHYSICAL_ADDRESS>(mem_map), sizeof(EFI_MEMORY_DESCRIPTOR) };
    UefiMemoryDescriptorIterator desc_end{
        reinterpret_cast<EFI_PHYSICAL_ADDRESS>(mem_map + M), sizeof(EFI_MEMORY_DESCRIPTOR) };
    SetFrameArray(frame_array, N, desc_begin, desc_end);
}

TEST(PageFrame, IsFree)
{
    auto desc = MakeDesc(EfiConventionalMemory, 0_KiB, 4_KiB);

    initial_stack_pointer = 10;
    CHECK_FALSE(IsFree(&desc));

    initial_stack_pointer = 10000;
    CHECK_TRUE(IsFree(&desc));
}

TEST(PageFrame, SetFrameArray1)
{
    PageFrame frame_array[4];
    for (auto& f : frame_array) { f.flags.SetUsed(true); }
    EFI_MEMORY_DESCRIPTOR mem_map[]{
        MakeDesc(EfiConventionalMemory, 0_MiB, 10_MiB),
    };
    initial_stack_pointer = 16_MiB;
    SetFrameArray(frame_array, mem_map);

    CHECK_FALSE(frame_array[0].flags.Used());
    CHECK_FALSE(frame_array[1].flags.Used());
    CHECK_FALSE(frame_array[2].flags.Used());
    CHECK_FALSE(frame_array[3].flags.Used());
}

TEST(PageFrame, SetFrameArray2)
{
    PageFrame frame_array[6];
    EFI_MEMORY_DESCRIPTOR mem_map[]{
        MakeDesc(EfiConventionalMemory, 0_MiB, 1_MiB),
        MakeDesc(EfiConventionalMemory, 3_MiB, 6_MiB),
        MakeDesc(EfiConventionalMemory, 7_MiB, 11_MiB),
    };
    initial_stack_pointer = 16_MiB;
    SetFrameArray(frame_array, mem_map);

    CHECK_TRUE(frame_array[0].flags.Used());
    CHECK_TRUE(frame_array[1].flags.Used());
    CHECK_FALSE(frame_array[2].flags.Used());
    CHECK_TRUE(frame_array[3].flags.Used());
    CHECK_FALSE(frame_array[4].flags.Used());
    CHECK_TRUE(frame_array[5].flags.Used());
}

TEST(PageFrame, SetFrameArray3)
{
    PageFrame frame_array[2];
    EFI_MEMORY_DESCRIPTOR mem_map[]{
        MakeDesc(EfiConventionalMemory, 0_MiB, 1_MiB),
        MakeDesc(EfiReservedMemoryType, 1_MiB, 2_MiB),
        MakeDesc(EfiConventionalMemory, 2_MiB, 3_MiB),
        MakeDesc(EfiBootServicesData, 3_MiB, 4_MiB),
    };

    initial_stack_pointer = 16_MiB;
    SetFrameArray(frame_array, mem_map);

    CHECK_TRUE(frame_array[0].flags.Used());
    CHECK_FALSE(frame_array[1].flags.Used());

    initial_stack_pointer = 4_MiB - 100_KiB;
    SetFrameArray(frame_array, mem_map);

    CHECK_TRUE(frame_array[0].flags.Used());
    CHECK_TRUE(frame_array[1].flags.Used());
}
