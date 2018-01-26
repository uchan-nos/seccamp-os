#include <CppUTest/CommandLineTestRunner.h>
#include "queue.hpp"

using namespace bitnos;

TEST_GROUP(QueueBasic) {
    bitnos::ArrayQueue<int, 5> q;

    TEST_SETUP()
    {}

    TEST_TEARDOWN()
    {}
};

TEST(QueueBasic, Push)
{
    CHECK_EQUAL(0, q.Count());
    auto err = q.Push(42);
    CHECK_EQUAL(errorcode::kSuccess, err);
    CHECK_EQUAL(1, q.Count());
    CHECK_EQUAL(42, q.Front());

    for (int i = 0; i < 4; ++i)
    {
        err = q.Push(43 + i);
        CHECK_EQUAL(errorcode::kSuccess, err);
        CHECK_EQUAL(2 + i, q.Count());
        CHECK_EQUAL(42, q.Front());
    }

    err = q.Push(10);
    CHECK_EQUAL(errorcode::kFull, err);
    CHECK_EQUAL(5, q.Count());
}

TEST(QueueBasic, Pop)
{
    q.Push(1);
    q.Push(2);

    CHECK_EQUAL(1, q.Front());
    auto err = q.Pop();
    CHECK_EQUAL(errorcode::kSuccess, err);
    CHECK_EQUAL(1, q.Count());

    CHECK_EQUAL(2, q.Front());
    err = q.Pop();
    CHECK_EQUAL(errorcode::kSuccess, err);
    CHECK_EQUAL(0, q.Count());

    err = q.Pop();
    CHECK_EQUAL(errorcode::kEmpty, err);
    CHECK_EQUAL(0, q.Count());
}

int main(int argc, char** argv)
{
    return RUN_ALL_TESTS(argc, argv);
}
