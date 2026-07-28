#pragma once

#include "GraphicsBase.h"
#include "GraphicsLGFXContext.h"

namespace PRUZEAmini {

class GraphicsILI9341 : public GraphicsBase
{
private:
    GraphicsILI9341Config config;
    GraphicsLGFXContext& context;

    uint16_t canvasHeight = 0;
    uint8_t canvasIndex = 0;

    int32_t localY(int32_t y) const;
    void setFont(const char* str, Font font);

public:
    GraphicsILI9341(const GraphicsILI9341Config& config, GraphicsLGFXContext& context);
 
    bool begin() override;
    void end() override;
 
    void clearScreen() override;
    void fillScreen(Graphics::Color color) override;
    void drawPixel(int16_t x, int16_t y, Graphics::Color color) override;
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color) override;
    void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void drawString(const char* str, int16_t x, int16_t y, Graphics::Color color, Font font) override;
    uint16_t getTextWidth(const char* text, Font font) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t spriteScale,  Color transparentColor, bool flipX = false, bool flipY = false) override;
    void setViewport(int16_t viewportX, int16_t viewportY) override;
    bool readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount) override;

    bool push() override;
};

} // namespace
