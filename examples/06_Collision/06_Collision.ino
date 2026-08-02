/*
===============================================================================
 PRUZEAmini Example
 06_Collision
===============================================================================

Move the player into each target to try four collision functions.

Controls:
- D-PAD : Move
- A     : Show / Hide hitboxes
- B     : Reset

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
// Application
// =============================================================================

class CollisionGame : public App
{
public:
    const char* getId() const override { return "collision"; }

private:
    enum Target : uint8_t
    {
        POINT_RECT,
        RECT_RECT,
        CIRCLE_CIRCLE,
        CIRCLE_RECT,
        TARGET_COUNT
    };

    static constexpr float PLAYER_SPEED = 120.0f;
    static constexpr float PLAYER_RADIUS = 9.0f;

    static constexpr float FIELD_LEFT = 10.0f;
    static constexpr float FIELD_TOP = 38.0f;
    static constexpr float FIELD_RIGHT = 310.0f;
    static constexpr float FIELD_BOTTOM = 194.0f;

    float playerX = 160.0f;
    float playerY = 116.0f;

    bool completed[TARGET_COUNT] = {};
    bool showHitboxes = false;

    void reset()
    {
        playerX = 160.0f;
        playerY = 116.0f;

        for (uint8_t i = 0; i < TARGET_COUNT; ++i)
        {
            completed[i] = false;
        }

        dirty = true;
    }

    uint8_t completedCount() const
    {
        uint8_t count = 0;

        for (uint8_t i = 0; i < TARGET_COUNT; ++i)
        {
            if (completed[i])
            {
                ++count;
            }
        }

        return count;
    }

    void updateCollisions()
    {
        const float pointX = playerX;
        const float pointY = playerY;

        const float playerRectX = playerX - PLAYER_RADIUS;
        const float playerRectY = playerY - PLAYER_RADIUS;
        const float playerRectSize = PLAYER_RADIUS * 2.0f;

        if (Collision::pointRect(
                pointX, pointY,
                34.0f, 56.0f, 76.0f, 42.0f))
        {
            completed[POINT_RECT] = true;
        }

        if (Collision::rectRect(
                playerRectX, playerRectY,
                playerRectSize, playerRectSize,
                210.0f, 56.0f, 76.0f, 42.0f))
        {
            completed[RECT_RECT] = true;
        }

        if (Collision::circleCircle(
                playerX, playerY, PLAYER_RADIUS,
                72.0f, 153.0f, 24.0f))
        {
            completed[CIRCLE_CIRCLE] = true;
        }

        if (Collision::circleRect(
                playerX, playerY, PLAYER_RADIUS,
                210.0f, 132.0f, 76.0f, 42.0f))
        {
            completed[CIRCLE_RECT] = true;
        }
    }

    Graphics::Color targetColor(Target target) const
    {
        return completed[target] ? Graphics::GREEN : Graphics::CYAN;
    }

    void drawHeader(Graphics& graphics)
    {
        graphics.fillRect(0, 0, 320, 34, Graphics::BLUE);

        graphics.drawString(
            "COLLISION",
            12, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        char progress[8];
        snprintf(
            progress,
            sizeof(progress),
            "%u/4",
            static_cast<unsigned>(completedCount()));

        graphics.drawString(
            progress,
            308, 17,
            Graphics::YELLOW,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawTargets(Graphics& graphics)
    {
        Graphics::Color color = targetColor(POINT_RECT);

        graphics.drawRect(34, 56, 76, 42, 3, color);
        graphics.drawString(
            "POINT / RECT",
            72, 105,
            color,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(RECT_RECT);

        graphics.fillRect(210, 56, 76, 42, color);
        graphics.drawString(
            "RECT / RECT",
            248, 105,
            color,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(CIRCLE_CIRCLE);

        graphics.drawCircle(72, 153, 24, color);
        graphics.drawCircle(72, 153, 17, color);
        graphics.drawString(
            "CIRCLE / CIRCLE",
            72, 184,
            color,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(CIRCLE_RECT);

        graphics.drawRoundRect(210, 132, 76, 42, 8, 3, color);
        graphics.drawString(
            "CIRCLE / RECT",
            248, 184,
            color,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawPlayer(Graphics& graphics)
    {
        const int16_t x = static_cast<int16_t>(playerX);
        const int16_t y = static_cast<int16_t>(playerY);

        graphics.fillCircle(
            x, y,
            static_cast<uint16_t>(PLAYER_RADIUS),
            Graphics::YELLOW);

        graphics.drawCircle(
            x, y,
            static_cast<uint16_t>(PLAYER_RADIUS),
            Graphics::WHITE);

        graphics.drawLine(x - 4, y, x + 4, y, Graphics::RED);
        graphics.drawLine(x, y - 4, x, y + 4, Graphics::RED);

        if (showHitboxes)
        {
            const int16_t size =
                static_cast<int16_t>(PLAYER_RADIUS * 2.0f);

            graphics.drawRect(
                x - static_cast<int16_t>(PLAYER_RADIUS),
                y - static_cast<int16_t>(PLAYER_RADIUS),
                static_cast<uint16_t>(size),
                static_cast<uint16_t>(size),
                Graphics::MAGENTA);
        }
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        showHitboxes = false;
        reset();
    }

    void onUpdate(
        Input& input,
        Audio& audio,
        Storage& storage,
        float deltaSec) override
    {
        (void)audio;
        (void)storage;

        float dx = 0.0f;
        float dy = 0.0f;

        if (input.pressed(Input::LEFT))
        {
            dx -= 1.0f;
        }

        if (input.pressed(Input::RIGHT))
        {
            dx += 1.0f;
        }

        if (input.pressed(Input::UP))
        {
            dy -= 1.0f;
        }

        if (input.pressed(Input::DOWN))
        {
            dy += 1.0f;
        }

        if (dx != 0.0f || dy != 0.0f)
        {
            Math::normalize(dx, dy);

            playerX += dx * PLAYER_SPEED * deltaSec;
            playerY += dy * PLAYER_SPEED * deltaSec;

            playerX = Math::clamp(
                playerX,
                FIELD_LEFT + PLAYER_RADIUS,
                FIELD_RIGHT - PLAYER_RADIUS);

            playerY = Math::clamp(
                playerY,
                FIELD_TOP + PLAYER_RADIUS,
                FIELD_BOTTOM - PLAYER_RADIUS);

            updateCollisions();
            dirty = true;
        }

        if (input.justPressed(Input::A))
        {
            showHitboxes = !showHitboxes;
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            reset();
        }
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }

        graphics.fillScreen(Graphics::BLACK);

        drawHeader(graphics);

        graphics.drawRoundRect(
            8, 36, 304, 160, 10, 2,
            Graphics::DARKGRAY);

        drawTargets(graphics);
        drawPlayer(graphics);

        if (completedCount() == TARGET_COUNT)
        {
            graphics.fillRoundRect(
                94, 91, 132, 48, 8,
                Graphics::DARKGRAY);

            graphics.drawRoundRect(
                94, 91, 132, 48, 8, 2,
                Graphics::WHITE);

            graphics.drawString(
                "ALL CLEAR!",
                160, 115,
                Graphics::YELLOW,
                Graphics::SIZE_22B,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);
        }

        graphics.drawString(
            "D-PAD: MOVE   A: HITBOX",
            160, 207,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "B: RESET",
            160, 219,
            Graphics::LIGHTGRAY,
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
// PRUZEAmini objects
// =============================================================================

CollisionGame app;


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
