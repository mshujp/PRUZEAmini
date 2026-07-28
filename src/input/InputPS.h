#pragma once

#include "InputBase.h"

namespace PRUZEAmini {

// Experimental PlayStation controller input implementation.
// ACK, vibration, and mode setup are intentionally not implemented.
class InputPS : public InputBase
{
private:
    int8_t clockPin;
    int8_t commandPin;
    int8_t attentionPin;
    int8_t dataPin;
    ButtonPins extraGpioButtonPins;
    uint8_t analogDeadZone;
    uint8_t axisCenters[4];
    uint8_t axisValues[4]{};
    bool analogAvailable = false;

    uint8_t transferByte(uint8_t command);
    bool pollController(uint32_t& buttons);
    uint32_t readButtons() override;

public:
    explicit InputPS(const InputPSConfig& config);

    bool begin() override;
    void end() override;
    bool hasAnalogSticks() const override;
    int16_t axis(Axis axis) const override;
};

} // namespace PRUZEAmini
