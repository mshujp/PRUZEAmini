#include "Platform.h"
#include "PRUZEAmini.h"
#include <Arduino.h>
#include <atomic>

#if defined(ARDUINO_ARCH_RP2040)
#include <pico/multicore.h>
#endif

using namespace PRUZEAmini;

namespace
{
#if defined(ARDUINO_ARCH_RP2040)
std::atomic<bool> manualCoreFlashLockoutReady{false};
#endif
}

uint32_t Platform::getMsec()
{
    return millis();
}

uint64_t Platform::getUsec()
{
    return micros();
}

void Platform::sleepMsec(uint32_t msec)
{
    delay(msec);
}

void Platform::sleepUsec(uint32_t usec)
{
    delayMicroseconds(usec);
}

bool Platform::elapsed(uint32_t now, uint32_t startMsec, uint32_t durationMsec)
{
    return static_cast<uint32_t>(now - startMsec) >= durationMsec;
}

void Platform::initializeManualCoreFlashLockout()
{
#if defined(ARDUINO_ARCH_RP2040)
    multicore_lockout_victim_init();
    manualCoreFlashLockoutReady.store(true, std::memory_order_release);
#endif
}

bool Platform::beginManualCoreFlashWrite()
{
#if defined(ARDUINO_ARCH_RP2040)
    if (!manualCoreFlashLockoutReady.load(std::memory_order_acquire)) return false;
    multicore_lockout_start_blocking();
    return true;
#else
    return false;
#endif
}

void Platform::endManualCoreFlashWrite(bool locked)
{
#if defined(ARDUINO_ARCH_RP2040)
    if (locked) multicore_lockout_end_blocking();
#else
    (void)locked;
#endif
}
