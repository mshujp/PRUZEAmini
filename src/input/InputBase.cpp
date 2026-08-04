#include "InputBase.h"
#include "../util/Platform.h"
#include <climits>

using namespace PRUZEAmini;

constexpr InputBase::Button InputBase::BUTTONS[];

int8_t InputBase::buttonIndex(Button b)
{
    for (size_t i = 0; i < BUTTON_COUNT; ++i)
    {
        if (BUTTONS[i] == b)
        {
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

InputBase::InputBase()
{
    reset();
}

void InputBase::update()
{
    if (!available)
    {
        reset();
        return;        
    }

    const uint64_t now = Platform::getMsec();

    previous = current;
    current = readButtons();

    repeatFlags = 0;

    for (size_t i = 0; i < BUTTON_COUNT; ++i)
    {
        const Button b = BUTTONS[i];

        if (justPressed(b))
        {
            buttons[i].pressStart = now;
            buttons[i].lastRepeat = now;
        
            repeatFlags |= static_cast<uint32_t>(b);
        }
        else if (!pressed(b))
        {
            buttons[i].pressStart = UINT64_MAX;
        }
        else
        {
            if (now - buttons[i].pressStart >= dasDelayMsec)
            {
                if (arrDelayMsec == 0 || (now - buttons[i].lastRepeat >= arrDelayMsec))
                {
                    buttons[i].lastRepeat = now;
                    repeatFlags |= static_cast<uint32_t>(b);
                }
            }
        }
    }
}

bool InputBase::hasInput()
{
    return current != 0;
}

bool InputBase::pressed(Button b) const
{
    return (current & static_cast<uint32_t>(b)) != 0;
}

bool InputBase::justPressed(Button b) const
{
    const uint32_t mask = static_cast<uint32_t>(b);
    return (current & mask) && !(previous & mask);
}

bool InputBase::released(Button b) const
{
    return !pressed(b);
}

bool InputBase::justReleased(Button b) const
{
    const uint32_t mask = static_cast<uint32_t>(b);
    return !(current & mask) && (previous & mask);
}

bool InputBase::held(Button b) const
{
    const uint32_t mask = static_cast<uint32_t>(b);
    return (current & mask) && (previous & mask);
}

uint64_t InputBase::holdMillis(Button b) const
{
    const int8_t idx = buttonIndex(b);
    if (idx < 0 || idx >= static_cast<int8_t>(BUTTON_COUNT) || buttons[idx].pressStart == UINT64_MAX)
    {
        return 0;
    }

    return Platform::getMsec() - buttons[idx].pressStart;
}

void InputBase::setRepeatSettings(uint16_t _dasDelayMsec, uint16_t _arrDelayMsec)
{
    dasDelayMsec = _dasDelayMsec;
    arrDelayMsec = _arrDelayMsec;
}

bool InputBase::repeat(Button b) const
{
    return (repeatFlags & static_cast<uint32_t>(b)) != 0;
}

void InputBase::reset()
{
    current = 0;
    previous = 0;
    repeatFlags = 0;

    for (size_t i = 0; i < BUTTON_COUNT; ++i)
    {
        buttons[i].pressStart = UINT64_MAX;
        buttons[i].lastRepeat = UINT64_MAX;
    }

    dasDelayMsec = DAS_DELAY_MSEC_DEFAULT;
    arrDelayMsec = ARR_DELAY_MSEC_DEFAULT;
}
