#pragma once

#include "../internal/TouchSource.h"

namespace PRUZEAmini {

template<typename BaseInput>
class InputTouch : public BaseInput
{
private:
    TouchSource& touchSource;
    bool currentTouched = false;
    bool previousTouched = false;
    int16_t currentX = -1;
    int16_t currentY = -1;

public:
    template<typename BaseInputConfig>
    InputTouch(const InputTouchConfig<BaseInputConfig>& config, TouchSource& source)
        : BaseInput(config.input), touchSource(source)
    {
    }

    bool begin() override
    {
        const bool baseAvailable = BaseInput::begin();
        const bool touchAvailable = touchSource.isTouchAvailable();
        return baseAvailable || touchAvailable;
    }

    void end() override
    {
        currentTouched = false;
        previousTouched = false;
        currentX = -1;
        currentY = -1;
        BaseInput::end();
    }

    void update() override
    {
        BaseInput::update();
        previousTouched = currentTouched;

        int16_t x = -1;
        int16_t y = -1;
        currentTouched = touchSource.readTouch(x, y);
        currentX = currentTouched ? x : -1;
        currentY = currentTouched ? y : -1;
    }

    bool touched() const override { return currentTouched; }
    bool justTouched() const override { return currentTouched && !previousTouched; }
    bool justTouchReleased() const override { return !currentTouched && previousTouched; }
    int16_t touchX() const override { return currentX; }
    int16_t touchY() const override { return currentY; }
};

} // namespace PRUZEAmini
