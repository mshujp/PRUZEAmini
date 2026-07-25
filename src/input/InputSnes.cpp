#include "InputSnes.h"
#include "GpioButtons.h"
#include "../util/Platform.h"
#include <Arduino.h>

using namespace PRUZEAmini;

namespace
{
constexpr Input::Button SNES_BUTTON_MAP[] = {
    Input::Button::B,
    Input::Button::Y,
    Input::Button::SELECT,
    Input::Button::START,
    Input::Button::UP,
    Input::Button::DOWN,
    Input::Button::LEFT,
    Input::Button::RIGHT,
    Input::Button::A,
    Input::Button::X,
    Input::Button::L,
    Input::Button::R
};

constexpr size_t SNES_BUTTON_COUNT = sizeof(SNES_BUTTON_MAP) / sizeof(SNES_BUTTON_MAP[0]);
}

InputSnes::InputSnes(const InputSnesConfig& config)
    : clkPin(config.clkPin),
      latPin(config.latPin),
      dataPin(config.dataPin),
      extraGpioButtonPins(config.extraGpioButtonPins)
{
}

bool InputSnes::begin()
{
    if (clkPin < 0 || latPin < 0 || dataPin < 0) return false;
    pinMode(clkPin, OUTPUT);
    pinMode(latPin, OUTPUT);
    pinMode(dataPin, INPUT_PULLUP);

    GpioButtons::init(extraGpioButtonPins);

    reset();
    available = true;
    return true;
}

void InputSnes::end()
{
    available = false;
    reset();
}

uint32_t InputSnes::readButtons()
{
    uint32_t buttons = 0;

    digitalWrite(clkPin, LOW);

    digitalWrite(latPin, HIGH);
    Platform::sleepUsec(12);

    digitalWrite(latPin, LOW);
    Platform::sleepUsec(6);

    for (size_t i = 0; i < 16; ++i)
    {
        if (i < SNES_BUTTON_COUNT && digitalRead(dataPin) == LOW)
        {
            buttons |= static_cast<uint32_t>(SNES_BUTTON_MAP[i]);
        }

        digitalWrite(clkPin, HIGH);
        Platform::sleepUsec(6);

        digitalWrite(clkPin, LOW);
        Platform::sleepUsec(6);
    }

    if (extraGpioButtonPins.VOL_DOWN < 0)
    {
        // L acts as VOL_DOWN.
        if (buttons & static_cast<uint32_t>(Button::L))
        {
            buttons |= static_cast<uint32_t>(Button::VOL_DOWN);
            buttons &= ~static_cast<uint32_t>(Button::L);
        }
    }
    if (extraGpioButtonPins.VOL_UP < 0)
    {
        // R acts as VOL_UP.
        if (buttons & static_cast<uint32_t>(Button::R))
        {
            buttons |= static_cast<uint32_t>(Button::VOL_UP);
            buttons &= ~static_cast<uint32_t>(Button::R);
        }
    }

    // Auxiliary GPIO buttons are already mapped to their final logical roles.
    buttons |= GpioButtons::read(extraGpioButtonPins);

    return buttons;
}
