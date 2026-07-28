/*
===============================================================================
 PRUZEAmini Example
 15_Analog_Stick
===============================================================================

This example shows the basic PlayStation analog-stick API.

- hasAnalogSticks() reports whether analog-stick data is available.
- axis() returns each normalized axis from -1000 to 1000.
- pressed() reads L2, R2, L3, and R3 as ordinary logical buttons.

Before compiling, replace the required -1 values with pins for your hardware.
*/

#include <PRUZEAmini.h>
#include <cstdio>

using namespace PRUZEAmini;


// =============================================================================
// Hardware configuration
// =============================================================================

GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost         = 0,
    .spiWriteFreq    = 60000000,
    .clkPin          = -1,
    .dataPin         = -1,
    .dcPin           = -1,
    .csPin           = -1,
    .resetPin        = -1,
    .backlightPin    = -1,
    .lcdRotate       = 0,
};

InputConfig inputConfig = InputPSConfig{
    .clockPin     = -1,
    .commandPin   = -1,
    .attentionPin = -1,
    .dataPin      = -1,

    // Increase this value if a centered stick still moves in the app.
    .analogDeadZone = 24,
    .leftXCenter    = 128,
    .leftYCenter    = 128,
    .rightXCenter   = 128,
    .rightYCenter   = 128,

    .extraGpioButtonPins = {
        .VOL_UP   = -1,
        .VOL_DOWN = -1,
        .MUTE     = -1,
    },
};

AudioConfig audioConfig = AudioStubConfig{};
StorageConfig storageConfig = StorageStubConfig{};


// =============================================================================
// App
// =============================================================================

class AnalogStickApp : public App
{
private:
    bool analogAvailable = false;
    int16_t leftX = 0;
    int16_t leftY = 0;
    int16_t rightX = 0;
    int16_t rightY = 0;
    bool l2Pressed = false;
    bool r2Pressed = false;
    bool l3Pressed = false;
    bool r3Pressed = false;

    void drawStick(Graphics& graphics, const char* name, int16_t centerX, int16_t x, int16_t y, bool stickPressed, bool triggerPressed)
    {
        constexpr int16_t CENTER_Y = 115;
        constexpr int16_t RADIUS = 50;
        constexpr int16_t MARKER_RANGE = 40;

        graphics.drawString(name, centerX, 43, Graphics::WHITE, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);

        graphics.drawCircle(centerX, CENTER_Y, RADIUS, Graphics::LIGHTGRAY);
        graphics.drawLine(centerX - RADIUS, CENTER_Y, centerX + RADIUS, CENTER_Y, Graphics::DARKGRAY);
        graphics.drawLine(centerX, CENTER_Y - RADIUS, centerX, CENTER_Y + RADIUS, Graphics::DARKGRAY);

        const int16_t markerX = centerX + static_cast<int16_t>(x * MARKER_RANGE / 1000);
        const int16_t markerY = CENTER_Y + static_cast<int16_t>(y * MARKER_RANGE / 1000);
        graphics.fillCircle(markerX, markerY, stickPressed ? 9 : 6, stickPressed ? Graphics::YELLOW : Graphics::CYAN);

        char text[32];
        std::snprintf(text, sizeof(text), "X:%5d Y:%5d", x, y);
        graphics.drawString(text, centerX, 180, Graphics::WHITE, Graphics::SIZE_13, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);

        std::snprintf(text, sizeof(text), "%s:%s  %s:%s",
            name[0] == 'L' ? "L3" : "R3", stickPressed ? "ON" : "OFF",
            name[0] == 'L' ? "L2" : "R2", triggerPressed ? "ON" : "OFF");
        graphics.drawString(text, centerX, 200,
            (stickPressed || triggerPressed) ? Graphics::YELLOW : Graphics::LIGHTGRAY,
            Graphics::SIZE_13, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)audio;
        (void)storage;
        (void)deltaSec;

        const bool nextAnalogAvailable = input.hasAnalogSticks();
        const int16_t nextLeftX = input.axis(Input::LEFT_X);
        const int16_t nextLeftY = input.axis(Input::LEFT_Y);
        const int16_t nextRightX = input.axis(Input::RIGHT_X);
        const int16_t nextRightY = input.axis(Input::RIGHT_Y);

        // Buttons use the same pressed() API as every other PRUZEAmini input.
        const bool nextL2Pressed = input.pressed(Input::L2);
        const bool nextR2Pressed = input.pressed(Input::R2);
        const bool nextL3Pressed = input.pressed(Input::L3);
        const bool nextR3Pressed = input.pressed(Input::R3);

        dirty = dirty ||
            analogAvailable != nextAnalogAvailable ||
            leftX != nextLeftX || leftY != nextLeftY ||
            rightX != nextRightX || rightY != nextRightY ||
            l2Pressed != nextL2Pressed || r2Pressed != nextR2Pressed ||
            l3Pressed != nextL3Pressed || r3Pressed != nextR3Pressed;

        analogAvailable = nextAnalogAvailable;
        leftX = nextLeftX;
        leftY = nextLeftY;
        rightX = nextRightX;
        rightY = nextRightY;
        l2Pressed = nextL2Pressed;
        r2Pressed = nextR2Pressed;
        l3Pressed = nextL3Pressed;
        r3Pressed = nextR3Pressed;
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty) return false;

        graphics.fillScreen(Graphics::BLACK);
        graphics.drawString("ANALOG STICK", 160, 4, Graphics::WHITE, Graphics::SIZE_22B,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
        graphics.drawString(analogAvailable ? "ANALOG: READY" : "NO ANALOG STICKS", 160, 28,
            analogAvailable ? Graphics::GREEN : Graphics::ORANGE, Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);

        drawStick(graphics, "LEFT", 84, leftX, leftY, l3Pressed, l2Pressed);
        drawStick(graphics, "RIGHT", 236, rightX, rightY, r3Pressed, r2Pressed);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
    }

public:
    const char* getId() const override { return "analog_stick"; }
};

AnalogStickApp app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
