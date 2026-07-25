#pragma once

#include "InputBase.h"

namespace PRUZEAmini {

// Experimental PlayStation controller input implementation.
// Compile-tested only; communication with real hardware has not been verified.
// Supports digital buttons only. ACK, analog axes, vibration, and mode setup
// are intentionally not implemented.
class InputPS : public InputBase
{
private:
    int8_t clockPin;
    int8_t commandPin;
    int8_t attentionPin;
    int8_t dataPin;
    ButtonPins extraGpioButtonPins;

    uint8_t transferByte(uint8_t command);
    bool pollController(uint32_t& buttons);
    uint32_t readButtons() override;

public:
    explicit InputPS(const InputPSConfig& config);

    bool begin() override;
    void end() override;
};

} // namespace PRUZEAmini
