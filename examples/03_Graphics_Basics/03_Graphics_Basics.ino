/*
===============================================================================
 PRUZEAmini Example
 03_Graphics_Basics
===============================================================================

This example shows the basic drawing functions provided by Graphics.

Controls:
- A : Next page
- B : Previous page

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>

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

InputConfig inputConfig = InputGpioButtonsConfig{
    .gpioButtonPins = {
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

class GraphicsBasicsGame : public App
{
public:
    const char* getId() const override { return "graphics_basics"; }

private:
    static constexpr uint8_t PAGE_COUNT = 3;

    uint8_t page = 0;

    void drawHeader(Graphics& graphics, const char* title)
    {
        graphics.fillRect(0, 0, 320, 34, Graphics::DARKGRAY);

        graphics.drawString(
            title,
            12, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        char pageText[8];
        snprintf(pageText, sizeof(pageText), "%u/%u", page + 1, PAGE_COUNT);

        graphics.drawString(
            pageText,
            308, 17,
            Graphics::CYAN,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawFooter(Graphics& graphics)
    {
        graphics.drawString(
            "B: PREV",
            12, 216,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "A: NEXT",
            308, 216,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawShapesPage(Graphics& graphics)
    {
        drawHeader(graphics, "SHAPES");

        graphics.drawRect(22, 54, 72, 46, 3, Graphics::CYAN);
        graphics.fillRect(112, 54, 72, 46, Graphics::BLUE);

        graphics.drawRoundRect(
            202, 54, 94, 46, 10, 3,
            Graphics::YELLOW);

        graphics.fillRoundRect(
            22, 118, 72, 58, 12,
            Graphics::GREEN);

        graphics.drawCircle(148, 147, 30, Graphics::MAGENTA);
        graphics.fillCircle(250, 147, 30, Graphics::ORANGE);

        graphics.drawString(
            "OUTLINE",
            58, 105,
            Graphics::WHITE,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::TOP);

        graphics.drawString(
            "FILLED",
            148, 105,
            Graphics::WHITE,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::TOP);

        graphics.drawString(
            "ROUNDED",
            249, 105,
            Graphics::WHITE,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::TOP);
    }

    void drawLinesPage(Graphics& graphics)
    {
        drawHeader(graphics, "LINES");

        for (int16_t x = 20; x <= 300; x += 20)
        {
            graphics.drawLine(160, 118, x, 48, Graphics::DARKGRAY);
            graphics.drawLine(160, 118, x, 188, Graphics::DARKGRAY);
        }

        graphics.drawLine(28, 58, 292, 178, Graphics::CYAN);
        graphics.drawLine(28, 178, 292, 58, Graphics::MAGENTA);

        graphics.drawTriangle(
            58, 176,
            104, 72,
            150, 176,
            Graphics::YELLOW);

        graphics.fillTriangle(
            180, 176,
            226, 72,
            272, 176,
            Graphics::GREEN);

        graphics.fillCircle(160, 118, 5, Graphics::WHITE);

        graphics.drawString(
            "LINES AND TRIANGLES",
            160, 192,
            Graphics::WHITE,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawTextPage(Graphics& graphics)
    {
        drawHeader(graphics, "TEXT");

        graphics.drawString(
            "LEFT",
            20, 55,
            Graphics::CYAN,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::TOP);

        graphics.drawString(
            "CENTER",
            160, 88,
            Graphics::YELLOW,
            Graphics::SIZE_25B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "RIGHT",
            300, 122,
            Graphics::MAGENTA,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "PRUZEAmini",
            160, 165,
            Graphics::WHITE,
            Graphics::SIZE_32B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawLine(20, 55, 300, 55, Graphics::DARKGRAY);
        graphics.drawLine(20, 88, 300, 88, Graphics::DARKGRAY);
        graphics.drawLine(20, 122, 300, 122, Graphics::DARKGRAY);
        graphics.drawLine(20, 165, 300, 165, Graphics::DARKGRAY);
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        page = 0;
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
            page = (page + 1) % PAGE_COUNT;
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            page = (page + PAGE_COUNT - 1) % PAGE_COUNT;
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

        switch (page)
        {
            case 0:
                drawShapesPage(graphics);
                break;

            case 1:
                drawLinesPage(graphics);
                break;

            default:
                drawTextPage(graphics);
                break;
        }

        drawFooter(graphics);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
    }
};


// =============================================================================
// PRUZEAmini objects
// =============================================================================

GraphicsBasicsGame app;


// =============================================================================
// Arduino entry points
// =============================================================================

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
