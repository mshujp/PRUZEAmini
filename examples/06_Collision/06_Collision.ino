/*
===============================================================================
 PRUZEAmini Example
 06_Collision
===============================================================================

Try the Collision API in two stages.

PHASE 1: Basic shape collisions
- pointRect
- rectRect
- circleCircle
- circleRect

PHASE 2: Point / line collisions
- pointCircle
- lineLine
- lineRect
- lineCircle

Phase 2 also demonstrates circleRect(..., Vector2& pushOut) for a solid wall
and raycast() for a visible hit point.

Controls:
- D-PAD : Move
- A     : Show / Hide hitboxes
- B     : Reset

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>
#include <cstdio>

using namespace PRUZEAmini;

// =============================================================================
// Hardware configuration
// =============================================================================

GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost         = 0,
    .spiWriteFreq    = 62500000,
    .clkPin          = -1,
    .dataPin         = -1,
    .dcPin           = -1,
    .csPin           = -1,
    .resetPin        = -1,
    .backlightPin    = -1,
    .lcdRotate       = 1,
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
    enum Phase : uint8_t
    {
        PHASE_BASIC,
        PHASE_TRANSITION,
        PHASE_LINE,
        PHASE_COMPLETE
    };

    enum Test : uint8_t
    {
        POINT_RECT,
        RECT_RECT,
        CIRCLE_CIRCLE,
        CIRCLE_RECT,
        POINT_CIRCLE,
        LINE_LINE,
        LINE_RECT,
        LINE_CIRCLE,
        TEST_COUNT
    };

    static constexpr float PLAYER_SPEED = 120.0f;
    static constexpr float PLAYER_RADIUS = 9.0f;

    static constexpr float FIELD_LEFT = 10.0f;
    static constexpr float FIELD_TOP = 38.0f;
    static constexpr float FIELD_RIGHT = 310.0f;
    static constexpr float FIELD_BOTTOM = 194.0f;

    static constexpr uint32_t TRANSITION_MSEC = 1300;

    // Phase 2 solid wall. The pushOut overload keeps the player outside it.
    static constexpr float WALL_X = 142.0f;
    static constexpr float WALL_Y = 104.0f;
    static constexpr float WALL_W = 36.0f;
    static constexpr float WALL_H = 38.0f;

    // raycast() target. This is not a completion test; it shows practical use.
    static constexpr float RAY_BOX_X = 284.0f;
    static constexpr float RAY_BOX_Y = 102.0f;
    static constexpr float RAY_BOX_W = 16.0f;
    static constexpr float RAY_BOX_H = 30.0f;

    Phase phase = PHASE_BASIC;
    float playerX = 160.0f;
    float playerY = 116.0f;
    bool completed[TEST_COUNT] = {};
    bool showHitboxes = false;
    uint32_t transitionStartMsec = 0;

    bool rayHit = false;
    float rayHitX = 0.0f;
    float rayHitY = 0.0f;

    void reset()
    {
        phase = PHASE_BASIC;
        playerX = 160.0f;
        playerY = 116.0f;
        transitionStartMsec = 0;
        rayHit = false;

        for (uint8_t i = 0; i < TEST_COUNT; ++i)
        {
            completed[i] = false;
        }

        dirty = true;
    }

    uint8_t completedCount() const
    {
        uint8_t count = 0;
        for (uint8_t i = 0; i < TEST_COUNT; ++i)
        {
            if (completed[i]) ++count;
        }
        return count;
    }

    bool basicComplete() const
    {
        return completed[POINT_RECT] &&
               completed[RECT_RECT] &&
               completed[CIRCLE_CIRCLE] &&
               completed[CIRCLE_RECT];
    }

    bool lineComplete() const
    {
        return completed[POINT_CIRCLE] &&
               completed[LINE_LINE] &&
               completed[LINE_RECT] &&
               completed[LINE_CIRCLE];
    }

    void beginPhase2()
    {
        phase = PHASE_LINE;
        playerX = 30.0f;
        playerY = 116.0f;
        rayHit = false;
        dirty = true;
    }

    void updateBasicCollisions()
    {
        const float pointX = playerX;
        const float pointY = playerY;
        const float playerRectX = playerX - PLAYER_RADIUS;
        const float playerRectY = playerY - PLAYER_RADIUS;
        const float playerRectSize = PLAYER_RADIUS * 2.0f;

        if (Collision::pointRect(pointX, pointY, 34.0f, 56.0f, 76.0f, 42.0f))
            completed[POINT_RECT] = true;

        if (Collision::rectRect(playerRectX, playerRectY, playerRectSize, playerRectSize,
                                210.0f, 56.0f, 76.0f, 42.0f))
            completed[RECT_RECT] = true;

        if (Collision::circleCircle(playerX, playerY, PLAYER_RADIUS,
                                    72.0f, 153.0f, 24.0f))
            completed[CIRCLE_CIRCLE] = true;

        if (Collision::circleRect(playerX, playerY, PLAYER_RADIUS,
                                  210.0f, 132.0f, 76.0f, 42.0f))
            completed[CIRCLE_RECT] = true;

        if (basicComplete())
        {
            phase = PHASE_TRANSITION;
            transitionStartMsec = Platform::getMsec();
        }
    }

    void resolvePhase2Wall()
    {
        Vector2 pushOut;
        if (Collision::circleRect(playerX, playerY, PLAYER_RADIUS,
                                  WALL_X, WALL_Y, WALL_W, WALL_H, pushOut))
        {
            playerX += pushOut.x;
            playerY += pushOut.y;
        }
    }

    void updateLineCollisions()
    {
        const float pointX = playerX + PLAYER_RADIUS;
        const float pointY = playerY;
        const float laserX1 = pointX;
        const float laserY1 = playerY;
        const float laserX2 = FIELD_RIGHT;
        const float laserY2 = playerY;

        // pointCircle: the red nose point must enter this sensor.
        if (Collision::pointCircle(pointX, pointY, 82.0f, 160.0f, 20.0f))
            completed[POINT_CIRCLE] = true;

        // lineLine: intentionally above the Phase 2 starting Y, so it is not
        // completed automatically when the scene begins.
        if (Collision::lineLine(laserX1, laserY1, laserX2, laserY2,
                                132.0f, 52.0f, 132.0f, 88.0f))
            completed[LINE_LINE] = true;

        if (Collision::lineRect(laserX1, laserY1, laserX2, laserY2,
                                206.0f, 55.0f, 58.0f, 32.0f))
            completed[LINE_RECT] = true;

        if (Collision::lineCircle(laserX1, laserY1, laserX2, laserY2,
                                  236.0f, 166.0f, 20.0f))
            completed[LINE_CIRCLE] = true;

        // raycast(): shoot an infinite right-facing ray at a small rectangle
        // and draw the returned hit point when there is one.
        rayHit = Collision::raycast(playerX, playerY, 1.0f, 0.0f,
                                    RAY_BOX_X, RAY_BOX_Y, RAY_BOX_W, RAY_BOX_H,
                                    rayHitX, rayHitY);

        if (lineComplete())
            phase = PHASE_COMPLETE;
    }

    Graphics::Color targetColor(Test test) const
    {
        return completed[test] ? Graphics::GREEN : Graphics::CYAN;
    }

    void drawHeader(Graphics& graphics)
    {
        graphics.fillRect(0, 0, 320, 34, Graphics::BLUE);
        graphics.drawString("COLLISION", 12, 17, Graphics::WHITE, Graphics::SIZE_22B,
                            Graphics::HorizontalAlign::LEFT,
                            Graphics::VerticalAlign::MIDDLE);

        char progress[12];
        snprintf(progress, sizeof(progress), "%u/8",
                 static_cast<unsigned>(completedCount()));
        graphics.drawString(progress, 308, 17, Graphics::YELLOW, Graphics::SIZE_18,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::MIDDLE);
    }

    void drawBasicTargets(Graphics& graphics)
    {
        Graphics::Color color = targetColor(POINT_RECT);
        graphics.drawRect(34, 56, 76, 42, 3, color);
        graphics.drawString("POINT / RECT", 72, 105, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(RECT_RECT);
        graphics.fillRect(210, 56, 76, 42, color);
        graphics.drawString("RECT / RECT", 248, 105, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(CIRCLE_CIRCLE);
        graphics.drawCircle(72, 153, 24, color);
        graphics.drawCircle(72, 153, 17, color);
        graphics.drawString("CIRCLE / CIRCLE", 72, 184, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(CIRCLE_RECT);
        graphics.drawRoundRect(210, 132, 76, 42, 8, 3, color);
        graphics.drawString("CIRCLE / RECT", 248, 184, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
    }

    void drawLineTargets(Graphics& graphics)
    {
        Graphics::Color color = targetColor(POINT_CIRCLE);
        graphics.drawCircle(82, 160, 20, color);
        graphics.drawString("P-C", 82, 160, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(LINE_LINE);
        graphics.drawLine(132, 52, 132, 88, color);
        graphics.drawLine(128, 52, 136, 52, color);
        graphics.drawLine(128, 88, 136, 88, color);
        graphics.drawString("L-L", 132, 96, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(LINE_RECT);
        graphics.drawRect(206, 55, 58, 32, 2, color);
        graphics.drawString("L-R", 235, 96, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        color = targetColor(LINE_CIRCLE);
        graphics.drawCircle(236, 166, 20, color);
        graphics.drawString("L-C", 236, 166, color, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        // pushOut demonstration wall.
        graphics.fillRect(static_cast<int16_t>(WALL_X), static_cast<int16_t>(WALL_Y),
                          static_cast<uint16_t>(WALL_W), static_cast<uint16_t>(WALL_H),
                          Graphics::DARKGRAY);
        graphics.drawString("PUSH", 160, 123, Graphics::WHITE, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        // raycast target and returned hit marker.
        graphics.drawRect(static_cast<int16_t>(RAY_BOX_X), static_cast<int16_t>(RAY_BOX_Y),
                          static_cast<uint16_t>(RAY_BOX_W), static_cast<uint16_t>(RAY_BOX_H),
                          Graphics::MAGENTA);
        if (rayHit)
        {
            const int16_t hx = static_cast<int16_t>(rayHitX);
            const int16_t hy = static_cast<int16_t>(rayHitY);
            graphics.fillCircle(hx, hy, 3, Graphics::MAGENTA);
        }
    }

    void drawPlayer(Graphics& graphics)
    {
        const int16_t x = static_cast<int16_t>(playerX);
        const int16_t y = static_cast<int16_t>(playerY);

        graphics.fillCircle(x, y, static_cast<uint16_t>(PLAYER_RADIUS), Graphics::YELLOW);
        graphics.drawCircle(x, y, static_cast<uint16_t>(PLAYER_RADIUS), Graphics::WHITE);

        const int16_t pointX = x + static_cast<int16_t>(PLAYER_RADIUS);
        graphics.drawLine(pointX - 2, y, pointX + 2, y, Graphics::RED);
        graphics.drawLine(pointX, y - 2, pointX, y + 2, Graphics::RED);

        if (phase == PHASE_LINE || phase == PHASE_COMPLETE)
            graphics.drawLine(pointX, y, static_cast<int16_t>(FIELD_RIGHT), y, Graphics::YELLOW);

        if (showHitboxes)
        {
            const uint16_t size = static_cast<uint16_t>(PLAYER_RADIUS * 2.0f);
            graphics.drawRect(x - static_cast<int16_t>(PLAYER_RADIUS),
                              y - static_cast<int16_t>(PLAYER_RADIUS),
                              size, size, Graphics::MAGENTA);
        }
    }

    void drawTransition(Graphics& graphics)
    {
        graphics.fillRectAlpha(0, 34, 320, 162, 210, Graphics::BLACK);
        graphics.drawString("PHASE 1 COMPLETE", 160, 92, Graphics::GREEN,
                            Graphics::SIZE_18,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
        graphics.drawString("PHASE 2", 160, 121, Graphics::YELLOW,
                            Graphics::SIZE_25B,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
        graphics.drawString("LINE TEST AREA", 160, 150, Graphics::WHITE,
                            Graphics::SIZE_18,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;
        showHitboxes = false;
        reset();
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)audio;
        (void)storage;
        const uint32_t now = Platform::getMsec();

        if (input.justPressed(Input::A))
        {
            showHitboxes = !showHitboxes;
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            reset();
            return;
        }

        if (phase == PHASE_TRANSITION)
        {
            if (Platform::elapsed(now, transitionStartMsec, TRANSITION_MSEC))
                beginPhase2();
            else
                dirty = true;
            return;
        }

        float dx = 0.0f;
        float dy = 0.0f;
        if (input.pressed(Input::LEFT))  dx -= 1.0f;
        if (input.pressed(Input::RIGHT)) dx += 1.0f;
        if (input.pressed(Input::UP))    dy -= 1.0f;
        if (input.pressed(Input::DOWN))  dy += 1.0f;

        if (dx != 0.0f || dy != 0.0f)
        {
            Math::normalize(dx, dy);
            playerX += dx * PLAYER_SPEED * deltaSec;
            playerY += dy * PLAYER_SPEED * deltaSec;

            playerX = Math::clamp(playerX, FIELD_LEFT + PLAYER_RADIUS,
                                  FIELD_RIGHT - PLAYER_RADIUS);
            playerY = Math::clamp(playerY, FIELD_TOP + PLAYER_RADIUS,
                                  FIELD_BOTTOM - PLAYER_RADIUS);

            if (phase == PHASE_BASIC)
                updateBasicCollisions();
            else if (phase == PHASE_LINE)
            {
                resolvePhase2Wall();
                updateLineCollisions();
            }

            dirty = true;
        }
        else if (phase == PHASE_LINE)
        {
            // Keep raycast output current even while the player is stationary.
            updateLineCollisions();
        }
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty) return false;

        graphics.fillScreen(Graphics::BLACK);
        drawHeader(graphics);
        graphics.drawRoundRect(8, 36, 304, 160, 10, 2, Graphics::DARKGRAY);

        if (phase == PHASE_BASIC || phase == PHASE_TRANSITION)
            drawBasicTargets(graphics);
        else
            drawLineTargets(graphics);

        drawPlayer(graphics);

        if (phase == PHASE_TRANSITION)
            drawTransition(graphics);
        else if (phase == PHASE_COMPLETE)
        {
            graphics.fillRectAlpha(0, 34, 320, 162, 190, Graphics::BLACK);
            graphics.drawString("8 / 8 COMPLETE", 160, 105, Graphics::GREEN,
                                Graphics::SIZE_25B,
                                Graphics::HorizontalAlign::CENTER,
                                Graphics::VerticalAlign::MIDDLE);
            graphics.drawString("B: RESET", 160, 139, Graphics::YELLOW,
                                Graphics::SIZE_13,
                                Graphics::HorizontalAlign::CENTER,
                                Graphics::VerticalAlign::MIDDLE);
        }

        graphics.drawString("D-PAD: MOVE   A: HITBOX", 160, 207,
                            Graphics::LIGHTGRAY, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
        graphics.drawString("B: RESET", 160, 219,
                            Graphics::LIGHTGRAY, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override { (void)storage; }
};

CollisionGame app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop() {}
