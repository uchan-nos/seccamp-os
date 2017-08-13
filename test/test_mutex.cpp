#include <CppUTest/CommandLineTestRunner.h>
#include <thread>
#include "mutex.hpp"

TEST_GROUP(Mutex) {
    bitnos::SpinLockMutex m;

    TEST_SETUP()
    {}

    TEST_TEARDOWN()
    {}
};

TEST(Mutex, Init)
{
    CHECK_TRUE(!m.IsLocked());
}

TEST(Mutex, Lock)
{
    m.Lock();
    CHECK_TRUE(m.IsLocked());
    m.Unlock();
    CHECK_TRUE(!m.IsLocked());
}

const unsigned int kMaxCount = 100000;
bitnos::SpinLockMutex counter_mutex;
unsigned int shared_counter = 0;
void incrementer()
{
    for (unsigned int i = 0; i < kMaxCount; ++i)
    {
        counter_mutex.Lock();
        ++shared_counter;
        counter_mutex.Unlock();
    }
}

TEST(Mutex, MultiThread)
{
    std::thread t1(incrementer);
    incrementer();
    t1.join();

    CHECK_EQUAL(2 * kMaxCount, shared_counter);
}

TEST(Mutex, LockGuard)
{
    CHECK_TRUE(!m.IsLocked());
    {
        bitnos::LockGuard<decltype(m)> lock(m);
        CHECK_TRUE(m.IsLocked());
    }
    CHECK_TRUE(!m.IsLocked());
}
