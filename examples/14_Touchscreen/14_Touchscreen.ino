/*
===============================================================================
 PRUZEAmini Example
 14_Touchscreen
===============================================================================

This example shows:
- XPT2046 touch panel input on SPI0
- Raw touch values and mapped screen coordinates
- The current touch position as a yellow dot in a small screen preview

Required library:
- XPT2046_Touchscreen by Paul Stoffregen

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Set the display and XPT2046 touch-panel pins.

Touch calibration values differ between panels.
Adjust TOUCH_MIN_X / TOUCH_MAX_X / TOUCH_MIN_Y / TOUCH_MAX_Y when necessary.
*/

#include <PRUZEAmini.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

using namespace PRUZEAmini;

// =============================================================================
// Touch configuration
// =============================================================================

static constexpr int8_t TOUCH_SCK_PIN  = -1;
static constexpr int8_t TOUCH_MOSI_PIN = -1;
static constexpr int8_t TOUCH_MISO_PIN = -1;
static constexpr int8_t TOUCH_CS_PIN   = -1;
static constexpr int8_t TOUCH_IRQ_PIN  = -1;

static constexpr int16_t TOUCH_MIN_X = 250;
static constexpr int16_t TOUCH_MAX_X = 3850;
static constexpr int16_t TOUCH_MIN_Y = 250;
static constexpr int16_t TOUCH_MAX_Y = 3850;
static constexpr int16_t TOUCH_MIN_Z = 200;

// =============================================================================
// PRUZEAmini hardware configuration
// =============================================================================

GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost       = 1,
    .spiWriteFreq  = 62500000,
    .clkPin        = -1,
    .dataPin       = -1,
    .dcPin         = -1,
    .csPin         = -1,
    .resetPin      = -1,
    .backlightPin  = -1,
    .lcdRotate     = 1,
};

InputConfig inputConfig = InputGpioButtonsConfig{
    .gpioButtonPins = {
        .UP       = -1,
        .DOWN     = -1,
        .LEFT     = -1,
        .RIGHT    = -1,
        .A        = -1,
        .B        = -1,
        .X        = -1,
        .Y        = -1,
        .L        = -1,
        .R        = -1,
        .START    = -1,
        .SELECT   = -1,
        .VOL_UP   = -1,
        .VOL_DOWN = -1,
        .MUTE     = -1,
    },
};

AudioConfig audioConfig = AudioStubConfig{};
StorageConfig storageConfig = StorageStubConfig{};

XPT2046_Touchscreen touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);

// =============================================================================
// App
// =============================================================================

class TouchscreenApp : public App
{
public:
    const char* getId() const override { return "touchscreen"; }

private:
    static constexpr int16_t PREVIEW_X = 10;
    static constexpr int16_t PREVIEW_Y = 48;
    static constexpr int16_t PREVIEW_W = 150;
    static constexpr int16_t PREVIEW_H = 112;

    bool touchReady = false;
    bool touching = false;

    int16_t rawX = 0;
    int16_t rawY = 0;
    int16_t rawZ = 0;
    int16_t screenX = 0;
    int16_t screenY = 0;

    uint32_t lastTouchReadMsec = 0;

    static int16_t clampMap(
        int32_t value,
        int32_t inMin,
        int32_t inMax,
        int16_t outMin,
        int16_t outMax)
    {
        if (inMin == inMax)
        {
            return outMin;
        }

        const int32_t mapped =
            (value - inMin) * (outMax - outMin) /
            (inMax - inMin) + outMin;

        const int16_t low = min(outMin, outMax);
        const int16_t high = max(outMin, outMax);
        return static_cast<int16_t>(constrain(mapped, low, high));
    }

    void readTouch()
    {
        touching = false;

        if (!touchReady || !touch.touched())
        {
            return;
        }

        const TS_Point point = touch.getPoint();
        rawX = static_cast<uint16_t>(point.x) & 0x0FFF;
        rawY = static_cast<uint16_t>(point.y) & 0x0FFF;
        rawZ = static_cast<uint16_t>(point.z) & 0x0FFF;

        if (rawZ < TOUCH_MIN_Z)
        {
            return;
        }

        // First map the touch panel in its unrotated 240 x 320 orientation.
        const int16_t touchX = clampMap(
            rawX, TOUCH_MIN_X, TOUCH_MAX_X, 0, 239);
        const int16_t touchY = clampMap(
            rawY, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, 319);

        // lcdRotate = 1: rotate the touch coordinates 90 degrees left.
        screenX = constrain(319 - touchY, 0, 319);
        screenY = constrain(touchX,       0, 239);
        touching = true;
    }

    void drawPreview(Graphics& graphics)
    {
        graphics.drawRect(
            PREVIEW_X - 1,
            PREVIEW_Y - 1,
            PREVIEW_W + 2,
            PREVIEW_H + 2,
            Graphics::DARKGRAY);

        graphics.fillRect(
            PREVIEW_X,
            PREVIEW_Y,
            PREVIEW_W,
            PREVIEW_H,
            Graphics::BLACK);

        if (!touching)
        {
            return;
        }

        const int16_t dotX = PREVIEW_X +
            static_cast<int32_t>(screenX) * (PREVIEW_W - 1) / 319;
        const int16_t dotY = PREVIEW_Y +
            static_cast<int32_t>(screenY) * (PREVIEW_H - 1) / 239;

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

        snprintf(text, sizeof(text), "X: %3d", screenX);
        graphics.drawString(text, 176, 112, Graphics::WHITE, Graphics::SIZE_13);

        snprintf(text, sizeof(text), "Y: %3d", screenY);
        graphics.drawString(text, 176, 134, Graphics::WHITE, Graphics::SIZE_13);

        snprintf(text, sizeof(text), "RAW X: %4d", rawX);
        graphics.drawString(text, 176, 168, Graphics::LIGHTGRAY, Graphics::SIZE_13);

        snprintf(text, sizeof(text), "RAW Y: %4d", rawY);
        graphics.drawString(text, 176, 190, Graphics::LIGHTGRAY, Graphics::SIZE_13);

        snprintf(text, sizeof(text), "Z: %4d", rawZ);
        graphics.drawString(text, 176, 212, Graphics::LIGHTGRAY, Graphics::SIZE_13);
    }

public:
    void onInit(Storage& storage) override
    {
        if (TOUCH_SCK_PIN < 0 ||
            TOUCH_MOSI_PIN < 0 ||
            TOUCH_MISO_PIN < 0 ||
            TOUCH_CS_PIN < 0)
        {
            touchReady = false;
            dirty = true;
            return;
        }

        pinMode(TOUCH_CS_PIN, OUTPUT);
        digitalWrite(TOUCH_CS_PIN, HIGH);

        SPI.setSCK(TOUCH_SCK_PIN);
        SPI.setTX(TOUCH_MOSI_PIN);
        SPI.setRX(TOUCH_MISO_PIN);
        SPI.begin();

        touchReady = touch.begin(SPI);
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        const uint32_t now = millis();
        if (now - lastTouchReadMsec < 16)
        {
            return;
        }
        lastTouchReadMsec = now;

        const bool previousTouching = touching;
        const int16_t previousX = screenX;
        const int16_t previousY = screenY;
        const int16_t previousZ = rawZ;

        readTouch();

        if (touching != previousTouching ||
            screenX != previousX ||
            screenY != previousY ||
            rawZ != previousZ)
        {
            dirty = true;
        }
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }

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
    }
};

TouchscreenApp app;

void setup()
{
    PRUZEAmini::start(
        graphicsConfig,
        inputConfig,
        audioConfig,
        storageConfig,
        app);
}

void loop()
{
}
