#pragma once

#include "../internal/TouchSource.h"
#include "PRUZEAmini.h"

namespace PRUZEAmini {

class GraphicsLGFXContext : public TouchSource
{
private:
    GraphicsILI9341Config graphicsConfig{};
    GraphicsILI9341ParallelConfig parallelGraphicsConfig{};
    TouchXPT2046Config touchConfig{};
    bool parallel = false;
    bool touchConfigured = false;
    bool touchAvailable = false;

public:
    void configureGraphics(const GraphicsILI9341Config& config);
    void configureGraphics(const GraphicsILI9341ParallelConfig& config);
    void configureTouch(const TouchXPT2046Config& config);
    void clearTouch();
    bool begin();
    void end();

    bool isTouchAvailable() const override { return touchAvailable; }
    bool readTouch(int16_t& x, int16_t& y) override;
};

} // namespace PRUZEAmini
