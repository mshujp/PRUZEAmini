/*
===============================================================================
 PLAMIOmini Example
 08_Breakout
===============================================================================

A small block-breaking game demonstrating:

- Ball movement and reflection
- Circle / rectangle collision
- Array-based block management
- Score and game states

Controls:
- LEFT / RIGHT : Move paddle
- START        : Start / Pause / Resume
- B            : Reset

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

class BreakoutGame : public GameMini
{
public:
    const char* getId() const override { return "breakout"; }

private:
    enum Mode : uint8_t
    {
        MODE_READY,
        MODE_PLAYING,
        MODE_PAUSED,
        MODE_CLEAR,
        MODE_GAME_OVER
    };

    static constexpr uint8_t BLOCK_COLUMNS = 8;
    static constexpr uint8_t BLOCK_ROWS = 4;
    static constexpr uint8_t BLOCK_COUNT =
        BLOCK_COLUMNS * BLOCK_ROWS;

    static constexpr float FIELD_LEFT = 8.0f;
    static constexpr float FIELD_TOP = 36.0f;
    static constexpr float FIELD_RIGHT = 312.0f;
    static constexpr float FIELD_BOTTOM = 196.0f;

    static constexpr float PADDLE_WIDTH = 58.0f;
    static constexpr float PADDLE_HEIGHT = 8.0f;
    static constexpr float PADDLE_Y = 181.0f;
    static constexpr float PADDLE_SPEED = 190.0f;

    static constexpr float BALL_RADIUS = 4.0f;
    static constexpr float BALL_SPEED = 128.0f;

    static constexpr float BLOCK_WIDTH = 33.0f;
    static constexpr float BLOCK_HEIGHT = 13.0f;
    static constexpr float BLOCK_GAP = 3.0f;
    static constexpr float BLOCK_START_X = 17.0f;
    static constexpr float BLOCK_START_Y = 48.0f;

    Mode mode = MODE_READY;

    float paddleX = 131.0f;

    float ballX = 160.0f;
    float ballY = 168.0f;
    float ballVX = 90.0f;
    float ballVY = -90.0f;

    bool blocks[BLOCK_COUNT] = {};
    uint16_t score = 0;

    void initializeBlocks()
    {
        for (uint8_t i = 0; i < BLOCK_COUNT; ++i)
        {
            blocks[i] = true;
        }
    }

    void attachBallToPaddle()
    {
        ballX = paddleX + PADDLE_WIDTH * 0.5f;
        ballY = PADDLE_Y - BALL_RADIUS - 2.0f;
    }

    void reset()
    {
        mode = MODE_READY;
        paddleX = 131.0f;

        ballVX = 90.0f;
        ballVY = -90.0f;

        score = 0;
        initializeBlocks();
        attachBallToPaddle();

        dirty = true;
    }

    uint8_t remainingBlocks() const
    {
        uint8_t count = 0;

        for (uint8_t i = 0; i < BLOCK_COUNT; ++i)
        {
            if (blocks[i])
            {
                ++count;
            }
        }

        return count;
    }

    Graphics::Color blockColor(uint8_t row) const
    {
        static const Graphics::Color COLORS[BLOCK_ROWS] = {
            Graphics::RED,
            Graphics::ORANGE,
            Graphics::YELLOW,
            Graphics::GREEN
        };

        return COLORS[row];
    }

    void startBall()
    {
        mode = MODE_PLAYING;

        ballVX = BALL_SPEED * 0.72f;
        ballVY = -BALL_SPEED * 0.70f;
    }

    void updatePaddle(Input& input, float deltaSec)
    {
        float direction = 0.0f;

        if (input.pressed(Input::LEFT))
        {
            direction -= 1.0f;
        }

        if (input.pressed(Input::RIGHT))
        {
            direction += 1.0f;
        }

        if (direction != 0.0f)
        {
            paddleX += direction * PADDLE_SPEED * deltaSec;

            paddleX = Math::clamp(
                paddleX,
                FIELD_LEFT,
                FIELD_RIGHT - PADDLE_WIDTH);

            if (mode == MODE_READY)
            {
                attachBallToPaddle();
            }

            dirty = true;
        }
    }

    void reflectFromPaddle()
    {
        const float paddleCenter =
            paddleX + PADDLE_WIDTH * 0.5f;

        float offset =
            (ballX - paddleCenter) / (PADDLE_WIDTH * 0.5f);

        offset = Math::clamp(offset, -1.0f, 1.0f);

        ballVX = offset * BALL_SPEED;
        ballVY = -BALL_SPEED * (1.0f - 0.30f * fabsf(offset));

        ballY = PADDLE_Y - BALL_RADIUS - 0.5f;
    }

    void updateBall(float deltaSec)
    {
        const float previousX = ballX;
        const float previousY = ballY;

        ballX += ballVX * deltaSec;
        ballY += ballVY * deltaSec;

        if (ballX - BALL_RADIUS < FIELD_LEFT)
        {
            ballX = FIELD_LEFT + BALL_RADIUS;
            ballVX = fabsf(ballVX);
        }
        else if (ballX + BALL_RADIUS > FIELD_RIGHT)
        {
            ballX = FIELD_RIGHT - BALL_RADIUS;
            ballVX = -fabsf(ballVX);
        }

        if (ballY - BALL_RADIUS < FIELD_TOP)
        {
            ballY = FIELD_TOP + BALL_RADIUS;
            ballVY = fabsf(ballVY);
        }

        if (ballVY > 0.0f &&
            Collision::circleRect(
                ballX, ballY, BALL_RADIUS,
                paddleX, PADDLE_Y,
                PADDLE_WIDTH, PADDLE_HEIGHT))
        {
            reflectFromPaddle();
        }

        for (uint8_t row = 0; row < BLOCK_ROWS; ++row)
        {
            for (uint8_t column = 0;
                 column < BLOCK_COLUMNS;
                 ++column)
            {
                const uint8_t index =
                    row * BLOCK_COLUMNS + column;

                if (!blocks[index])
                {
                    continue;
                }

                const float blockX =
                    BLOCK_START_X +
                    column * (BLOCK_WIDTH + BLOCK_GAP);

                const float blockY =
                    BLOCK_START_Y +
                    row * (BLOCK_HEIGHT + BLOCK_GAP);

                if (!Collision::circleRect(
                        ballX, ballY, BALL_RADIUS,
                        blockX, blockY,
                        BLOCK_WIDTH, BLOCK_HEIGHT))
                {
                    continue;
                }

                blocks[index] = false;
                ++score;

                const bool cameFromLeft =
                    previousX + BALL_RADIUS <= blockX;

                const bool cameFromRight =
                    previousX - BALL_RADIUS >= blockX + BLOCK_WIDTH;

                if (cameFromLeft || cameFromRight)
                {
                    ballVX = -ballVX;
                    ballX = previousX;
                }
                else
                {
                    ballVY = -ballVY;
                    ballY = previousY;
                }

                if (remainingBlocks() == 0)
                {
                    mode = MODE_CLEAR;
                }

                return;
            }
        }

        if (ballY - BALL_RADIUS > FIELD_BOTTOM)
        {
            mode = MODE_GAME_OVER;
        }
    }

    void drawHeader(Graphics& graphics)
    {
        graphics.fillRect(0, 0, 320, 34, Graphics::BLUE);

        graphics.drawString(
            "BREAKOUT",
            12, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        char scoreText[20];
        snprintf(
            scoreText,
            sizeof(scoreText),
            "SCORE %u",
            static_cast<unsigned>(score));

        graphics.drawString(
            scoreText,
            308, 17,
            Graphics::YELLOW,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawBlocks(Graphics& graphics)
    {
        for (uint8_t row = 0; row < BLOCK_ROWS; ++row)
        {
            for (uint8_t column = 0;
                 column < BLOCK_COLUMNS;
                 ++column)
            {
                const uint8_t index =
                    row * BLOCK_COLUMNS + column;

                if (!blocks[index])
                {
                    continue;
                }

                const int16_t x = static_cast<int16_t>(
                    BLOCK_START_X +
                    column * (BLOCK_WIDTH + BLOCK_GAP));

                const int16_t y = static_cast<int16_t>(
                    BLOCK_START_Y +
                    row * (BLOCK_HEIGHT + BLOCK_GAP));

                graphics.fillRoundRect(
                    x, y,
                    static_cast<uint16_t>(BLOCK_WIDTH),
                    static_cast<uint16_t>(BLOCK_HEIGHT),
                    3,
                    blockColor(row));
            }
        }
    }

    void drawPlayField(Graphics& graphics)
    {
        graphics.drawRoundRect(
            8, 36, 304, 160, 8, 2,
            Graphics::DARKGRAY);

        drawBlocks(graphics);

        graphics.fillRoundRect(
            static_cast<int16_t>(paddleX),
            static_cast<int16_t>(PADDLE_Y),
            static_cast<uint16_t>(PADDLE_WIDTH),
            static_cast<uint16_t>(PADDLE_HEIGHT),
            3,
            Graphics::CYAN);

        graphics.fillCircle(
            static_cast<int16_t>(ballX),
            static_cast<int16_t>(ballY),
            static_cast<uint16_t>(BALL_RADIUS),
            Graphics::WHITE);
    }

    void drawOverlay(
        Graphics& graphics,
        const char* title,
        const char* subtitle,
        Graphics::Color color)
    {
        graphics.fillRoundRect(
            65, 94, 190, 58, 8,
            Graphics::DARKGRAY);

        graphics.drawRoundRect(
            65, 94, 190, 58, 8, 2,
            Graphics::WHITE);

        graphics.drawString(
            title,
            160, 111,
            color,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            subtitle,
            160, 137,
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
                startBall();
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
            }

            dirty = true;
        }

        if (mode == MODE_READY ||
            mode == MODE_PLAYING)
        {
            updatePaddle(input, deltaSec);
        }

        if (mode == MODE_PLAYING)
        {
            updateBall(deltaSec);
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
        drawPlayField(graphics);

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
        else if (mode == MODE_CLEAR)
        {
            drawOverlay(
                graphics,
                "ALL CLEAR!",
                "START: RESTART",
                Graphics::GREEN);
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
            "LEFT / RIGHT: MOVE",
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
// PLAMIOmini objects
// =============================================================================

BreakoutGame game;


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
