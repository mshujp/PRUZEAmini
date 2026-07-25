/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

  Original Source: https://github.com/lovyan03/LovyanGFX/
  Licence: FreeBSD

  This copy is limited to the SPI/I2C platforms used by PRUZEAmini.
/----------------------------------------------------------------------------*/
#pragma once

#if defined(ESP_PLATFORM)

#include "esp32/Bus_I2C.hpp"
#include "esp32/Bus_SPI.hpp"

#elif defined(ARDUINO_ARCH_MBED_RP2040) || defined(ARDUINO_ARCH_RP2040) || defined(USE_PICO_SDK)

#include "rp2040/Bus_I2C.hpp"
#include "rp2040/Bus_SPI.hpp"

#endif
