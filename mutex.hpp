#ifndef MUTEX_HPP_
#define MUTEX_HPP_

#include <stdint.h>
#include "asmfunc.h"

namespace bitnos
{
    class SpinLockMutex
    {
        uint64_t flag_;

    public:
        const static uint64_t kClear = 0;
        const static uint64_t kFlagged = 1;

        SpinLockMutex()
            : flag_(0)
        {}

        ~SpinLockMutex() = default;
        SpinLockMutex(const SpinLockMutex&) = delete;
        SpinLockMutex& operator =(const SpinLockMutex&) = delete;

        void Lock()
        {
            while (kFlagged == CompareExchange(&flag_, kClear, kFlagged));
        }

        bool TryLock()
        {
            return kClear == CompareExchange(&flag_, kClear, kFlagged);
        }

        void Unlock()
        {
            LFencedWrite(&flag_, kClear);
        }

        bool IsLocked() const
        {
            return flag_ == 1;
        }
    };

    template <typename Mutex>
    class LockGuard
    {
        Mutex& mutex_;
    public:
        LockGuard(Mutex& mutex)
            : mutex_(mutex)
        {
            mutex_.Lock();
        }

        ~LockGuard()
        {
            mutex_.Unlock();
        }

        LockGuard(const LockGuard&) = delete;
        LockGuard& operator =(const LockGuard&) = delete;
    };
}

#endif // MUTEX_HPP_
