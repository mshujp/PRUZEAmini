#include "Platform.h"
#include "PRUZEAmini.h"
#include <Arduino.h>

using namespace PRUZEAmini;

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
