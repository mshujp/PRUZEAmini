#pragma once

#include "PRUZEAmini.h"

namespace PRUZEAmini {

class GraphicsBase : public Graphics
{
protected:
    int16_t viewportX = 0;
    int16_t viewportY = 0;
    bool screenDirty = false;

public:
    virtual ~GraphicsBase() = default;
    virtual bool begin() = 0; 
    virtual void end() = 0; 

    virtual bool push() = 0; 

    virtual bool readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount) { return false;}

    using Graphics::drawString;
    using Graphics::drawRect;
    using Graphics::drawRoundRect;
    uint16_t getTextHeight(const char* text, Font font) override;
    void setViewport(int16_t x, int16_t y) override;
    int16_t getViewportX() const { return viewportX; }
    int16_t getViewportY() const { return viewportY; }
    void resetViewport() override;
    void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t thickness, Graphics::Color color) override;
    void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t thickness, Graphics::Color color) override;
    void drawString(const char* str, int16_t x, int16_t y, Color color, Font font, HorizontalAlign ha, VerticalAlign va) override;
};

} // namespace
