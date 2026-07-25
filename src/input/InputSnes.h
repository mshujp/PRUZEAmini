#pragma once

#include "InputBase.h"

namespace PLAMIOmini {

class InputSnes : public InputBase
{
private:
    int8_t clkPin;
    int8_t latPin;
    int8_t dataPin;
    ButtonPins extraGpioButtonPins;

    uint32_t readButtons() override;

public:
    explicit InputSnes(const InputSnesConfig& config);

    bool begin() override;
    void end() override;
};

} // namespace PLAMIOmini
