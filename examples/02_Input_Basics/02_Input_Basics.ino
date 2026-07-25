/*
===============================================================================
 PLAMIOmini Example
 02_Input_Basics
===============================================================================

This example shows how to use game input in a simple interactive scene.

Controls:
- D-PAD : Move
- A     : Change color
- B     : Reset position
- START : Pause / Resume

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

class InputBasicsGame : public App
{
public:
    const char* getId() const override { return "input_basics"; }

private:
    static constexpr float MOVE_SPEED = 120.0f;
    static constexpr int16_t PLAYER_SIZE = 28;
    static constexpr float FIELD_LEFT = 18.0f;
    static constexpr float FIELD_TOP = 42.0f;
    static constexpr float FIELD_RIGHT = 302.0f;
    static constexpr float FIELD_BOTTOM = 192.0f;

    float playerX = 0.0f;
    float playerY = 0.0f;
    uint8_t colorIndex = 0;
    bool paused = false;

    Graphics::Color playerColor() const
    {
        static const Graphics::Color COLORS[] = {
            Graphics::CYAN,
            Graphics::GREEN,
            Graphics::ORANGE,
            Graphics::MAGENTA,
        };

        return COLORS[colorIndex];
    }

    void resetPosition()
    {
        playerX = FIELD_LEFT
                + (FIELD_RIGHT - FIELD_LEFT - PLAYER_SIZE) / 2.0f;

        playerY = FIELD_TOP
                + (FIELD_BOTTOM - FIELD_TOP - PLAYER_SIZE) / 2.0f;
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        resetPosition();
        colorIndex = 0;
        paused = false;
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

        if (input.justPressed(Input::START))
        {
            paused = !paused;
            dirty = true;
        }

        if (paused)
        {
            return;
        }

        bool moved = false;

        if (input.pressed(Input::LEFT))
        {
            playerX -= MOVE_SPEED * deltaSec;
            moved = true;
        }

        if (input.pressed(Input::RIGHT))
        {
            playerX += MOVE_SPEED * deltaSec;
            moved = true;
        }

        if (input.pressed(Input::UP))
        {
            playerY -= MOVE_SPEED * deltaSec;
            moved = true;
        }

        if (input.pressed(Input::DOWN))
        {
            playerY += MOVE_SPEED * deltaSec;
            moved = true;
        }

        if (playerX < FIELD_LEFT)
            playerX = FIELD_LEFT;

        if (playerX > FIELD_RIGHT - PLAYER_SIZE)
            playerX = FIELD_RIGHT - PLAYER_SIZE;

        if (playerY < FIELD_TOP)
            playerY = FIELD_TOP;

        if (playerY > FIELD_BOTTOM - PLAYER_SIZE)
            playerY = FIELD_BOTTOM - PLAYER_SIZE;

        if (input.justPressed(Input::A))
        {
            colorIndex = (colorIndex + 1) % 4;
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            resetPosition();
            dirty = true;
        }

        if (moved)
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
            "INPUT BASICS",
            160, 20,
            Graphics::YELLOW,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawRoundRect(
            16, 40, 288, 154, 10, 2,
            Graphics::DARKGRAY);

        const int16_t x = static_cast<int16_t>(playerX);
        const int16_t y = static_cast<int16_t>(playerY);

        graphics.fillRoundRect(
            x, y,
            PLAYER_SIZE, PLAYER_SIZE,
            7,
            playerColor());

        graphics.drawRoundRect(
            x, y,
            PLAYER_SIZE, PLAYER_SIZE,
            7, 2,
            Graphics::WHITE);

        graphics.fillCircle(x + 9, y + 10, 2, Graphics::BLACK);
        graphics.fillCircle(x + 19, y + 10, 2, Graphics::BLACK);
        graphics.drawRect(x + 9, y + 19, 10, 2, Graphics::BLACK);

        graphics.drawString(
            "D-PAD: MOVE   A: COLOR",
            160, 205,
            Graphics::WHITE,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "B: RESET   START: PAUSE",
            160, 217,
            Graphics::WHITE,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        if (paused)
        {
            graphics.fillRoundRect(
                70, 83, 180, 74, 10,
                Graphics::DARKGRAY);

            graphics.drawRoundRect(
                70, 83, 180, 74, 10, 2,
                Graphics::WHITE);

            graphics.drawString(
                "PAUSED",
                160, 108,
                Graphics::YELLOW,
                Graphics::SIZE_32B,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);

            graphics.drawString(
                "PRESS START",
                160, 138,
                Graphics::WHITE,
                Graphics::SIZE_13,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);
        }

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

InputBasicsGame app;


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
