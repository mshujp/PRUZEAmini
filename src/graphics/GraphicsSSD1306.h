#pragma once

#include "GraphicsBase.h"

namespace PRUZEAmini {

class GraphicsSSD1306 : public GraphicsBase
{
private:
    GraphicsSSD1306Config config;

    uint16_t mono(Graphics::Color color) const;
    void setFont(const char* str, Font font);

    uint16_t* spriteSheetBuf = nullptr;
    uint32_t spriteSheetBufSize = 0;

public:
    explicit GraphicsSSD1306(const GraphicsSSD1306Config& config);

    bool begin() override;
    void end() override;

    uint16_t getWidth() const override { return Display::SSD1306_SCREEN_W; }
    uint16_t getHeight() const override { return Display::SSD1306_SCREEN_H; }
    void clearScreen() override;
    void fillScreen(Graphics::Color color) override;
    void drawPixel(int16_t x, int16_t y, Graphics::Color color) override;
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawWideLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, Color color) override;
    void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) override;
    void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color) override;
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color) override;
    void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color) override {};
    void fillRectGradient(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color0, Color color1, FillStyle style) override;
    void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float angle0, float angle1, Color color) override;
    void drawString(const char* str, int16_t x, int16_t y, Graphics::Color color, Font font) override;
    uint16_t getTextWidth(const char* text, Font font) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options) override;
    void drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options) override;
    void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void getClipRect(int16_t& x, int16_t& y, uint16_t& w, uint16_t& h) override;
    void resetClipRect() override;

    bool readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount) override;
    bool push() override;
};

} // namespace PRUZEAmini
