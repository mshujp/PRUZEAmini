#pragma once

#if defined(ARDUINO_ARCH_RP2040)
#include <pico/mutex.h>
#else
#include <atomic>
#endif

namespace PRUZEAmini
{

// Cross-platform lock for very short critical sections shared between
// worker threads or processor cores. Do not acquire this lock from an ISR.
class SpinLock
{
public:
#if defined(ARDUINO_ARCH_RP2040)
    SpinLock()
    {
        mutex_init(&mutex);
    }
#else
    SpinLock() = default;
#endif
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    void lock()
    {
#if defined(ARDUINO_ARCH_RP2040)
        mutex_enter_blocking(&mutex);
#else
        while (flag.test_and_set(std::memory_order_acquire))
        {
        }
#endif
    }

    void unlock()
    {
#if defined(ARDUINO_ARCH_RP2040)
        mutex_exit(&mutex);
#else
        flag.clear(std::memory_order_release);
#endif
    }

private:
#if defined(ARDUINO_ARCH_RP2040)
    mutex_t mutex;
#else
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
#endif
};

class SpinLockGuard
{
public:
    explicit SpinLockGuard(SpinLock& lock)
        : lock(lock)
    {
        lock.lock();
    }

    ~SpinLockGuard()
    {
        lock.unlock();
    }

    SpinLockGuard(const SpinLockGuard&) = delete;
    SpinLockGuard& operator=(const SpinLockGuard&) = delete;

private:
    SpinLock& lock;
};

} // namespace PRUZEAmini
