#include "InputPS.h"
#include "GpioButtons.h"
#include "../util/Platform.h"
#include <Arduino.h>

using namespace PRUZEAmini;

namespace
{
constexpr uint32_t buttonMask(Input::Button button)
{
    return static_cast<uint32_t>(button);
}

constexpr uint8_t TRANSFER_HALF_PERIOD_USEC = 4;
constexpr uint8_t ATTENTION_SETUP_USEC = 10;

enum PSButton : uint16_t
{
    SELECT   = 1u << 0,
    L3       = 1u << 1,
    R3       = 1u << 2,
    START    = 1u << 3,
    UP       = 1u << 4,
    RIGHT    = 1u << 5,
    DOWN     = 1u << 6,
    LEFT     = 1u << 7,
    L2       = 1u << 8,
    R2       = 1u << 9,
    L1       = 1u << 10,
    R1       = 1u << 11,
    TRIANGLE = 1u << 12,
    CIRCLE   = 1u << 13,
    CROSS    = 1u << 14,
    SQUARE   = 1u << 15
};
}

InputPS::InputPS(const InputPSConfig& config)
    : clockPin(config.clockPin),
      commandPin(config.commandPin),
      attentionPin(config.attentionPin),
      dataPin(config.dataPin),
      extraGpioButtonPins(config.extraGpioButtonPins)
{
}

bool InputPS::begin()
{
    if (clockPin < 0 || commandPin < 0 || attentionPin < 0 || dataPin < 0)
    {
        available = false;
        reset();
        return false;
    }

    pinMode(clockPin,OUTPUT);digitalWrite(clockPin,HIGH);
    pinMode(commandPin,OUTPUT);digitalWrite(commandPin,HIGH);
    pinMode(attentionPin,OUTPUT);digitalWrite(attentionPin,HIGH);
    pinMode(dataPin,INPUT_PULLUP);

    GpioButtons::init(extraGpioButtonPins);
    reset();

    uint32_t buttons = 0;
    available = pollController(buttons);
    return available;
}

void InputPS::end()
{
    available = false;
    if (attentionPin >= 0) digitalWrite(attentionPin,HIGH);
    if (clockPin >= 0) digitalWrite(clockPin,HIGH);
    if (commandPin >= 0) digitalWrite(commandPin,HIGH);
    reset();
}

uint8_t InputPS::transferByte(uint8_t command)
{
    uint8_t response = 0;
    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        digitalWrite(commandPin,((command>>bit)&1u)?HIGH:LOW);
        digitalWrite(clockPin,LOW);
        Platform::sleepUsec(TRANSFER_HALF_PERIOD_USEC);

        if (digitalRead(dataPin)==HIGH)
        {
            response |= static_cast<uint8_t>(1u << bit);
        }

        digitalWrite(clockPin,HIGH);
        Platform::sleepUsec(TRANSFER_HALF_PERIOD_USEC);
    }
    digitalWrite(commandPin,HIGH);
    return response;
}

bool InputPS::pollController(uint32_t& buttons)
{
    static constexpr uint8_t COMMAND[] = {0x01, 0x42, 0x00, 0x00, 0x00};
    uint8_t response[sizeof(COMMAND)]{};

    digitalWrite(attentionPin,LOW);
    Platform::sleepUsec(ATTENTION_SETUP_USEC);

    for (size_t i = 0; i < sizeof(COMMAND); ++i)
    {
        response[i] = transferByte(COMMAND[i]);
    }

    digitalWrite(attentionPin,HIGH);
    Platform::sleepUsec(ATTENTION_SETUP_USEC);

    if (response[1] == 0x00 || response[1] == 0xFF || response[2] != 0x5A)
    {
        buttons = 0;
        return false;
    }

    const uint16_t pressed = static_cast<uint16_t>(~(
        static_cast<uint16_t>(response[3]) |
        static_cast<uint16_t>(static_cast<uint16_t>(response[4]) << 8)));

    buttons = 0;
    if (pressed & PSButton::UP)       buttons |= buttonMask(Button::UP);
    if (pressed & PSButton::DOWN)     buttons |= buttonMask(Button::DOWN);
    if (pressed & PSButton::LEFT)     buttons |= buttonMask(Button::LEFT);
    if (pressed & PSButton::RIGHT)    buttons |= buttonMask(Button::RIGHT);
    if (pressed & PSButton::CROSS)    buttons |= buttonMask(Button::A);
    if (pressed & PSButton::CIRCLE)   buttons |= buttonMask(Button::B);
    if (pressed & PSButton::SQUARE)   buttons |= buttonMask(Button::X);
    if (pressed & PSButton::TRIANGLE) buttons |= buttonMask(Button::Y);
    if (pressed & PSButton::L1)       buttons |= buttonMask(Button::L);
    if (pressed & PSButton::R1)       buttons |= buttonMask(Button::R);
    if (pressed & PSButton::START)    buttons |= buttonMask(Button::START);
    if (pressed & PSButton::SELECT)   buttons |= buttonMask(Button::SELECT);

    if (extraGpioButtonPins.VOL_DOWN < 0 && (buttons & buttonMask(Button::L)))
    {
        buttons |= buttonMask(Button::VOL_DOWN);
        buttons &= ~buttonMask(Button::L);
    }
    if (extraGpioButtonPins.VOL_UP < 0 && (buttons & buttonMask(Button::R)))
    {
        buttons |= buttonMask(Button::VOL_UP);
        buttons &= ~buttonMask(Button::R);
    }
    buttons |= GpioButtons::read(extraGpioButtonPins);
    return true;
}

uint32_t InputPS::readButtons()
{
    uint32_t buttons = 0;
    pollController(buttons);
    return buttons;
}
