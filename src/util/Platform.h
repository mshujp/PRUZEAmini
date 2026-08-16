#pragma once

#include <stdint.h>
#include <stddef.h>

namespace PRUZEAmini {
namespace Platform {

// This file defines internal helper/utility functions to support the 
// implementation of functions declared under `namespace Platform` 
// in the public header "PRUZEAmini.h".

void sleepMsec(uint32_t msec);
void sleepUsec(uint32_t usec);

void initializeManualCoreFlashLockout();
bool beginManualCoreFlashWrite();
void endManualCoreFlashWrite(bool locked);

} // namespace
} // namespace
