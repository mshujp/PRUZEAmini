/*
===============================================================================
 PLAMIOmini Example
 07_Animation
===============================================================================

This example combines three basic animation techniques:

- A scrolling starfield
- A simple two-frame walking character
- Short-lived particles

Controls:
- D-PAD : Move
- A     : Particle burst
- B     : Reset
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

class AnimationGame : public GameMini
{
public:
    const char* getId() const override { return "animation"; }

private:
    static constexpr uint8_t STAR_COUNT = 28;
    static constexpr uint8_t PARTICLE_COUNT = 24;

    static constexpr float FIELD_LEFT = 8.0f;
    static constexpr float FIELD_TOP = 36.0f;
    static constexpr float FIELD_RIGHT = 312.0f;
    static constexpr float FIELD_BOTTOM = 196.0f;

    static constexpr float PLAYER_SPEED = 110.0f;
    static constexpr float PLAYER_HALF_W = 8.0f;
    static constexpr float PLAYER_HALF_H = 10.0f;

    struct Star
    {
        float x;
        float y;
        float speed;
        uint8_t size;
    };

    struct Particle
    {
        float x;
        float y;
        float vx;
        float vy;
        float life;
        float maxLife;
        bool active;
    };

    Star stars[STAR_COUNT] = {};
    Particle particles[PARTICLE_COUNT] = {};

    float playerX = 160.0f;
    float playerY = 116.0f;

    bool facingRight = true;
    bool moving = false;
    bool paused = false;

    uint32_t randomState = 0x13572468u;
    float walkTimer = 0.0f;
    uint8_t walkFrame = 0;

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

    void initializeStars()
    {
        for (uint8_t i = 0; i < STAR_COUNT; ++i)
        {
            stars[i].x = randomRange(FIELD_LEFT, FIELD_RIGHT);
            stars[i].y = randomRange(FIELD_TOP, FIELD_BOTTOM);
            stars[i].speed = randomRange(18.0f, 58.0f);
            stars[i].size = static_cast<uint8_t>(1u + (nextRandom() % 2u));
        }
    }

    void reset()
    {
        playerX = 160.0f;
        playerY = 116.0f;

        facingRight = true;
        moving = false;
        paused = false;

        walkTimer = 0.0f;
        walkFrame = 0;

        for (uint8_t i = 0; i < PARTICLE_COUNT; ++i)
        {
            particles[i].active = false;
        }

        initializeStars();
        dirty = true;
    }

    void createBurst()
    {
        static constexpr float TWO_PI_VALUE = 6.28318531f;

        for (uint8_t i = 0; i < PARTICLE_COUNT; ++i)
        {
            const float angle =
                (static_cast<float>(i) / PARTICLE_COUNT) * TWO_PI_VALUE;

            const float speed = randomRange(36.0f, 82.0f);

            particles[i].x = playerX;
            particles[i].y = playerY;
            particles[i].vx = Math::cos(angle) * speed;
            particles[i].vy = Math::sin(angle) * speed;
            particles[i].life = randomRange(0.45f, 0.85f);
            particles[i].maxLife = particles[i].life;
            particles[i].active = true;
        }
    }

    void updateStars(float deltaSec)
    {
        for (uint8_t i = 0; i < STAR_COUNT; ++i)
        {
            stars[i].x -= stars[i].speed * deltaSec;

            if (stars[i].x < FIELD_LEFT)
            {
                stars[i].x = FIELD_RIGHT;
                stars[i].y = randomRange(FIELD_TOP, FIELD_BOTTOM);
            }
        }
    }

    void updateParticles(float deltaSec)
    {
        for (uint8_t i = 0; i < PARTICLE_COUNT; ++i)
        {
            Particle& particle = particles[i];

            if (!particle.active)
            {
                continue;
            }

            particle.life -= deltaSec;

            if (particle.life <= 0.0f)
            {
                particle.active = false;
                continue;
            }

            particle.x += particle.vx * deltaSec;
            particle.y += particle.vy * deltaSec;

            particle.vx *= 0.97f;
            particle.vy *= 0.97f;
        }
    }

    void updatePlayer(Input& input, float deltaSec)
    {
        float dx = 0.0f;
        float dy = 0.0f;

        if (input.pressed(Input::LEFT))
        {
            dx -= 1.0f;
            facingRight = false;
        }

        if (input.pressed(Input::RIGHT))
        {
            dx += 1.0f;
            facingRight = true;
        }

        if (input.pressed(Input::UP))
        {
            dy -= 1.0f;
        }

        if (input.pressed(Input::DOWN))
        {
            dy += 1.0f;
        }

        moving = dx != 0.0f || dy != 0.0f;

        if (moving)
        {
            Math::normalize(dx, dy);

            playerX += dx * PLAYER_SPEED * deltaSec;
            playerY += dy * PLAYER_SPEED * deltaSec;

            playerX = Math::clamp(
                playerX,
                FIELD_LEFT + PLAYER_HALF_W,
                FIELD_RIGHT - PLAYER_HALF_W);

            playerY = Math::clamp(
                playerY,
                FIELD_TOP + PLAYER_HALF_H,
                FIELD_BOTTOM - PLAYER_HALF_H);

            walkTimer += deltaSec;

            if (walkTimer >= 0.14f)
            {
                walkTimer = 0.0f;
                walkFrame ^= 1u;
            }
        }
        else
        {
            walkTimer = 0.0f;
            walkFrame = 0;
        }
    }

    void drawStars(Graphics& graphics)
    {
        for (uint8_t i = 0; i < STAR_COUNT; ++i)
        {
            const int16_t x = static_cast<int16_t>(stars[i].x);
            const int16_t y = static_cast<int16_t>(stars[i].y);

            if (stars[i].size == 1)
            {
                graphics.drawPixel(x, y, Graphics::LIGHTGRAY);
            }
            else
            {
                graphics.fillRect(x, y, 2, 2, Graphics::WHITE);
            }
        }
    }

    void drawParticles(Graphics& graphics)
    {
        for (uint8_t i = 0; i < PARTICLE_COUNT; ++i)
        {
            const Particle& particle = particles[i];

            if (!particle.active)
            {
                continue;
            }

            const float lifeRatio = particle.life / particle.maxLife;

            const Graphics::Color color =
                lifeRatio > 0.66f ? Graphics::WHITE :
                lifeRatio > 0.33f ? Graphics::YELLOW :
                                    Graphics::ORANGE;

            const int16_t x = static_cast<int16_t>(particle.x);
            const int16_t y = static_cast<int16_t>(particle.y);

            if (lifeRatio > 0.5f)
            {
                graphics.fillCircle(x, y, 2, color);
            }
            else
            {
                graphics.drawPixel(x, y, color);
            }
        }
    }

    void drawPlayer(Graphics& graphics)
    {
        const int16_t x = static_cast<int16_t>(playerX);
        const int16_t y = static_cast<int16_t>(playerY);

        const int16_t step = moving && walkFrame != 0 ? 2 : 0;

        graphics.fillCircle(x, y - 6, 5, Graphics::YELLOW);
        graphics.fillRect(x - 5, y - 1, 10, 10, Graphics::CYAN);

        const int16_t eyeX = facingRight ? x + 2 : x - 2;
        graphics.drawPixel(eyeX, y - 7, Graphics::BLACK);

        graphics.drawLine(
            x - 3, y + 9,
            x - 5 - step, y + 13,
            Graphics::WHITE);

        graphics.drawLine(
            x + 3, y + 9,
            x + 5 + step, y + 13,
            Graphics::WHITE);

        graphics.drawLine(
            x - 5, y + 2,
            x - 9, y + 5 + step,
            Graphics::CYAN);

        graphics.drawLine(
            x + 5, y + 2,
            x + 9, y + 5 - step,
            Graphics::CYAN);
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

        if (input.justPressed(Input::START))
        {
            paused = !paused;
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            reset();
            return;
        }

        if (paused)
        {
            return;
        }

        if (input.justPressed(Input::A))
        {
            createBurst();
        }

        updatePlayer(input, deltaSec);
        updateStars(deltaSec);
        updateParticles(deltaSec);

        dirty = true;
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }

        graphics.fillScreen(Graphics::BLACK);

        graphics.fillRect(0, 0, 320, 34, Graphics::BLUE);

        graphics.drawString(
            "ANIMATION",
            12, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            paused ? "PAUSED" : "RUNNING",
            308, 17,
            paused ? Graphics::YELLOW : Graphics::GREEN,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawRoundRect(
            8, 36, 304, 160, 10, 2,
            Graphics::DARKGRAY);

        drawStars(graphics);
        drawParticles(graphics);
        drawPlayer(graphics);

        if (paused)
        {
            graphics.fillRoundRect(
                105, 92, 110, 48, 8,
                Graphics::DARKGRAY);

            graphics.drawRoundRect(
                105, 92, 110, 48, 8, 2,
                Graphics::WHITE);

            graphics.drawString(
                "PAUSE",
                160, 116,
                Graphics::YELLOW,
                Graphics::SIZE_25B,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);
        }

        graphics.drawString(
            "D-PAD: MOVE   A: BURST",
            160, 207,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "B: RESET   START: PAUSE",
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
// PLAMIOmini objects
// =============================================================================

AnimationGame game;


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
