/*
===============================================================================
 PLAMIOmini Example
 01_Hello_PLAMIO_SSD1306
===============================================================================

This example shows the minimum structure of a PLAMIOmini game
using a 128 x 64 SSD1306 OLED display.

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change oledRotate if necessary to match the display orientation.
*/

#include <PLAMIOmini.h>

using namespace PLAMIOmini;


// =============================================================================
// Hardware configuration
// =============================================================================

GraphicsConfig graphicsConfig = GraphicsSSD1306Config{
    .i2cPort   = 0,
    .i2cAddr   = 0x3C,
    .sdaPin    = -1,
    .sclPin    = -1,
    .resetPin  = -1,
    .oledRotate = 0,
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

class HelloPLAMIOGame : public GameMini
{
public:
    const char* getId() const override { return "hello_plamio_ssd1306"; }

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

        graphics.fillScreen(Graphics::SSD1306_OFF);

        graphics.drawRoundRect(
            1, 1, 126, 62, 6,
            Graphics::SSD1306_ON);

        graphics.drawString(
            "PLAMIOmini",
            64, 13,
            Graphics::SSD1306_ON,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawLine(
            10, 23, 117, 23,
            Graphics::SSD1306_ON);

        graphics.drawString(
            greeted ? "HELLO!" : "HELLO",
            64, 38,
            Graphics::SSD1306_ON,
            greeted ? Graphics::SIZE_22B : Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            greeted ? "NICE TO MEET YOU" : "PRESS A",
            64, 55,
            Graphics::SSD1306_ON,
            Graphics::SIZE_10,
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

HelloPLAMIOGame game;


// =============================================================================
// Arduino entry points
// =============================================================================

void setup()
{
    PLAMIOmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, game);
}

void loop()
{
}
