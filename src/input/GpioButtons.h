#pragma once

#include "InputBase.h"
#include <Arduino.h>

namespace PLAMIOmini {
namespace GpioButtons {

inline void init(const ButtonPins& buttonPins)
{
    const int16_t pins[] =
    {
        buttonPins.UP, buttonPins.DOWN, buttonPins.LEFT, buttonPins.RIGHT,
        buttonPins.A, buttonPins.B, buttonPins.X, buttonPins.Y,
        buttonPins.L, buttonPins.R, buttonPins.START, buttonPins.SELECT,
        buttonPins.VOL_UP, buttonPins.VOL_DOWN, buttonPins.MUTE
    };

    for (int16_t pin : pins)
    {
        if (pin >= 0) pinMode(pin, INPUT_PULLUP);
    }
}

inline uint32_t read(const ButtonPins& buttonPins)
{
    struct Entry
    {
        Input::Button button;
        int16_t pin;
    };

    const Entry entries[] =
    {
        {Input::UP, buttonPins.UP}, {Input::DOWN, buttonPins.DOWN},
        {Input::LEFT, buttonPins.LEFT}, {Input::RIGHT, buttonPins.RIGHT},
        {Input::A, buttonPins.A}, {Input::B, buttonPins.B},
        {Input::X, buttonPins.X}, {Input::Y, buttonPins.Y},
        {Input::L, buttonPins.L}, {Input::R, buttonPins.R},
        {Input::START, buttonPins.START}, {Input::SELECT, buttonPins.SELECT},
        {Input::VOL_UP, buttonPins.VOL_UP}, {Input::VOL_DOWN, buttonPins.VOL_DOWN},
        {Input::MUTE, buttonPins.MUTE}
    };

    uint32_t value = 0;
    for (const auto& entry : entries)
    {
        if (entry.pin >= 0 && digitalRead(entry.pin) == LOW) value |= entry.button;
    }
    return value;
}

} // namespace GpioButtons
} // namespace PLAMIOmini
