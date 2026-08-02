/*
===============================================================================
 PRUZEAmini Example
 14_Touchscreen
===============================================================================

This example shows:
- XPT2046 touch input through InputTouchConfig
- ILI9341 and XPT2046 sharing SPI1 through LovyanGFX
- Touch coordinates already converted to visible screen coordinates

Supported targets:
- RP2040 / RP2350
- ESP32 / ESP32-S3
- ESP32-C3 / ESP32-C6 are not supported by this example.

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Adjust the calibration values when necessary.
*/

#include <PRUZEAmini.h>
#include <cstdio>

using namespace PRUZEAmini;

// =============================================================================
// PRUZEAmini hardware configuration
// =============================================================================

GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost      = 1,
    .spiWriteFreq = 62500000,
    .clkPin       = -1,
    .dataPin      = -1,
    .dcPin        = -1,
    .csPin        = -1,
    .resetPin     = -1,
    .backlightPin = -1,
    .lcdRotate    = 1,
};

InputConfig inputConfig = InputTouchConfig<InputStubConfig>{
    .input = {},
    .touch = {
        .spiHost = 1,
        .spiFreq = 2000000,
        .clkPin  = -1, // Same pin as graphicsConfig.clkPin.
        .mosiPin = -1, // Same pin as graphicsConfig.dataPin.
        .misoPin = -1,
        .csPin   = -1,
        .irqPin  = -1,
        .minX    = 250,
        .maxX    = 3850,
        .minY    = 250,
        .maxY    = 3850,
        .minZ          = 2048,
        .offsetRotation = 5,
    },
};

AudioConfig audioConfig = AudioStubConfig{};
StorageConfig storageConfig = StorageStubConfig{};

// =============================================================================
// Application
// =============================================================================

class TouchscreenApp : public App
{
private:
    static constexpr int16_t PREVIEW_X = 10;
    static constexpr int16_t PREVIEW_Y = 48;
    static constexpr int16_t PREVIEW_W = 150;
    static constexpr int16_t PREVIEW_H = 112;

    bool touchReady = false;
    bool touching = false;
    int16_t screenX = 0;
    int16_t screenY = 0;

    void drawPreview(Graphics& graphics)
    {
        graphics.drawRect(PREVIEW_X - 1, PREVIEW_Y - 1, PREVIEW_W + 2, PREVIEW_H + 2, Graphics::DARKGRAY);
        graphics.fillRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, Graphics::BLACK);

        if (!touching) return;

        const int16_t dotX = PREVIEW_X +
            static_cast<int32_t>(screenX) * (PREVIEW_W - 1) /
            (Display::ILI9341_SCREEN_W - 1);
        const int16_t dotY = PREVIEW_Y +
            static_cast<int32_t>(screenY) * (PREVIEW_H - 1) /
            (Display::ILI9341_SCREEN_H - 1);

        graphics.fillCircle(dotX, dotY, 4, Graphics::YELLOW);
    }

    void drawValues(Graphics& graphics)
    {
        char text[32];

        graphics.drawString(
            touchReady ? "TOUCH: READY" : "TOUCH: NOT FOUND",
            176,
            50,
            touchReady ? Graphics::GREEN : Graphics::RED,
            Graphics::SIZE_13);

        graphics.drawString(
            touching ? "STATE: TOUCHED" : "STATE: RELEASED",
            176,
            78,
            touching ? Graphics::YELLOW : Graphics::LIGHTGRAY,
            Graphics::SIZE_13);

        std::snprintf(text, sizeof(text), "X: %3d", static_cast<int>(screenX));
        graphics.drawString(text, 176, 112, Graphics::WHITE, Graphics::SIZE_13);

        std::snprintf(text, sizeof(text), "Y: %3d", static_cast<int>(screenY));
        graphics.drawString(text, 176, 134, Graphics::WHITE, Graphics::SIZE_13);
    }

public:
    const char* getId() const override { return "touchscreen"; }

    void onInit(Storage& storage) override
    {
        (void)storage;
        const auto& config =
            std::get<InputTouchConfig<InputStubConfig>>(inputConfig).touch;
        touchReady =
            config.clkPin >= 0 &&
            config.mosiPin >= 0 &&
            config.misoPin >= 0 &&
            config.csPin >= 0;
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)audio;
        (void)storage;
        (void)deltaSec;

        const bool nextTouching = input.touched();
        const int16_t nextScreenX =
            nextTouching ? input.touchX() : screenX;
        const int16_t nextScreenY =
            nextTouching ? input.touchY() : screenY;

        if (touching != nextTouching ||
            screenX != nextScreenX ||
            screenY != nextScreenY)
        {
            touching = nextTouching;
            screenX = nextScreenX;
            screenY = nextScreenY;
            dirty = true;
        }
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty) return false;

        graphics.fillScreen(Graphics::BLACK);
        graphics.drawString(
            "TOUCHSCREEN",
            10,
            8,
            Graphics::WHITE,
            Graphics::SIZE_22B);

        drawPreview(graphics);
        drawValues(graphics);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
    }
};

TouchscreenApp app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
