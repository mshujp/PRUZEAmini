#include "InputGpioButtons.h"

#include <Arduino.h>

using namespace PRUZEAmini;

namespace {

struct MappingEntry
{
    Input::Button button;
    int16_t ButtonPins::* pin;
};

constexpr MappingEntry MAPPINGS[] =
{
    {Input::UP, &ButtonPins::UP},
    {Input::DOWN, &ButtonPins::DOWN},
    {Input::LEFT, &ButtonPins::LEFT},
    {Input::RIGHT, &ButtonPins::RIGHT},
    {Input::A, &ButtonPins::A},
    {Input::B, &ButtonPins::B},
    {Input::X, &ButtonPins::X},
    {Input::Y, &ButtonPins::Y},
    {Input::L, &ButtonPins::L},
    {Input::R, &ButtonPins::R},
    {Input::START, &ButtonPins::START},
    {Input::SELECT, &ButtonPins::SELECT},
    {Input::VOL_UP, &ButtonPins::VOL_UP},
    {Input::VOL_DOWN, &ButtonPins::VOL_DOWN},
    {Input::MUTE, &ButtonPins::MUTE}
};

} // namespace

InputGpioButtons::InputGpioButtons(const InputGpioButtonsConfig& config)
    : gpioButtonPins(config.gpioButtonPins)
{
}

bool InputGpioButtons::begin()
{
    for (const auto& entry : MAPPINGS)
    {
        const int16_t pin = gpioButtonPins.*(entry.pin);
        if (pin >= 0) pinMode(pin, INPUT_PULLUP);
    }

    reset();
    available = true;
    return true;
}

void InputGpioButtons::end()
{
    available = false;
    reset();
}

uint32_t InputGpioButtons::readButtons()
{
    uint32_t result = 0;
    for (const auto& entry : MAPPINGS)
    {
        const int16_t pin = gpioButtonPins.*(entry.pin);
        if (pin >= 0 && digitalRead(pin) == LOW) result |= entry.button;
    }
    return result;
}
