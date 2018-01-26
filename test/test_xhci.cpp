#include <CppUTest/CommandLineTestRunner.h>
#include "xhci.hpp"
#include <string.h>

TEST_GROUP(SlotContext) {
    bitnos::xhci::SlotContext *context;

    TEST_SETUP()
    {
        context = new bitnos::xhci::SlotContext;
        memset(context, 0, sizeof(bitnos::xhci::SlotContext));
    }

    TEST_TEARDOWN()
    {
        delete context;
    }
};

TEST(SlotContext, endianness)
{
    // test bit-field endianness
    context->bits.route_string = 0xdeadb;
    context->bits.context_entries = 0x16;
    context->bits.interrupter_target = 0x15a;
    context->bits.usb_device_address = 0xa5;

    // 0x16 (5 bits) = 0b10110 -> 0xb0
    CHECK_EQUAL(0xb00deadb, context->dwords[0]);
    CHECK_EQUAL(0x00000000, context->dwords[1]);
    // 0x15a (10 bits) = 0b0101011010 -> 0x568
    CHECK_EQUAL(0x56800000, context->dwords[2]);
    CHECK_EQUAL(0x000000a5, context->dwords[3]);
}
