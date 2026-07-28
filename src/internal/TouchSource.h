#pragma once

#include <stdint.h>

namespace PRUZEAmini {

class TouchSource
{
public:
    virtual ~TouchSource() = default;
    virtual bool isTouchAvailable() const = 0;
    virtual bool readTouch(int16_t& x, int16_t& y) = 0;
};

} // namespace PRUZEAmini
