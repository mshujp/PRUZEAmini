#include "GraphicsILI9341.h"
#include "GraphicsSSD1306.h"
#include <Arduino.h>
#include "../third_party/LovyanGFX/src/LovyanGFX.hpp"

#if defined(ARDUINO_ARCH_RP2040)
#include "BusParallelPIO.h"
#elif defined(ESP_PLATFORM)
#include "../third_party/LovyanGFX/src/lgfx/v1/platforms/esp32/Bus_Parallel8.hpp"
#endif

using namespace PRUZEAmini;

namespace {

// ILI9341 implementation

class LGFX_ILI9341 : public lgfx::LGFX_Device
{
private:
    lgfx::Panel_ILI9341 panel;
    lgfx::Bus_SPI bus;
#if defined(ARDUINO_ARCH_RP2040)
    BusParallelPIO parallelBus;
#elif defined(ESP_PLATFORM) && (!defined(CONFIG_IDF_TARGET) || defined(CONFIG_IDF_TARGET_ESP32))
    lgfx::Bus_Parallel8 parallelBus;
#endif
    lgfx::Touch_XPT2046 touch;
    int16_t minimumTouchPressure = 0;
    bool configured = false;

public:
    void configure(const GraphicsILI9341Config& config, const TouchXPT2046Config* touchConfig)
    {
        configured = true;
        auto busConfig = bus.config();
        busConfig.spi_host = static_cast<decltype(busConfig.spi_host)>(config.spiHost);
        busConfig.spi_mode = 0;
        busConfig.freq_write = config.spiWriteFreq;
        busConfig.freq_read = 16000000;
        busConfig.pin_sclk = config.clkPin;
        busConfig.pin_mosi = config.dataPin;
        busConfig.pin_miso =
            touchConfig != nullptr && touchConfig->spiHost == config.spiHost
                ? touchConfig->misoPin
                : -1;
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

        if (touchConfig != nullptr)
        {
            auto touchDriverConfig = touch.config();
            touchDriverConfig.spi_host = static_cast<decltype(touchDriverConfig.spi_host)>(touchConfig->spiHost);
            touchDriverConfig.freq = touchConfig->spiFreq;
            touchDriverConfig.pin_sclk = touchConfig->clkPin;
            touchDriverConfig.pin_mosi = touchConfig->mosiPin;
            touchDriverConfig.pin_miso = touchConfig->misoPin;
            touchDriverConfig.pin_cs = touchConfig->csPin;
            // XPT2046 IRQ behavior differs between supported Arduino cores.
            // Polling keeps touch input consistent on both Pico and ESP32.
            touchDriverConfig.pin_int = -1;
            touchDriverConfig.x_min = touchConfig->minX;
            touchDriverConfig.x_max = touchConfig->maxX;
            touchDriverConfig.y_min = touchConfig->minY;
            touchDriverConfig.y_max = touchConfig->maxY;
            touchDriverConfig.offset_rotation = touchConfig->offsetRotation;
            touchDriverConfig.bus_shared = touchConfig->spiHost == config.spiHost;
            touch.config(touchDriverConfig);
            panel.setTouch(&touch);
            minimumTouchPressure = touchConfig->minZ;
        }
        else
        {
            panel.setTouch(nullptr);
            minimumTouchPressure = 0;
        }
        setPanel(&panel);
    }

    void configure(const GraphicsILI9341ParallelConfig& config, const TouchXPT2046Config* touchConfig)
    {
#if defined(ARDUINO_ARCH_RP2040)
        parallelBus.configure(config);
        configured = true;
#elif defined(ESP_PLATFORM) && (!defined(CONFIG_IDF_TARGET) || defined(CONFIG_IDF_TARGET_ESP32))
        auto busConfig = parallelBus.config();
        busConfig.i2s_port = I2S_NUM_0;
        busConfig.freq_write = config.writeFreq;
        busConfig.pin_rd = config.rdPin;
        busConfig.pin_wr = config.wrPin;
        busConfig.pin_rs = config.dcPin;
        for (uint8_t i = 0; i < 8; ++i) busConfig.pin_data[i] = config.dataPinBase + i;
        parallelBus.config(busConfig);
        configured = true;
#else
        configured = false;
        return;
#endif

#if defined(ARDUINO_ARCH_RP2040) || (defined(ESP_PLATFORM) && (!defined(CONFIG_IDF_TARGET) || defined(CONFIG_IDF_TARGET_ESP32)))
        panel.setBus(&parallelBus);

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
        panelConfig.bus_shared = false;
        panel.config(panelConfig);

        if (touchConfig != nullptr)
        {
            auto touchDriverConfig = touch.config();
            touchDriverConfig.spi_host = static_cast<decltype(touchDriverConfig.spi_host)>(touchConfig->spiHost);
            touchDriverConfig.freq = touchConfig->spiFreq;
            touchDriverConfig.pin_sclk = touchConfig->clkPin;
            touchDriverConfig.pin_mosi = touchConfig->mosiPin;
            touchDriverConfig.pin_miso = touchConfig->misoPin;
            touchDriverConfig.pin_cs = touchConfig->csPin;
            touchDriverConfig.pin_int = -1;
            touchDriverConfig.x_min = touchConfig->minX;
            touchDriverConfig.x_max = touchConfig->maxX;
            touchDriverConfig.y_min = touchConfig->minY;
            touchDriverConfig.y_max = touchConfig->maxY;
            touchDriverConfig.offset_rotation = touchConfig->offsetRotation;
            touchDriverConfig.bus_shared = false;
            touch.config(touchDriverConfig);
            panel.setTouch(&touch);
            minimumTouchPressure = touchConfig->minZ;
        }
        else
        {
            panel.setTouch(nullptr);
            minimumTouchPressure = 0;
        }
        setPanel(&panel);
#endif
    }

    bool isConfigured() const { return configured; }

    bool readTouch(int16_t& x, int16_t& y)
    {
        lgfx::touch_point_t point;
        if (getTouch(&point) == 0) return false;
        if (static_cast<uint32_t>(point.size) * 256 < static_cast<uint16_t>(minimumTouchPressure)) return false;

        x = point.x;
        y = point.y;
        return true;
    }
};

LGFX_ILI9341 ili9341Lcd;
lgfx::LGFX_Sprite ili9341Canvas(&ili9341Lcd);

} // namespace

void GraphicsLGFXContext::configureGraphics(const GraphicsILI9341Config& config)
{
    graphicsConfig = config;
    parallel = false;
}

void GraphicsLGFXContext::configureGraphics(const GraphicsILI9341ParallelConfig& config)
{
    parallelGraphicsConfig = config;
    parallel = true;
}

void GraphicsLGFXContext::configureTouch(const TouchXPT2046Config& config)
{
    touchConfig = config;
    touchConfigured =
        config.clkPin >= 0 &&
        config.mosiPin >= 0 &&
        config.misoPin >= 0 &&
        config.csPin >= 0;
}

void GraphicsLGFXContext::clearTouch()
{
    touchConfigured = false;
    touchAvailable = false;
}

bool GraphicsLGFXContext::begin()
{
    if (parallel)
    {
        ili9341Lcd.configure(parallelGraphicsConfig, touchConfigured ? &touchConfig : nullptr);
    }
    else
    {
        ili9341Lcd.configure(graphicsConfig, touchConfigured ? &touchConfig : nullptr);
    }
    if (!ili9341Lcd.isConfigured()) return false;
    const bool displayAvailable = ili9341Lcd.init();
    touchAvailable = displayAvailable && touchConfigured;
    return displayAvailable;
}

void GraphicsLGFXContext::end()
{
    touchAvailable = false;
    ili9341Lcd.clear();
}

bool GraphicsLGFXContext::readTouch(int16_t& x, int16_t& y)
{
    if (!touchAvailable) return false;
    return ili9341Lcd.readTouch(x, y);
}

GraphicsILI9341::GraphicsILI9341(const GraphicsILI9341Config& config, GraphicsLGFXContext& context)
    : config(config), context(context)
{
}

GraphicsILI9341::GraphicsILI9341(const GraphicsILI9341ParallelConfig& config, GraphicsLGFXContext& context)
    : parallelConfig(config), context(context), parallel(true)
{
}

bool GraphicsILI9341::begin()
{
    if (parallel)
    {
        context.configureGraphics(parallelConfig);
    }
    else
    {
        context.configureGraphics(config);
    }
    if (!context.begin()) return false;
    const uint8_t lcdRotate = parallel ? parallelConfig.lcdRotate : config.lcdRotate;
    ili9341Lcd.setRotation(lcdRotate);

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

    const int8_t backlightPin = parallel ? parallelConfig.backlightPin : config.backlightPin;
    if (backlightPin >= 0)
    {
        pinMode(backlightPin, OUTPUT);
        digitalWrite(backlightPin, HIGH);
    }

    viewportX = 0;
    viewportY = 0;
    screenDirty = true;
    return ili9341Canvas.getBuffer() != nullptr;
}

void GraphicsILI9341::end()
{
    ili9341Canvas.deleteSprite();
    context.end();
    const int8_t backlightPin = parallel ? parallelConfig.backlightPin : config.backlightPin;
    if (backlightPin >= 0) digitalWrite(backlightPin, LOW);
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

void GraphicsILI9341::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    drawSprite(bitmap, x, y, w, h, SpriteOptions{});
}

void GraphicsILI9341::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options)
{
    if (bitmap == nullptr || options.scale == 0) return;

    const bool transformed = options.scale != 1 || options.angle != 0.0f || options.flipX || options.flipY;

    if (!transformed)
    {
        if (options.transparent)
        {
            ili9341Canvas.pushImage(
                x, localY(y), w, h,
                bitmap,
                static_cast<uint16_t>(options.transparentColor));
        }
        else
        {
            ili9341Canvas.pushImage(x, localY(y), w, h, bitmap);
        }
    }
    else
    {
        const float zoomX = options.flipX ? -static_cast<float>(options.scale) : static_cast<float>(options.scale);
        const float zoomY = options.flipY ? -static_cast<float>(options.scale) : static_cast<float>(options.scale);
        const float destinationX = x + (w * options.scale) * 0.5f;
        const float destinationY = localY(y + static_cast<int32_t>((h * options.scale) / 2));

        if (options.transparent)
        {
            ili9341Canvas.pushImageRotateZoom(
                destinationX, destinationY,
                w * 0.5f, h * 0.5f,
                Math::radToDeg(options.angle),
                zoomX, zoomY,
                w, h,
                bitmap,
                static_cast<uint16_t>(options.transparentColor));
        }
        else
        {
            ili9341Canvas.pushImageRotateZoom(
                destinationX, destinationY,
                w * 0.5f, h * 0.5f,
                Math::radToDeg(options.angle),
                zoomX, zoomY,
                w, h,
                bitmap);
        }
    }

    screenDirty = true;
}

void GraphicsILI9341::drawImage(const Image::ImageData& image, int16_t x, int16_t y)
{
    drawSprite(image.getBuffer(), x, y, image.getWidth(), image.getHeight());
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

void GraphicsSSD1306::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    drawSprite(bitmap, x, y, w, h, SpriteOptions{});
}

void GraphicsSSD1306::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options)
{
    if (bitmap == nullptr || options.scale == 0) return;

    constexpr auto sourceDepth = lgfx::color_depth_t::rgb565_nonswapped;
    const auto* palette = static_cast<const lgfx::rgb888_t*>(nullptr);
    const bool transformed = options.scale != 1 || options.angle != 0.0f || options.flipX || options.flipY;

    if (!transformed)
    {
        if (options.transparent)
        {
            ssd1306Canvas.pushImage(
                x, y, w, h,
                bitmap,
                static_cast<uint16_t>(options.transparentColor),
                sourceDepth,
                palette);
        }
        else
        {
            ssd1306Canvas.pushImage(x, y, w, h, bitmap, sourceDepth, palette);
        }
    }
    else
    {
        const float zoomX = options.flipX ? -static_cast<float>(options.scale) : static_cast<float>(options.scale);
        const float zoomY = options.flipY ? -static_cast<float>(options.scale) : static_cast<float>(options.scale);
        const float destinationX = x + (w * options.scale) * 0.5f;
        const float destinationY = y + (h * options.scale) * 0.5f;

        if (options.transparent)
        {
            ssd1306Canvas.pushImageRotateZoom(destinationX, destinationY,
                w * 0.5f, h * 0.5f,
                Math::radToDeg(options.angle),
                zoomX, zoomY,
                w, h,
                bitmap,
                static_cast<uint16_t>(options.transparentColor),
                sourceDepth,
                palette);
        }
        else
        {
            ssd1306Canvas.pushImageRotateZoom(
                destinationX, destinationY,
                w * 0.5f, h * 0.5f,
                Math::radToDeg(options.angle),
                zoomX, zoomY,
                w, h,
                bitmap,
                sourceDepth,
                palette);
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
