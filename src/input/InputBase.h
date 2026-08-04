#pragma once

#include "PRUZEAmini.h"
#include <stddef.h>

namespace PRUZEAmini {

class InputBase : public Input
{
private:
    uint32_t current = 0;
    uint32_t previous = 0;
    uint32_t repeatFlags = 0;

    static constexpr Button BUTTONS[] = {
        Button::UP,
        Button::DOWN,
        Button::LEFT,
        Button::RIGHT,
        Button::A,
        Button::B,
        Button::X,
        Button::Y,
        Button::L,
        Button::R,
        Button::L2,
        Button::R2,
        Button::L3,
        Button::R3,
        Button::START,
        Button::SELECT,
        Button::VOL_UP,
        Button::VOL_DOWN,
        Button::MUTE
    };

    static constexpr size_t BUTTON_COUNT = sizeof(BUTTONS) / sizeof(BUTTONS[0]);

    uint64_t dasDelayMsec = DAS_DELAY_MSEC_DEFAULT;
    uint64_t arrDelayMsec = ARR_DELAY_MSEC_DEFAULT;

    struct ButtonState
    {
        uint64_t pressStart = 0;
        uint64_t lastRepeat = 0;
    };

    ButtonState buttons[BUTTON_COUNT];

    static int8_t buttonIndex(Button b);

protected:
    virtual uint32_t readButtons() = 0;
    bool available = false;

public:
    InputBase();
    virtual ~InputBase() = default;
    virtual bool begin() = 0;
    virtual void end() = 0;

    virtual void update();
    bool hasInput();
    void reset();

    bool pressed(Button b) const override;
    bool justPressed(Button b) const override;
    bool released(Button b) const override;
    bool justReleased(Button b) const override;
    bool held(Button b) const override;
    uint64_t holdMillis(Button b) const override;
    void setRepeatSettings(uint16_t dasDelayMsec, uint16_t arrDelayMsec) override;
    bool repeat(Button b) const override;
};

} // namespace PRUZEAmini
