#pragma once

#include "../internal/TouchSource.h"
#include "PRUZEAmini.h"

namespace PRUZEAmini {

class GraphicsLGFXContext : public TouchSource
{
private:
    GraphicsILI9341Config graphicsConfig{};
    TouchXPT2046Config touchConfig{};
    bool touchConfigured = false;
    bool touchAvailable = false;

public:
    void configureGraphics(const GraphicsILI9341Config& config);
    void configureTouch(const TouchXPT2046Config& config);
    void clearTouch();
    bool begin();
    void end();

    bool isTouchAvailable() const override { return touchAvailable; }
    bool readTouch(int16_t& x, int16_t& y) override;
};

} // namespace PRUZEAmini
