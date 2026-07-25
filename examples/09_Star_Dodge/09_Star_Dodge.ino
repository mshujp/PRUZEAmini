/*
===============================================================================
 PRUZEAmini Example
 09_Star_Dodge
===============================================================================

Dodge falling meteors and survive as long as possible.

This example demonstrates:

- Player movement
- Reusable object arrays
- Random enemy spawning
- Gradually increasing difficulty
- Score and game states

Controls:
- D-PAD : Move
- START : Start / Pause / Resume
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
// Game
// =============================================================================

class StarDodgeGame : public App
{
public:
    const char* getId() const override { return "star_dodge"; }

private:
    enum Mode : uint8_t
    {
        MODE_READY,
        MODE_PLAYING,
        MODE_PAUSED,
        MODE_GAME_OVER
    };

    static constexpr uint8_t BACKGROUND_STAR_COUNT = 32;
    static constexpr uint8_t METEOR_COUNT = 12;

    static constexpr float FIELD_LEFT = 8.0f;
    static constexpr float FIELD_TOP = 36.0f;
    static constexpr float FIELD_RIGHT = 312.0f;
    static constexpr float FIELD_BOTTOM = 196.0f;

    static constexpr float PLAYER_RADIUS = 7.0f;
    static constexpr float PLAYER_SPEED = 125.0f;

    struct BackgroundStar
    {
        float x;
        float y;
        float speed;
        uint8_t size;
    };

    struct Meteor
    {
        float x;
        float y;
        float radius;
        float speed;
        bool active;
    };

    Mode mode = MODE_READY;

    BackgroundStar backgroundStars[BACKGROUND_STAR_COUNT] = {};
    Meteor meteors[METEOR_COUNT] = {};

    float playerX = 160.0f;
    float playerY = 170.0f;

    float spawnTimer = 0.0f;
    float survivalTime = 0.0f;

    uint32_t randomState = 0x9E3779B9u;

    uint32_t nextRandom()
    {
        randomState = randomState * 1664525u + 1013904223u;
        return randomState;
    }

    float randomRange(float minValue, float maxValue)
    {
        const float unit =
            static_cast<float>(nextRandom() & 0xFFFFu) / 65535.0f;

        return minValue + (maxValue - minValue) * unit;
    }

    void initializeBackgroundStars()
    {
        for (uint8_t i = 0; i < BACKGROUND_STAR_COUNT; ++i)
        {
            backgroundStars[i].x =
                randomRange(FIELD_LEFT, FIELD_RIGHT);

            backgroundStars[i].y =
                randomRange(FIELD_TOP, FIELD_BOTTOM);

            backgroundStars[i].speed =
                randomRange(18.0f, 52.0f);

            backgroundStars[i].size =
                static_cast<uint8_t>(1u + nextRandom() % 2u);
        }
    }

    void clearMeteors()
    {
        for (uint8_t i = 0; i < METEOR_COUNT; ++i)
        {
            meteors[i].active = false;
        }
    }

    void reset()
    {
        mode = MODE_READY;

        playerX = 160.0f;
        playerY = 170.0f;

        spawnTimer = 0.0f;
        survivalTime = 0.0f;

        clearMeteors();
        initializeBackgroundStars();

        dirty = true;
    }

    float currentDifficulty() const
    {
        return Math::clamp(
            survivalTime / 45.0f,
            0.0f,
            1.0f);
    }

    float currentSpawnInterval() const
    {
        return Math::lerp(
            0.72f,
            0.25f,
            currentDifficulty());
    }

    float currentMeteorSpeed() const
    {
        return Math::lerp(
            72.0f,
            155.0f,
            currentDifficulty());
    }

    void spawnMeteor()
    {
        for (uint8_t i = 0; i < METEOR_COUNT; ++i)
        {
            if (meteors[i].active)
            {
                continue;
            }

            meteors[i].radius = randomRange(5.0f, 11.0f);

            meteors[i].x = randomRange(
                FIELD_LEFT + meteors[i].radius,
                FIELD_RIGHT - meteors[i].radius);

            meteors[i].y =
                FIELD_TOP - meteors[i].radius - 2.0f;

            meteors[i].speed =
                currentMeteorSpeed() * randomRange(0.85f, 1.18f);

            meteors[i].active = true;
            return;
        }
    }

    void updateBackgroundStars(float deltaSec)
    {
        for (uint8_t i = 0; i < BACKGROUND_STAR_COUNT; ++i)
        {
            BackgroundStar& star = backgroundStars[i];

            star.y += star.speed * deltaSec;

            if (star.y > FIELD_BOTTOM)
            {
                star.y = FIELD_TOP;
                star.x = randomRange(FIELD_LEFT, FIELD_RIGHT);
            }
        }
    }

    void updatePlayer(Input& input, float deltaSec)
    {
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

        if (dx == 0.0f && dy == 0.0f)
        {
            return;
        }

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
    }

    void updateMeteors(float deltaSec)
    {
        for (uint8_t i = 0; i < METEOR_COUNT; ++i)
        {
            Meteor& meteor = meteors[i];

            if (!meteor.active)
            {
                continue;
            }

            meteor.y += meteor.speed * deltaSec;

            if (meteor.y - meteor.radius > FIELD_BOTTOM)
            {
                meteor.active = false;
                continue;
            }

            if (Collision::circleCircle(
                    playerX,
                    playerY,
                    PLAYER_RADIUS,
                    meteor.x,
                    meteor.y,
                    meteor.radius))
            {
                mode = MODE_GAME_OVER;
                return;
            }
        }
    }

    void updateGame(Input& input, float deltaSec)
    {
        survivalTime += deltaSec;
        spawnTimer += deltaSec;

        updateBackgroundStars(deltaSec);
        updatePlayer(input, deltaSec);
        updateMeteors(deltaSec);

        if (mode != MODE_PLAYING)
        {
            return;
        }

        if (spawnTimer >= currentSpawnInterval())
        {
            spawnTimer = 0.0f;
            spawnMeteor();
        }
    }

    uint32_t score() const
    {
        return static_cast<uint32_t>(survivalTime * 10.0f);
    }

    void drawHeader(Graphics& graphics)
    {
        graphics.fillRect(0, 0, 320, 34, Graphics::BLUE);

        graphics.drawString(
            "STAR DODGE",
            12, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        char scoreText[20];

        snprintf(
            scoreText,
            sizeof(scoreText),
            "SCORE %lu",
            static_cast<unsigned long>(score()));

        graphics.drawString(
            scoreText,
            308, 17,
            Graphics::YELLOW,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawBackgroundStars(Graphics& graphics)
    {
        for (uint8_t i = 0; i < BACKGROUND_STAR_COUNT; ++i)
        {
            const BackgroundStar& star = backgroundStars[i];

            const int16_t x = static_cast<int16_t>(star.x);
            const int16_t y = static_cast<int16_t>(star.y);

            if (star.size == 1)
            {
                graphics.drawPixel(x, y, Graphics::GRAY);
            }
            else
            {
                graphics.fillRect(x, y, 2, 2, Graphics::LIGHTGRAY);
            }
        }
    }

    void drawMeteors(Graphics& graphics)
    {
        for (uint8_t i = 0; i < METEOR_COUNT; ++i)
        {
            const Meteor& meteor = meteors[i];

            if (!meteor.active)
            {
                continue;
            }

            const int16_t x = static_cast<int16_t>(meteor.x);
            const int16_t y = static_cast<int16_t>(meteor.y);
            const uint16_t radius =
                static_cast<uint16_t>(meteor.radius);

            graphics.fillCircle(
                x,
                y,
                radius,
                Graphics::ORANGE);

            graphics.drawCircle(
                x,
                y,
                radius,
                Graphics::YELLOW);

            if (radius >= 8)
            {
                graphics.fillCircle(
                    x - 2,
                    y - 2,
                    2,
                    Graphics::RED);
            }
        }
    }

    void drawPlayer(Graphics& graphics)
    {
        const int16_t x = static_cast<int16_t>(playerX);
        const int16_t y = static_cast<int16_t>(playerY);

        graphics.fillTriangle(
            x,
            y - 10,
            x - 8,
            y + 8,
            x + 8,
            y + 8,
            Graphics::CYAN);

        graphics.drawTriangle(
            x,
            y - 10,
            x - 8,
            y + 8,
            x + 8,
            y + 8,
            Graphics::WHITE);

        graphics.fillCircle(
            x,
            y + 2,
            2,
            Graphics::BLUE);
    }

    void drawOverlay(
        Graphics& graphics,
        const char* title,
        const char* subtitle,
        Graphics::Color titleColor)
    {
        graphics.fillRoundRect(
            61, 91, 198, 64, 8,
            Graphics::DARKGRAY);

        graphics.drawRoundRect(
            61, 91, 198, 64, 8, 2,
            Graphics::WHITE);

        graphics.drawString(
            title,
            160, 111,
            titleColor,
            Graphics::SIZE_25B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            subtitle,
            160, 140,
            Graphics::WHITE,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;
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

        if (input.justPressed(Input::B))
        {
            reset();
            return;
        }

        if (input.justPressed(Input::START))
        {
            if (mode == MODE_READY)
            {
                mode = MODE_PLAYING;
                spawnTimer = currentSpawnInterval();
            }
            else if (mode == MODE_PLAYING)
            {
                mode = MODE_PAUSED;
            }
            else if (mode == MODE_PAUSED)
            {
                mode = MODE_PLAYING;
            }
            else
            {
                reset();
                mode = MODE_PLAYING;
                spawnTimer = currentSpawnInterval();
            }

            dirty = true;
        }

        if (mode == MODE_PLAYING)
        {
            updateGame(input, deltaSec);
            dirty = true;
        }
    }

    bool onDraw(
        Graphics& graphics,
        bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }

        graphics.fillScreen(Graphics::BLACK);

        drawHeader(graphics);

        graphics.drawRoundRect(
            8, 36, 304, 160, 8, 2,
            Graphics::DARKGRAY);

        drawBackgroundStars(graphics);
        drawMeteors(graphics);
        drawPlayer(graphics);

        if (mode == MODE_READY)
        {
            drawOverlay(
                graphics,
                "READY",
                "PRESS START",
                Graphics::YELLOW);
        }
        else if (mode == MODE_PAUSED)
        {
            drawOverlay(
                graphics,
                "PAUSE",
                "PRESS START",
                Graphics::YELLOW);
        }
        else if (mode == MODE_GAME_OVER)
        {
            drawOverlay(
                graphics,
                "GAME OVER",
                "START: RETRY",
                Graphics::RED);
        }

        graphics.drawString(
            "D-PAD: MOVE",
            160, 207,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "START: PLAY / PAUSE   B: RESET",
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

StarDodgeGame app;


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
