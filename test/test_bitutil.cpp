#include <CppUTest/CommandLineTestRunner.h>
#include "bitutil.hpp"

TEST_GROUP(Bitutil) {
    TEST_SETUP()
    {}

    TEST_TEARDOWN()
    {}
};

TEST(Bitutil, bsf)
{
    CHECK_EQUAL(0, bitutil::BitScanForward(0x0001u));
    CHECK_EQUAL(6, bitutil::BitScanForward(0xa5c0u));
    CHECK_EQUAL(63, bitutil::BitScanForward(static_cast<uint64_t>(1u) << 63));

    CHECK_EQUAL(-1, bitutil::BitScanForward(0));
}

TEST(Bitutil, clear_bits)
{
    CHECK_EQUAL(0xdeadbeefdead0000,
                bitutil::ClearBits(0xdeadbeefdeadbeef, 0xffff));
    CHECK_EQUAL(0x0e0dbeefdead0000,
                bitutil::ClearBits(0xdeadbeefdeadbeef, 0xf0f000000000ffff));
}

static_assert(0 == bitutil::BitScanForwardConst(0x0001u));
static_assert(6 == bitutil::BitScanForwardConst(0xa5c0u));
static_assert(63 == bitutil::BitScanForwardConst(static_cast<uint64_t>(1u) << 63));
