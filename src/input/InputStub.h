#pragma once

#include "InputBase.h"

namespace PRUZEAmini {

class InputStub : public InputBase
{
private:
    uint32_t readButtons() override { return 0; }

public:
    explicit InputStub(const InputStubConfig&) {}

    bool begin() override
    {
        available = false;
        return false;
    }

    void end() override
    {
        available = false;
        reset();
    }
};

} // namespace PRUZEAmini
