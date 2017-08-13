#ifndef BOOTPARAM_H_
#define BOOTPARAM_H_

#include <stdint.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

enum GraphicPixelFormat {
    kPixelRedGreenBlueReserved8BitPerColor,
    kPixelBlueGreenRedReserved8BitPerColor,
    kPixelBitMask,
    kPixelBltOnly,
    kPixelFormatMax
};

struct GraphicPixelBitmask {
    uint32_t RedMask, GreenMask, BlueMask, ReservedMask;
};

struct GraphicMode {
    uintptr_t frame_buffer_base;
    unsigned int frame_buffer_size;
    uint32_t horizontal_resolution, vertical_resolution;
    enum GraphicPixelFormat pixel_format;
    struct GraphicPixelBitmask pixel_information;
    uint32_t pixels_per_scan_line;
};

struct BootParam {
    EFI_SYSTEM_TABLE *efi_system_table;
    EFI_MEMORY_DESCRIPTOR *memory_map;
    uint64_t memory_map_size;
    uint64_t memory_descriptor_size;
    struct GraphicMode *graphic_mode;
};

#endif // BOOTPARAM_H_
