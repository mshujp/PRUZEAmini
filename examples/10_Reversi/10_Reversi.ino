/*
===============================================================================
 PRUZEAmini Example
 10_Reversi
===============================================================================

A complete Reversi game against a simple CPU.

Controls:
- D-PAD : Move cursor
- A     : Place a disc
- START : Pause / Resume
- B     : Restart after game over

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>

using namespace PRUZEAmini;

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

AudioConfig audioConfig = AudioPWMConfig{
    .pwmPin = -1,
};

StorageConfig storageConfig = StorageStubConfig{};

static const Audio::ToneNote REVERSI_BGM_NOTES[] = {
    { Audio::ToneNote::C4, Audio::ToneNote::E },
    { Audio::ToneNote::G4, Audio::ToneNote::E },
    { Audio::ToneNote::E4, Audio::ToneNote::E },
    { Audio::ToneNote::G4, Audio::ToneNote::E },
    { Audio::ToneNote::D4, Audio::ToneNote::E },
    { Audio::ToneNote::A4, Audio::ToneNote::E },
    { Audio::ToneNote::F4, Audio::ToneNote::E },
    { Audio::ToneNote::A4, Audio::ToneNote::E },
};

static const Audio::Music REVERSI_BGM = {
    REVERSI_BGM_NOTES,
    112,
    static_cast<uint16_t>(
        sizeof(REVERSI_BGM_NOTES) / sizeof(REVERSI_BGM_NOTES[0])),
    0,
    0.28f,
};

class ReversiGame : public App
{
public:
    const char* getId() const override { return "reversi"; }

private:
    enum Cell : uint8_t
    {
        EMPTY,
        BLACK_DISC,
        WHITE_DISC
    };

    enum Mode : uint8_t
    {
        MODE_PLAYING,
        MODE_CPU_WAIT,
        MODE_PAUSED,
        MODE_GAME_OVER
    };

    static constexpr uint8_t BOARD_SIZE = 8;
    static constexpr int16_t CELL_SIZE = 23;
    static constexpr int16_t BOARD_X = 10;
    static constexpr int16_t BOARD_Y = 38;

    Cell board[BOARD_SIZE][BOARD_SIZE] = {};
    uint8_t cursorX = 2;
    uint8_t cursorY = 3;
    Mode mode = MODE_PLAYING;
    uint32_t cpuWaitStart = 0;
    bool bgmStarted = false;

    bool inside(int x, int y) const
    {
        return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
    }

    Cell opponent(Cell disc) const
    {
        return disc == BLACK_DISC ? WHITE_DISC : BLACK_DISC;
    }

    uint8_t flipsInDirection(
        int x, int y, int dx, int dy, Cell disc) const
    {
        const Cell other = opponent(disc);
        int px = x + dx;
        int py = y + dy;
        uint8_t count = 0;

        while (inside(px, py) && board[py][px] == other)
        {
            ++count;
            px += dx;
            py += dy;
        }

        if (count > 0 && inside(px, py) && board[py][px] == disc)
        {
            return count;
        }

        return 0;
    }

    bool isLegalMove(int x, int y, Cell disc) const
    {
        if (!inside(x, y) || board[y][x] != EMPTY)
        {
            return false;
        }

        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0)
                {
                    continue;
                }

                if (flipsInDirection(x, y, dx, dy, disc) > 0)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool hasLegalMove(Cell disc) const
    {
        for (uint8_t y = 0; y < BOARD_SIZE; ++y)
        {
            for (uint8_t x = 0; x < BOARD_SIZE; ++x)
            {
                if (isLegalMove(x, y, disc))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void placeDisc(int x, int y, Cell disc)
    {
        board[y][x] = disc;

        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0)
                {
                    continue;
                }

                const uint8_t flipCount =
                    flipsInDirection(x, y, dx, dy, disc);

                for (uint8_t i = 1; i <= flipCount; ++i)
                {
                    board[y + dy * i][x + dx * i] = disc;
                }
            }
        }
    }

    void countDiscs(uint8_t& black, uint8_t& white) const
    {
        black = 0;
        white = 0;

        for (uint8_t y = 0; y < BOARD_SIZE; ++y)
        {
            for (uint8_t x = 0; x < BOARD_SIZE; ++x)
            {
                if (board[y][x] == BLACK_DISC)
                {
                    ++black;
                }
                else if (board[y][x] == WHITE_DISC)
                {
                    ++white;
                }
            }
        }
    }

    void resetBoard()
    {
        for (uint8_t y = 0; y < BOARD_SIZE; ++y)
        {
            for (uint8_t x = 0; x < BOARD_SIZE; ++x)
            {
                board[y][x] = EMPTY;
            }
        }

        board[3][3] = WHITE_DISC;
        board[3][4] = BLACK_DISC;
        board[4][3] = BLACK_DISC;
        board[4][4] = WHITE_DISC;

        cursorX = 2;
        cursorY = 3;
        mode = MODE_PLAYING;
        cpuWaitStart = 0;
        dirty = true;
    }

    void finishOrPass(Audio& audio)
    {
        if (hasLegalMove(WHITE_DISC))
        {
            mode = MODE_CPU_WAIT;
            cpuWaitStart = Platform::getMsec();
            return;
        }

        if (hasLegalMove(BLACK_DISC))
        {
            audio.playSE(&Audio::SE::NO_13, 0.55f);
            mode = MODE_PLAYING;
            return;
        }

        mode = MODE_GAME_OVER;
        audio.stopMusic();
        bgmStarted = false;
        audio.playSE(&Audio::SE::NO_8, 0.8f);
    }

    void cpuMove(Audio& audio)
    {
        int bestX = -1;
        int bestY = -1;
        uint8_t bestScore = 0;

        for (uint8_t y = 0; y < BOARD_SIZE; ++y)
        {
            for (uint8_t x = 0; x < BOARD_SIZE; ++x)
            {
                if (!isLegalMove(x, y, WHITE_DISC))
                {
                    continue;
                }

                uint8_t score = 0;

                for (int dy = -1; dy <= 1; ++dy)
                {
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        if (dx != 0 || dy != 0)
                        {
                            score += flipsInDirection(
                                x, y, dx, dy, WHITE_DISC);
                        }
                    }
                }

                if ((x == 0 || x == 7) && (y == 0 || y == 7))
                {
                    score = static_cast<uint8_t>(score + 20);
                }

                if (bestX < 0 || score > bestScore)
                {
                    bestX = x;
                    bestY = y;
                    bestScore = score;
                }
            }
        }

        if (bestX >= 0)
        {
            placeDisc(bestX, bestY, WHITE_DISC);
            audio.playSE(&Audio::SE::NO_2, 0.45f);
        }

        if (hasLegalMove(BLACK_DISC))
        {
            mode = MODE_PLAYING;
        }
        else if (hasLegalMove(WHITE_DISC))
        {
            audio.playSE(&Audio::SE::NO_13, 0.55f);
            mode = MODE_CPU_WAIT;
            cpuWaitStart = Platform::getMsec();
        }
        else
        {
            mode = MODE_GAME_OVER;
            audio.stopMusic();
            bgmStarted = false;
            audio.playSE(&Audio::SE::NO_8, 0.8f);
        }

        dirty = true;
    }

    void moveCursor(Input& input, Audio& audio)
    {
        int dx = 0;
        int dy = 0;

        if (input.justPressed(Input::LEFT))  dx = -1;
        if (input.justPressed(Input::RIGHT)) dx = 1;
        if (input.justPressed(Input::UP))    dy = -1;
        if (input.justPressed(Input::DOWN))  dy = 1;

        if (dx != 0 || dy != 0)
        {
            cursorX = static_cast<uint8_t>(
                (cursorX + BOARD_SIZE + dx) % BOARD_SIZE);
            cursorY = static_cast<uint8_t>(
                (cursorY + BOARD_SIZE + dy) % BOARD_SIZE);

            audio.playSE(&Audio::SE::NO_1, 0.22f);
            dirty = true;
        }
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;
        bgmStarted = false;
        resetBoard();
    }

    void onUpdate(
        Input& input,
        Audio& audio,
        Storage& storage,
        float deltaSec) override
    {
        (void)storage;
        (void)deltaSec;

        if (!bgmStarted && mode != MODE_GAME_OVER && mode != MODE_PAUSED)
        {
            audio.playMusic(&REVERSI_BGM);
            bgmStarted = true;
        }

        if (input.justPressed(Input::START))
        {
            if (mode == MODE_PAUSED)
            {
                mode = MODE_PLAYING;
                audio.playMusic(&REVERSI_BGM);
                bgmStarted = true;
            }
            else if (mode == MODE_PLAYING)
            {
                mode = MODE_PAUSED;
                audio.stopMusic();
                bgmStarted = false;
            }

            dirty = true;
            return;
        }

        if (mode == MODE_PAUSED)
        {
            return;
        }

        if (mode == MODE_GAME_OVER)
        {
            if (input.justPressed(Input::B))
            {
                resetBoard();
            }
            return;
        }

        if (mode == MODE_CPU_WAIT)
        {
            if (Platform::elapsed(
                    Platform::getMsec(),
                    cpuWaitStart,
                    450))
            {
                cpuMove(audio);
            }
            return;
        }

        moveCursor(input, audio);

        if (input.justPressed(Input::A))
        {
            if (isLegalMove(cursorX, cursorY, BLACK_DISC))
            {
                placeDisc(cursorX, cursorY, BLACK_DISC);
                audio.playSE(&Audio::SE::NO_3, 0.55f);
                finishOrPass(audio);
            }
            else
            {
                audio.playSE(&Audio::SE::NO_13, 0.55f);
            }

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
        graphics.fillRect(0, 0, 320, 34, Graphics::BLUE);

        graphics.drawString(
            "REVERSI",
            10, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        uint8_t black = 0;
        uint8_t white = 0;
        countDiscs(black, white);

        char scoreText[24];
        snprintf(
            scoreText,
            sizeof(scoreText),
            "YOU %u  CPU %u",
            static_cast<unsigned>(black),
            static_cast<unsigned>(white));

        graphics.drawString(
            scoreText,
            310, 17,
            Graphics::YELLOW,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);

        graphics.fillRect(
            BOARD_X,
            BOARD_Y,
            CELL_SIZE * BOARD_SIZE,
            CELL_SIZE * BOARD_SIZE,
            Graphics::GREEN);

        for (uint8_t i = 0; i <= BOARD_SIZE; ++i)
        {
            const int16_t p = i * CELL_SIZE;

            graphics.drawLine(
                BOARD_X + p,
                BOARD_Y,
                BOARD_X + p,
                BOARD_Y + CELL_SIZE * BOARD_SIZE,
                Graphics::BLACK);

            graphics.drawLine(
                BOARD_X,
                BOARD_Y + p,
                BOARD_X + CELL_SIZE * BOARD_SIZE,
                BOARD_Y + p,
                Graphics::BLACK);
        }

        for (uint8_t y = 0; y < BOARD_SIZE; ++y)
        {
            for (uint8_t x = 0; x < BOARD_SIZE; ++x)
            {
                const int16_t cx =
                    BOARD_X + x * CELL_SIZE + CELL_SIZE / 2;

                const int16_t cy =
                    BOARD_Y + y * CELL_SIZE + CELL_SIZE / 2;

                if (board[y][x] == BLACK_DISC)
                {
                    graphics.fillCircle(cx, cy, 9, Graphics::BLACK);
                    graphics.drawCircle(cx, cy, 9, Graphics::LIGHTGRAY);
                }
                else if (board[y][x] == WHITE_DISC)
                {
                    graphics.fillCircle(cx, cy, 9, Graphics::WHITE);
                    graphics.drawCircle(cx, cy, 9, Graphics::DARKGRAY);
                }
                else if (mode == MODE_PLAYING &&
                         isLegalMove(x, y, BLACK_DISC))
                {
                    graphics.fillCircle(cx, cy, 2, Graphics::YELLOW);
                }
            }
        }

        const int16_t cursorPX =
            BOARD_X + cursorX * CELL_SIZE;

        const int16_t cursorPY =
            BOARD_Y + cursorY * CELL_SIZE;

        if (mode == MODE_PLAYING)
        {
            graphics.drawRect(
                cursorPX + 2,
                cursorPY + 2,
                CELL_SIZE - 4,
                CELL_SIZE - 4,
                2,
                Graphics::YELLOW);
        }

        graphics.drawString(
            mode == MODE_CPU_WAIT ? "CPU THINKING..." :
            mode == MODE_GAME_OVER ? "GAME OVER" :
            mode == MODE_PAUSED ? "PAUSED" :
            "A: PLACE",
            252, 68,
            mode == MODE_GAME_OVER ? Graphics::RED : Graphics::WHITE,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        if (mode == MODE_GAME_OVER)
        {
            const char* result =
                black > white ? "YOU WIN!" :
                black < white ? "CPU WINS" :
                                "DRAW";

            graphics.drawString(
                result,
                252, 104,
                black > white ? Graphics::YELLOW : Graphics::CYAN,
                Graphics::SIZE_25B,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);

            graphics.drawString(
                "B: RESTART",
                252, 139,
                Graphics::LIGHTGRAY,
                Graphics::SIZE_13,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);
        }
        else
        {
            graphics.drawString(
                "BLACK: YOU",
                252, 108,
                Graphics::LIGHTGRAY,
                Graphics::SIZE_13,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);

            graphics.drawString(
                "WHITE: CPU",
                252, 130,
                Graphics::LIGHTGRAY,
                Graphics::SIZE_13,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);

            graphics.drawString(
                "START: PAUSE",
                252, 168,
                Graphics::LIGHTGRAY,
                Graphics::SIZE_10,
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

ReversiGame app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
