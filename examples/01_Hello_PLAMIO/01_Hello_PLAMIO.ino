/*
===============================================================================
 PLAMIOmini Example
 01_Hello_PLAMIO
===============================================================================

This example shows the minimum structure of a PLAMIOmini game.

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PLAMIOmini.h>

using namespace PLAMIOmini;


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

InputConfig inputConfig = InputGpioButtonsConfig{
    .buttonMapping = {
        .UP       = -1,
        .DOWN     = -1,
        .LEFT     = -1,
        .RIGHT    = -1,
        .A        = -1,
        .B        = -1,
        .START    = -1,
        .VOL_UP   = -1,
        .VOL_DOWN = -1,
        .MUTE     = -1,
    }
};

AudioConfig audioConfig = AudioStubConfig{};
StorageConfig storageConfig = StorageStubConfig{};

// =============================================================================
// Game
// =============================================================================

class HelloPLAMIOGame : public App
{
public:
    const char* getId() const override { return "hello_plamio"; }

private:
    bool greeted = false;

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        greeted = false;
        dirty = true;
    }

    void onUpdate(
        Input& input,
        Audio& audio,
        Storage& storage,
        float deltaSec) override
    {
        (void)audio;
        (void)storage;
        (void)deltaSec;

        if (input.justPressed(Input::A))
        {
            greeted = !greeted;
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

        graphics.fillRoundRect(
            24, 24, 272, 192, 12,
            greeted ? Graphics::BLUE : Graphics::DARKGRAY);

        graphics.drawRoundRect(
            24, 24, 272, 192, 12, 3,
            Graphics::WHITE);

        graphics.drawString(
            "PLAMIOmini",
            160, 70,
            Graphics::YELLOW,
            Graphics::SIZE_32B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            greeted ? "HELLO!" : "HELLO PLAMIO",
            160, 125,
            Graphics::WHITE,
            greeted ? Graphics::SIZE_42B : Graphics::SIZE_32B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "PRESS A",
            160, 180,
            Graphics::CYAN,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
    }
};


// =============================================================================
// PLAMIOmini objects
// =============================================================================

HelloPLAMIOGame app;


// =============================================================================
// Arduino entry points
// =============================================================================

void setup()
{
    PLAMIOmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
