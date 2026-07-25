#include "GraphicsILI9341.h"
#include "GraphicsSSD1306.h"
#include <Arduino.h>
#include "../third_party/LovyanGFX/src/LovyanGFX.hpp"

using namespace PRUZEAmini;

namespace {

// ILI9341 implementation

class LGFX_ILI9341 : public lgfx::LGFX_Device
{
private:
    lgfx::Panel_ILI9341 panel;
    lgfx::Bus_SPI bus;

public:
    void configure(const GraphicsILI9341Config& config)
    {
        auto busConfig = bus.config();
        busConfig.spi_host = static_cast<decltype(busConfig.spi_host)>(config.spiHost);
        busConfig.spi_mode = 0;
        busConfig.freq_write = config.spiWriteFreq;
        busConfig.freq_read = 16000000;
        busConfig.pin_sclk = config.clkPin;
        busConfig.pin_mosi = config.dataPin;
        busConfig.pin_miso = -1;
        busConfig.pin_dc = config.dcPin;
        bus.config(busConfig);
        panel.setBus(&bus);

        auto panelConfig = panel.config();
        panelConfig.pin_cs = config.csPin;
        panelConfig.pin_rst = config.resetPin;
        panelConfig.pin_busy = -1;
        panelConfig.panel_width = Display::ILI9341_SCREEN_H;
        panelConfig.panel_height = Display::ILI9341_SCREEN_W;
        panelConfig.memory_width = Display::ILI9341_SCREEN_H;
        panelConfig.memory_height = Display::ILI9341_SCREEN_W;
        panelConfig.readable = false;
        panelConfig.invert = false;
        panelConfig.rgb_order = false;
        panelConfig.dlen_16bit = false;
        panelConfig.bus_shared = true;
        panel.config(panelConfig);
        setPanel(&panel);
    }
};

LGFX_ILI9341 ili9341Lcd;
lgfx::LGFX_Sprite ili9341Canvas(&ili9341Lcd);

} // namespace

GraphicsILI9341::GraphicsILI9341(const GraphicsILI9341Config& config)
    : config(config)
{
}

bool GraphicsILI9341::begin()
{
    ili9341Lcd.configure(config);
    ili9341Lcd.init();
    ili9341Lcd.setRotation(config.lcdRotate);

    ili9341Canvas.setColorDepth(ili9341Lcd.getColorDepth());
    ili9341Canvas.setSwapBytes(true);
    canvasHeight = 0;
    canvasIndex = 0;

    for (uint8_t i = 1; i <= 4; ++i)
    {
        if (!ili9341Canvas.createSprite(Display::ILI9341_SCREEN_W, Display::ILI9341_SCREEN_H / i)) continue;

        canvasHeight = Display::ILI9341_SCREEN_H / i;
        break;
    }

    if (canvasHeight == 0) return false;

    ili9341Canvas.clear();

    if (config.backlightPin >= 0)
    {
        pinMode(config.backlightPin, OUTPUT);
        digitalWrite(config.backlightPin, HIGH);
    }

    viewportX = 0;
    viewportY = 0;
    screenDirty = true;
    return ili9341Canvas.getBuffer() != nullptr;
}

void GraphicsILI9341::end()
{
    ili9341Canvas.deleteSprite();
    ili9341Lcd.clear();
    if (config.backlightPin >= 0) digitalWrite(config.backlightPin, LOW);
    screenDirty = false;
}

int32_t GraphicsILI9341::localY(int32_t y) const
{
    return y - static_cast<int32_t>(canvasHeight * canvasIndex);
}

void GraphicsILI9341::clearScreen()
{
    ili9341Canvas.clear();
    screenDirty = true;
}

void GraphicsILI9341::fillScreen(Graphics::Color color)
{
    ili9341Canvas.fillScreen(color);
    screenDirty = true;
}

void GraphicsILI9341::drawPixel(int16_t x, int16_t y, Graphics::Color color)
{
    ili9341Canvas.drawPixel(x, localY(y), color);
    screenDirty = true;
}

void GraphicsILI9341::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    ili9341Canvas.drawLine(x1, localY(y1), x2, localY(y2), color);
    screenDirty = true;
}

void GraphicsILI9341::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    ili9341Canvas.drawTriangle(x0, localY(y0), x1, localY(y1), x2, localY(y2), color);
    screenDirty = true;
}

void GraphicsILI9341::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    ili9341Canvas.fillTriangle(x0, localY(y0), x1, localY(y1), x2, localY(y2), color);
    screenDirty = true;
}

void GraphicsILI9341::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    ili9341Canvas.drawRect(x, localY(y), w, h, color);
    screenDirty = true;
}

void GraphicsILI9341::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color)
{
    ili9341Canvas.drawRoundRect(x, localY(y), w, h, radius, color);
    screenDirty = true;
}

void GraphicsILI9341::fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    ili9341Canvas.fillRect(x, localY(y), w, h, color);
    screenDirty = true;
}

void GraphicsILI9341::fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Graphics::Color color)
{
    ili9341Canvas.fillRoundRect(x, localY(y), w, h, r, color);
    screenDirty = true;
}

void GraphicsILI9341::drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    ili9341Canvas.drawCircle(x, localY(y), r, color);
    screenDirty = true;
}

void GraphicsILI9341::drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    ili9341Canvas.drawEllipse(x, localY(y), rx, ry, color);
    screenDirty = true;
}

void GraphicsILI9341::fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    ili9341Canvas.fillCircle(x, localY(y), r, color);
    screenDirty = true;
}

void GraphicsILI9341::fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    ili9341Canvas.fillEllipse(x, localY(y), rx, ry, color);
    screenDirty = true;
}

void GraphicsILI9341::setFont(const char* str, Font font)
{
    const lgfx::IFont* targetFont = &fonts::DejaVu12;
    float scaleS = 1.0;

    switch (font)
    {
        case Font::SIZE_10:  targetFont = &fonts::DejaVu9; break;
        case Font::SIZE_13:  targetFont = &fonts::DejaVu12; break;
        case Font::SIZE_18:  targetFont = &fonts::DejaVu18; break;
        case Font::SIZE_22B: targetFont = &fonts::FreeSansBold9pt7b; break;
        case Font::SIZE_25:  targetFont = &fonts::DejaVu24; break;
        case Font::SIZE_25B: targetFont = &fonts::FreeSansBold9pt7b; scaleS = 1.13; break;
        case Font::SIZE_32:  targetFont = &fonts::DejaVu24; scaleS = 1.33; break;
        case Font::SIZE_32B: targetFont = &fonts::FreeSansBold12pt7b; scaleS = 1.10; break;
        case Font::SIZE_42:  targetFont = &fonts::DejaVu40; break;
        case Font::SIZE_42B: targetFont = &fonts::FreeSansBold18pt7b; break;
#ifdef PRUZEA_JAPANESE_FONT
        case Font::SIZE_16J: targetFont = &fonts::DejaVu18; break;
        case Font::SIZE_20J: targetFont = &fonts::DejaVu18; break;
        case Font::SIZE_32J: targetFont = &fonts::DejaVu24; scaleS = 1.33; break;
#endif
        default: targetFont = &fonts::DejaVu9; break;
    }
 
    ili9341Canvas.setFont(targetFont);
    ili9341Canvas.setTextSize(scaleS);
}

void GraphicsILI9341::drawString(const char* str, int16_t x, int16_t y, Graphics::Color color, Font font)
{
    if (str == nullptr) return;
    setFont(str, font);
    ili9341Canvas.setTextColor(color);
    ili9341Canvas.drawString(str, x, localY(y));
    screenDirty = true;
}

uint16_t GraphicsILI9341::getTextWidth(const char* text, Font font)
{
    if (text == nullptr) return 0;
    setFont(text, font);
    return ili9341Canvas.textWidth(text);
}

void GraphicsILI9341::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t spriteScale,  Color transparentColor, bool flipX, bool flipY)
{
    if (bitmap == nullptr || spriteScale == 0) return;

    if (spriteScale == 1 && !flipX && !flipY)
    {
        ili9341Canvas.pushImage(x, localY(y), w, h, bitmap, transparentColor);
    }
    else
    {
        const float zoomX = flipX ? -static_cast<float>(spriteScale)
                                  :  static_cast<float>(spriteScale);
        const float zoomY = flipY ? -static_cast<float>(spriteScale)
                                  :  static_cast<float>(spriteScale);

        ili9341Canvas.pushImageRotateZoom(
            x + static_cast<int16_t>((w * spriteScale) / 2),
            localY(y + static_cast<int16_t>((h * spriteScale) / 2)),
            w / 2, h / 2,
            0.0f,
            zoomX, zoomY,
            w, h,
            bitmap,
            transparentColor
        );
    }

    screenDirty = true;
}

void GraphicsILI9341::setViewport(int16_t x, int16_t y)
{
    if (viewportX == x && viewportY == y) return;
    viewportX = x;
    viewportY = y;
    screenDirty = true;
}

bool GraphicsILI9341::push()
{
    if (!screenDirty) return false;

    ili9341Canvas.pushSprite(&ili9341Lcd, -viewportX, (canvasIndex * canvasHeight) - viewportY);

    if (++canvasIndex * canvasHeight < Display::ILI9341_SCREEN_H)
    {
        ili9341Canvas.clear();
        screenDirty = false;
        return true;
    }

    canvasIndex = 0;
    screenDirty = false;
    return false;
}

bool GraphicsILI9341::readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount)
{
    if (outPixels == nullptr) return false;

    if (canvasHeight != Display::ILI9341_SCREEN_H) return false;

    if (y >= Display::ILI9341_SCREEN_H || pixelCount < Display::ILI9341_SCREEN_W) return false;

    const int32_t sourceY = static_cast<int32_t>(y) + viewportY;
    for (uint16_t x = 0; x < Display::ILI9341_SCREEN_W; ++x)
    {
        const int32_t sourceX = static_cast<int32_t>(x) + viewportX;
        if (sourceX < 0 || sourceY < 0 ||
            sourceX >= Display::ILI9341_SCREEN_W || sourceY >= Display::ILI9341_SCREEN_H)
        {
            outPixels[x] = static_cast<uint16_t>(Graphics::BLACK);
        }
        else
        {
            outPixels[x] = static_cast<uint16_t>(ili9341Canvas.readPixel(sourceX, sourceY));
        }
    }
    return true;
}

namespace {

// SSD1306 implementation

class LGFX_SSD1306 : public lgfx::LGFX_Device
{
private:
    lgfx::Panel_SSD1306 panel;
    lgfx::Bus_I2C bus;

public:
    void configure(const GraphicsSSD1306Config& config)
    {
        auto busConfig = bus.config();
        busConfig.i2c_port = config.i2cPort;
        busConfig.i2c_addr = config.i2cAddr;
        busConfig.pin_sda = config.sdaPin;
        busConfig.pin_scl = config.sclPin;
        busConfig.freq_write = 400000;
        busConfig.freq_read = 400000;
        bus.config(busConfig);
        panel.setBus(&bus);

        auto panelConfig = panel.config();
        panelConfig.pin_cs = -1;
        panelConfig.pin_rst = config.resetPin;
        panelConfig.pin_busy = -1;
        panelConfig.panel_width = Display::SSD1306_SCREEN_W;
        panelConfig.panel_height = Display::SSD1306_SCREEN_H;
        panelConfig.memory_width = Display::SSD1306_SCREEN_W;
        panelConfig.memory_height = Display::SSD1306_SCREEN_H;
        panel.config(panelConfig);
        setPanel(&panel);
    }
};

LGFX_SSD1306 ssd1306Lcd;
lgfx::LGFX_Sprite ssd1306Canvas(&ssd1306Lcd);

} // namespace

GraphicsSSD1306::GraphicsSSD1306(const GraphicsSSD1306Config& config)
    : config(config)
{
}

uint16_t GraphicsSSD1306::mono(Graphics::Color color) const
{
    return color == Graphics::SSD1306_OFF || color == Graphics::BLACK ? 0 : 1;
}

bool GraphicsSSD1306::begin()
{
    ssd1306Lcd.configure(config);
    ssd1306Lcd.init();
    ssd1306Lcd.setRotation(config.oledRotate);
    
    ssd1306Canvas.setColorDepth(1);
    ssd1306Canvas.createSprite(Display::SSD1306_SCREEN_W, Display::SSD1306_SCREEN_H);
    ssd1306Canvas.clear();

    viewportX = 0;
    viewportY = 0;

    screenDirty = true;
    return ssd1306Canvas.getBuffer() != nullptr;
}

void GraphicsSSD1306::end()
{
    ssd1306Canvas.deleteSprite();
    ssd1306Lcd.clear();
    screenDirty = false;
}

void GraphicsSSD1306::clearScreen()
{
    ssd1306Canvas.clear();
    screenDirty = true;
}

void GraphicsSSD1306::fillScreen(Graphics::Color color)
{
    ssd1306Canvas.fillScreen(mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::drawPixel(int16_t x, int16_t y, Graphics::Color color)
{
    ssd1306Canvas.drawPixel((int32_t)x, (int32_t)y, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    ssd1306Canvas.drawLine(x1, y1, x2, y2, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    ssd1306Canvas.drawTriangle(x0, y0, x1, y1, x2, y2, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    ssd1306Canvas.fillTriangle(x0, y0, x1, y1, x2, y2, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    ssd1306Canvas.drawRect(x, y, w, h, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color)
{
    ssd1306Canvas.drawRoundRect(x, y, w, h, radius, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    ssd1306Canvas.fillRect(x, y, w, h, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Graphics::Color color)
{
    ssd1306Canvas.fillRoundRect(x, y, w, h, r, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    ssd1306Canvas.drawCircle(x, y, r, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    ssd1306Canvas.drawEllipse(x, y, rx, ry, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    ssd1306Canvas.fillCircle(x, y, r, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    ssd1306Canvas.fillEllipse(x, y, rx, ry, mono(color));
    screenDirty = true;
}

void GraphicsSSD1306::setFont(const char* str, Font font)
{
    const lgfx::IFont* targetFont = &fonts::DejaVu12;
    float scaleS = 1.0;

    switch (font)
    {
        case Font::SIZE_10:  targetFont = &fonts::DejaVu9; break;
        case Font::SIZE_13:  targetFont = &fonts::DejaVu12; break;
        case Font::SIZE_18:  targetFont = &fonts::DejaVu18; break;
        case Font::SIZE_22B: targetFont = &fonts::FreeSansBold9pt7b; break;
        case Font::SIZE_25:  targetFont = &fonts::DejaVu24; break;
        case Font::SIZE_25B: targetFont = &fonts::FreeSansBold9pt7b; scaleS = 1.13; break;
        case Font::SIZE_32:  targetFont = &fonts::DejaVu24; scaleS = 1.33; break;
        case Font::SIZE_32B: targetFont = &fonts::FreeSansBold12pt7b; scaleS = 1.10; break;
        case Font::SIZE_42:  targetFont = &fonts::DejaVu40; break;
        case Font::SIZE_42B: targetFont = &fonts::FreeSansBold18pt7b; break;
#ifdef PRUZEA_JAPANESE_FONT
        case Font::SIZE_16J: targetFont = &fonts::DejaVu18; break;
        case Font::SIZE_20J: targetFont = &fonts::DejaVu18; break;
        case Font::SIZE_32J: targetFont = &fonts::DejaVu24; scaleS = 1.33; break;
#endif
        default: targetFont = &fonts::DejaVu9; break;
    }
 
    ssd1306Canvas.setFont(targetFont);
    ssd1306Canvas.setTextSize(scaleS);
}

void GraphicsSSD1306::drawString(const char* str, int16_t x, int16_t y, Graphics::Color color, Font font)
{
    if (str == nullptr) return;
    setFont(str, font);
    ssd1306Canvas.setTextColor(mono(color), 0);
    ssd1306Canvas.drawString(str, x, y);
    screenDirty = true;
}

uint16_t GraphicsSSD1306::getTextWidth(const char* text, Font font)
{
    if (text == nullptr) return 0;
    setFont(text, font);
    return ssd1306Canvas.textWidth(text);
}

void GraphicsSSD1306::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t scale, Color transparentColor, bool flipX, bool flipY)
{
    if (bitmap == nullptr || scale == 0) return;

    for (uint16_t sy = 0; sy < h; ++sy)
    {
        const uint16_t srcY = flipY ? (h - 1 - sy) : sy;

        for (uint16_t sx = 0; sx < w; ++sx)
        {
            const uint16_t srcX = flipX ? (w - 1 - sx) : sx;
            const Graphics::Color c = static_cast<Graphics::Color>(bitmap[static_cast<uint32_t>(srcY) * w + srcX]);

            if (c == transparentColor) continue;

            const int32_t dx = static_cast<int32_t>(x) + static_cast<int32_t>(sx) * scale;
            const int32_t dy = static_cast<int32_t>(y) + static_cast<int32_t>(sy) * scale;

            if (dx >= ssd1306Canvas.width() || dy >= ssd1306Canvas.height() || dx + scale <= 0 || dy + scale <= 0)
            {
                continue;
            }

            for (uint16_t yy = 0; yy < scale; ++yy)
            {
                for (uint16_t xx = 0; xx < scale; ++xx)
                {
                    ssd1306Canvas.drawPixel(dx + xx, dy + yy, 1);
                }
            }
        }
    }

    screenDirty = true;
}

bool GraphicsSSD1306::readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount)
{
    if (outPixels == nullptr) return false;

    if (y >= Display::SSD1306_SCREEN_H || pixelCount < Display::SSD1306_SCREEN_W) return false;

    const int32_t sourceY = static_cast<int32_t>(viewportY) + y;

    for (uint16_t x = 0; x < Display::SSD1306_SCREEN_W; ++x)
    {
        const int32_t sourceX = static_cast<int32_t>(viewportX) + x;

        if (sourceX < 0 || sourceY < 0 ||
            sourceX >= Display::SSD1306_SCREEN_W || sourceY >= Display::SSD1306_SCREEN_H)
        {
            outPixels[x] = static_cast<uint16_t>(Graphics::BLACK);
            continue;
        }

        const uint16_t pixel = static_cast<uint16_t>(
            ssd1306Canvas.readPixel(sourceX, sourceY));

        outPixels[x] = pixel == 0
            ? static_cast<uint16_t>(Graphics::BLACK)
            : static_cast<uint16_t>(Graphics::WHITE);
    }

    return true;
}

bool GraphicsSSD1306::push()
{
    if (!screenDirty) return false;
    ssd1306Canvas.pushSprite(&ssd1306Lcd, -viewportX, -viewportY);
    screenDirty = false;
    return false;
}
