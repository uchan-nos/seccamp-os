/** @file main.cpp is a main file of UEFI bitnos.
 *
 * Coding style basically follows "Google C++ Style Guide".
 * @see https://google.github.io/styleguide/cppguide.html
 */

#include <errno.h>
#undef errno
extern "C" int errno;
int errno;

#include <stddef.h>
#include <stdio.h>

#include "printk.hpp"
#include "asmfunc.h"
#include "bootparam.h"
#include "memory.hpp"
#include "memory_op.hpp"
#include "graphics.hpp"
#include "debug_console.hpp"
#include "desctable.hpp"
#include "queue.hpp"
#include "mutex.hpp"
#include "xhci.hpp"
#include "xhci_tr.hpp"

using namespace bitnos;

extern "C" void __cxa_pure_virtual() {
    while (1);
}

extern "C" int write(int file, char *ptr, int len)
{
    if (file != 1) // support only stdout
    {
        errno = EBADF;
        return -1;
    }
    if (default_debug_console == nullptr)
    {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < len; i++)
    {
        default_debug_console->PutChar(*ptr++);
    }
    return len;
}

SpinLockMutex printk_mutex;

int printk(const char* format, ...)
{
    LockGuard<SpinLockMutex> lock(printk_mutex);

    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    write(1, s, result);
    va_end(ap);

    return result;
}

void print_desc_table(void)
{
    auto gdtr = GetGDTR();
    printk("GDTR: Base %08lx, Limit %x\n", gdtr.base, gdtr.limit);

    for (unsigned int i = 0; i <= (gdtr.limit >> 3); i++) {
        uint32_t *p = (uint32_t*)(gdtr.base + (i << 3));
        uint32_t base = (p[1] & 0xff000000u) | ((p[1] & 0xffu) << 16) | (p[0] >> 16);
        uint32_t limit = (p[1] & 0x000f0000u) | (p[0] & 0x0000ffffu);
        uint8_t attr_g = (p[1] >> 23) & 1;
        uint8_t attr_db = (p[1] >> 22) & 1;
        uint8_t attr_l = (p[1] >> 21) & 1;
        uint8_t attr_avl = (p[1] >> 20) & 1;
        uint8_t attr_p = (p[1] >> 15) & 1;
        uint8_t attr_dpl = (p[1] >> 13) & 3;
        uint8_t attr_s = (p[1] >> 12) & 1;
        uint8_t attr_type = (p[1] >> 8) & 15;
        printk("%3u: Base=%08x Limit=%05x "
            "G=%d D/B=%d L=%d AVL=%d P=%d DPL=%d S=%d Type=%d\n",
            i, base, limit,
            attr_g, attr_db, attr_l, attr_avl, attr_p, attr_dpl, attr_s, attr_type);
    }

    auto idtr = GetIDTR();
    printk("IDTR: Base %08lx, Limit %x\n", idtr.base, idtr.limit);

    for (unsigned int i = 0x20; i <= (idtr.limit >> 4) && i < 0x30; i++) {
        uint32_t *p = (uint32_t*)(idtr.base + (i << 4));
        uint16_t selector = p[0] >> 16;
        uint64_t offset = ((uint64_t)p[2] << 32) | (p[1] & 0xffff0000u) | (p[0] & 0x0000ffffu);
        uint8_t attr_p = (p[1] >> 15) & 1;
        uint8_t attr_dpl = (p[1] >> 13) & 3;
        uint8_t attr_d = (p[1] >> 11) & 1;
        uint8_t attr_type = (p[1] >> 8) & 0x1fu;
        uint8_t attr_ist = p[1] & 7;
        if (attr_type != 15 && attr_type != 14)
        {
            printk("%3u: It is not 64-bit interrupt and trap gates!\n", i);
            break;
        }

        printk("%3u: Selector=%04x Offset=%016lx "
            "P=%d DPL=%d Type=%d(D=%d) IST=%d\n",
            i, selector, offset, attr_p, attr_dpl, attr_type, attr_d, attr_ist);
    }

    uint16_t tr;
    __asm__("str %0" : "=m"(tr));
    printk("%04x\n", tr);
}

#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

void init_pic()
{
    IoOut8(PIC0_IMR,  0xff  ); /* 全ての割り込みを受け付けない */
    IoOut8(PIC1_IMR,  0xff  ); /* 全ての割り込みを受け付けない */

    IoOut8(PIC0_ICW1, 0x11  ); /* エッジトリガモード */
    IoOut8(PIC0_ICW2, 0x20  ); /* IRQ0-7は、INT20-27で受ける */
    IoOut8(PIC0_ICW3, 1 << 2); /* PIC1はIRQ2にて接続 */
    IoOut8(PIC0_ICW4, 0x01  ); /* ノンバッファモード */

    IoOut8(PIC1_ICW1, 0x11  ); /* エッジトリガモード */
    IoOut8(PIC1_ICW2, 0x28  ); /* IRQ8-15は、INT28-2fで受ける */
    IoOut8(PIC1_ICW3, 2     ); /* PIC1はIRQ2にて接続 */
    IoOut8(PIC1_ICW4, 0x01  ); /* ノンバッファモード */

    IoOut8(PIC0_IMR,  0xf9  ); /* 11111011 PIC1以外は全て禁止 */
    IoOut8(PIC1_IMR,  0xff  ); /* 11111111 全ての割り込みを受け付けない */
}

#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064
#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void)
{
    /* キーボードコントローラがデータ送信可能になるのを待つ */
    for (;;) {
        if ((IoIn8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

void init_keyboard()
{
    /* キーボードコントローラの初期化 */
    wait_KBC_sendready();
    IoOut8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    IoOut8(PORT_KEYDAT, KBC_MODE);
}

ArrayQueue<uint8_t, 16> keydat;
SpinLockMutex keydat_mutex;
volatile unsigned int num_lostkey = 0;


extern "C" void Inthandler00(void)
{
    printk("#DE\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler01(void)
{
    printk("#DB\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler03(void)
{
    printk("#BP\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler04(void)
{
    printk("#OF\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler05(void)
{
    printk("#BR\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler06(void)
{
    printk("#UD\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler07(void)
{
    printk("#NM\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler08(void)
{
    printk("#DF\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler0a(void)
{
    printk("#TS\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler0b(void)
{
    printk("#NP\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler0c(void)
{
    printk("#SS\n");
    for (;;) asm("hlt");
}

__attribute__ ((no_caller_saved_registers))
extern "C" void Inthandler0d(
    uint64_t error_code, uint64_t rip, uint64_t cs, uint64_t rflags)
{
    printk("#GP Error Code %016lx, RIP %016lx, CS %04lx, RFLAGS %016lx\n",
        error_code, rip, cs, rflags);
    for (;;) asm("hlt");
}

extern "C" void Inthandler0e(void)
{
    printk("#PF\n");
    for (;;) asm("hlt");
}

extern "C" void Inthandler21(void)
{
    IoOut8(PIC0_OCW2, 0x61);	/* IRQ-01受付完了をPICに通知 */
    auto dat = IoIn8(PORT_KEYDAT);

    if (keydat_mutex.TryLock())
    {
        keydat.Push(dat);
        keydat_mutex.Unlock();
    }
    else
    {
        ++num_lostkey;
    }
}

SpinLockMutex usb_key_received_mutex;
volatile bool usb_key_received = false;

void ShowEventTRB(xhci::TRB& trb);

unsigned long long inthandler_40_called = 0;
__attribute__ ((no_caller_saved_registers))
extern "C" void Inthandler40(void)
{
    inthandler_40_called++;
    //if (printk_mutex.TryLock())
    //{
    //    //printk("I40\n");
    //    printk_mutex.Unlock();
    //}

    extern xhci::Controller* xhc;
    while (xhc->EventRingManager().HasFront())
    {
        auto trb = xhc->EventRingManager().Front();
        xhc->EventRingManager().Pop();
        //ShowEventTRB(trb);

        if (trb.bits.trb_type == 32) // Transfer Event
        {
            auto& transfer_event = reinterpret_cast<xhci::TransferEventTRB&>(trb);
            auto issue_trb = reinterpret_cast<xhci::NormalTRB*>(transfer_event.bits.trb_pointer);
            if (issue_trb->bits.trb_type == 1) // Normal TRB
            {
                uint8_t* buf = reinterpret_cast<uint8_t*>(issue_trb->bits.data_buffer_pointer);
                if (keydat_mutex.TryLock())
                {
                    keydat.Push(buf[2]);
                    keydat_mutex.Unlock();
                }
            }
        }
    }

    *reinterpret_cast<volatile uint32_t*>(0xfee000b0) = 0;
}

static char keytable_normal[0x80] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
};

static char keytable_shifted[0x80] = {
    0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
};

const char keycode_map[256] = {
       0,    0,    0,    0,  'a',  'b',  'c',  'd', // 0
     'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l', // 8
     'm',  'n',  'o',  'p',  'q',  'r',  's',  't', // 16
     'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2', // 24
     '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0', // 32
    '\n', '\b', 0x08, '\t',  ' ',  '-',  '=',  '[', // 40
     ']', '\\',  '#',  ';', '\'',  '`',  ',',  '.', // 48
     '/',    0,    0,    0,    0,    0,    0,    0, // 56
       0,    0,    0,    0,    0,    0,    0,    0, // 64
       0,    0,    0,    0,    0,    0,    0,    0, // 72
       0,    0,    0,    0,  '/',  '*',  '-',  '+', // 80
    '\n',  '1',  '2',  '3',  '4',  '5',  '6',  '7', // 88
     '8',  '9',  '0',  '.', '\\',    0,    0,  '=', // 96
};
const char keycode_map_shifted[256] = {
       0,    0,    0,    0,  'A',  'B',  'C',  'D', // 0
     'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L', // 8
     'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T', // 16
     'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '@', // 24
     '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')', // 32
    '\n', '\b', 0x08, '\t',  ' ',  '_',  '+',  '{', // 40
     '}',  '|',  '~',  ':',  '"',  '~',  '<',  '>', // 48
     '?',    0,    0,    0,    0,    0,    0,    0, // 56
       0,    0,    0,    0,    0,    0,    0,    0, // 64
       0,    0,    0,    0,    0,    0,    0,    0, // 72
       0,    0,    0,    0,  '/',  '*',  '-',  '+', // 80
    '\n',  '1',  '2',  '3',  '4',  '5',  '6',  '7', // 88
     '8',  '9',  '0',  '.', '\\',    0,    0,  '=', // 96
};

BootParam* kernel_boot_param;

extern "C" unsigned long MyMain(struct BootParam *param)
{
    asm("movq %%rsp, %0" : "=m"(memory::initial_stack_pointer));

    uint64_t tmp;
    asm("movq %%cr0, %0" : "=r"(tmp));
    tmp &= ~(1lu << 2);
    tmp |= (1lu << 1);
    asm("movq %0, %%cr0" : : "r"(tmp));
    asm("movq %%cr4, %0" : "=r"(tmp));
    tmp |= (1lu << 9) | (1lu << 10);
    asm("movq %0, %%cr4" : : "r"(tmp));

    kernel_boot_param = param;

    struct GraphicMode *mode = param->graphic_mode;
    graphics::PixelWriter *writer = nullptr;
    uint8_t writer_buf[256];
    switch (mode->pixel_format) {
    case kPixelRedGreenBlueReserved8BitPerColor:
        writer = new(writer_buf) graphics::PixelWriterRedGreenBlueReserved8BitPerColor(mode);
        break;
    case kPixelBlueGreenRedReserved8BitPerColor:
        writer = new(writer_buf) graphics::PixelWriterBlueGreenRedReserved8BitPerColor(mode);
        break;
    defualt:
        break;
    }

    if (writer == nullptr) {
        return 0;
    }
    DebugConsole cons(
        *writer,
        {mode->horizontal_resolution / 8, mode->vertical_resolution / 16}
        );
    default_debug_console = &cons;

    for (int y = 0; y < mode->vertical_resolution; y++) {
        for (int x = 0; x < mode->horizontal_resolution; x++) {
            writer->Write({x, y}, {255, 255, 255, 0});
        }
    }
    /*
    writer->DrawRect(
        {0, 0},
        {mode->horizontal_resolution, mode->vertical_resolution},
        {255, 255, 255, 0});
        */

    printk("initial stack pointer: %016lx\n", memory::initial_stack_pointer);

    auto idtr = GetIDTR();
    auto err = SetIDTEntry(
        idtr,
        0x21,
        reinterpret_cast<uint64_t>(AsmInthandler21),
        MakeIDTAttr(1, 0, 14, 0),
        0x38);

    if (IsError(err))
    {
        printk("SetIDTEntry: %d\n", err);
    }

#define SET_IDT_ENTRY(num) \
    SetIDTEntry( \
        idtr, \
        0x ## num, \
        reinterpret_cast<uint64_t>(AsmInthandler ## num), \
        MakeIDTAttr(1, 0, 14, 0), \
        0x38);

    SET_IDT_ENTRY(00);
    SET_IDT_ENTRY(01);
    SET_IDT_ENTRY(03);
    SET_IDT_ENTRY(04);
    SET_IDT_ENTRY(05);
    SET_IDT_ENTRY(06);
    SET_IDT_ENTRY(07);
    SET_IDT_ENTRY(08);
    SET_IDT_ENTRY(0a);
    SET_IDT_ENTRY(0b);
    SET_IDT_ENTRY(0c);
    SET_IDT_ENTRY(0d);
    SET_IDT_ENTRY(0e);

    // MSI
    SET_IDT_ENTRY(40);

#undef SET_IDT_ENTRY

    DebugShell shell(cons);

    //init_pic();
    //init_keyboard();

    memory::Init();

    __asm__("sti");
    const char* auto_cmd = "xhci\n";
    for (int i = 0; auto_cmd[i]; ++i)
    {
        shell.PutChar(auto_cmd[i]);
    }

    /*
    printk("printing frame array: %016lx\n", reinterpret_cast<uintptr_t>(memory::frame_array));
    for (size_t i = 0; i < memory::frame_array_size; ++i)
    {
        printk("%3lu: %016lx Type=%u Free=%u\n",
            i, memory::kHostPageSize * i,
            memory::frame_array[i].flags.Type(),
            memory::frame_array[i].flags.Used() ? 0 : 1);
    }
    */

    bool shifted = false;

    extern xhci::transferring::Manager* tr_mgr_for_keyboard;
    uint8_t buf[128];
    memset(buf, 0, sizeof(buf));
    xhci::NormalTRB normal{};
    normal.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(buf);
    normal.bits.trb_transfer_length = 8;
    normal.bits.interrupt_on_completion = 1;
    tr_mgr_for_keyboard->Push(normal);

    for (;;) {
        {
            keydat_mutex.Lock();
            if (keydat.Count() == 0)
            {
                keydat_mutex.Unlock();
                __asm__("hlt");
                continue;
            }

            auto key = keydat.Front();
            keydat.Pop();
            keydat_mutex.Unlock();
            //printk("%2x (lost keys %u)\n", key, num_lostkey);
            /*
            if (key < 0x80)
            {
                // press
                if (key == 0x2a || key == 0x36)
                {
                    shifted = true;
                }
                else
                {
                    auto ascii = shifted ? keytable_shifted[key] : keytable_normal[key];
                    if (!shifted && 'A' <= ascii && ascii <= 'Z')
                    {
                        ascii += 'a' - 'A';
                    }
                    if (ascii)
                    {
                        shell.PutChar(ascii);
                    }
                }
            }
            else
            {
                key &= 0x7fu;
                if (key == 0x2a || key == 0x36)
                {
                    shifted = false;
                }
            }
            */

            memset(buf, 0, sizeof(buf));
            xhci::NormalTRB normal{};
            normal.bits.data_buffer_pointer = reinterpret_cast<uint64_t>(buf);
            normal.bits.trb_transfer_length = 8;
            normal.bits.interrupt_on_completion = 1;
            tr_mgr_for_keyboard->Push(normal);

            char ascii = keycode_map[key];
            if (ascii)
            {
                shell.PutChar(ascii);
            }
        }
    }

    for (;;) {
        __asm__("hlt\t\n");
    }

    return 0;
}
