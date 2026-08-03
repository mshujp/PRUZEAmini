/*
===============================================================================
 PRUZEAmini Example
 11_Memory_Tiles
===============================================================================

A complete 4 x 4 memory matching game.

Controls:
- D-PAD : Move cursor
- A     : Flip tile
- START : Pause / Resume
- B     : Restart after clear

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

static const Audio::ToneNote MEMORY_BGM_NOTES[] = {
    { Audio::ToneNote::C4, Audio::ToneNote::E },
    { Audio::ToneNote::E4, Audio::ToneNote::E },
    { Audio::ToneNote::G4, Audio::ToneNote::E },
    { Audio::ToneNote::E4, Audio::ToneNote::E },
    { Audio::ToneNote::D4, Audio::ToneNote::E },
    { Audio::ToneNote::F4, Audio::ToneNote::E },
    { Audio::ToneNote::A4, Audio::ToneNote::E },
    { Audio::ToneNote::F4, Audio::ToneNote::E },
};

static const Audio::Music MEMORY_BGM = {
    MEMORY_BGM_NOTES,
    126,
    static_cast<uint16_t>(
        sizeof(MEMORY_BGM_NOTES) / sizeof(MEMORY_BGM_NOTES[0])),
    0,
    0.25f,
};

class MemoryTilesGame : public App
{
public:
    const char* getId() const override { return "memory_tiles"; }

private:
    enum TileState : uint8_t
    {
        HIDDEN,
        OPEN,
        MATCHED
    };

    enum Mode : uint8_t
    {
        MODE_PLAYING,
        MODE_WAITING,
        MODE_PAUSED,
        MODE_CLEAR
    };

    static constexpr uint8_t COLUMNS = 4;
    static constexpr uint8_t ROWS = 4;
    static constexpr uint8_t TILE_COUNT = 16;

    static constexpr int16_t TILE_W = 48;
    static constexpr int16_t TILE_H = 35;
    static constexpr int16_t GAP = 6;
    static constexpr int16_t BOARD_X = 40;
    static constexpr int16_t BOARD_Y = 48;

    uint8_t values[TILE_COUNT] = {};
    TileState states[TILE_COUNT] = {};

    uint8_t cursor = 0;
    int8_t firstOpen = -1;
    int8_t secondOpen = -1;

    uint16_t moves = 0;
    uint8_t pairs = 0;

    Mode mode = MODE_PLAYING;
    uint32_t waitStart = 0;
    uint32_t randomState = 0x2468ACE1u;
    bool bgmStarted = false;

    uint32_t nextRandom()
    {
        randomState = randomState * 1664525u + 1013904223u;
        return randomState;
    }

    void shuffleTiles()
    {
        for (uint8_t i = 0; i < TILE_COUNT; ++i)
        {
            values[i] = static_cast<uint8_t>(i / 2);
            states[i] = HIDDEN;
        }

        for (int i = TILE_COUNT - 1; i > 0; --i)
        {
            const uint8_t j =
                static_cast<uint8_t>(nextRandom() % (i + 1));

            const uint8_t temp = values[i];
            values[i] = values[j];
            values[j] = temp;
        }
    }

    void reset()
    {
        cursor = 0;
        firstOpen = -1;
        secondOpen = -1;
        moves = 0;
        pairs = 0;
        mode = MODE_PLAYING;
        waitStart = 0;
        shuffleTiles();
        dirty = true;
    }

    Graphics::Color valueColor(uint8_t value) const
    {
        static const Graphics::Color COLORS[8] = {
            Graphics::RED,
            Graphics::ORANGE,
            Graphics::YELLOW,
            Graphics::GREEN,
            Graphics::CYAN,
            Graphics::BLUE,
            Graphics::PURPLE,
            Graphics::PINK
        };

        return COLORS[value & 7u];
    }

    void moveCursor(Input& input, Audio& audio)
    {
        int x = cursor % COLUMNS;
        int y = cursor / COLUMNS;
        bool moved = false;

        if (input.justPressed(Input::LEFT))
        {
            x = (x + COLUMNS - 1) % COLUMNS;
            moved = true;
        }
        else if (input.justPressed(Input::RIGHT))
        {
            x = (x + 1) % COLUMNS;
            moved = true;
        }
        else if (input.justPressed(Input::UP))
        {
            y = (y + ROWS - 1) % ROWS;
            moved = true;
        }
        else if (input.justPressed(Input::DOWN))
        {
            y = (y + 1) % ROWS;
            moved = true;
        }

        if (moved)
        {
            cursor = static_cast<uint8_t>(y * COLUMNS + x);
            audio.playSE(&Audio::SE::NO_1, 0.2f);
            dirty = true;
        }
    }

    void flipCurrent(Audio& audio)
    {
        if (states[cursor] != HIDDEN)
        {
            audio.playSE(&Audio::SE::NO_13, 0.5f);
            return;
        }

        states[cursor] = OPEN;
        audio.playSE(&Audio::SE::NO_5, 0.4f);

        if (firstOpen < 0)
        {
            firstOpen = static_cast<int8_t>(cursor);
        }
        else
        {
            secondOpen = static_cast<int8_t>(cursor);
            ++moves;
            mode = MODE_WAITING;
            waitStart = Platform::getMsec();
        }

        dirty = true;
    }

    void resolvePair(Audio& audio)
    {
        if (firstOpen < 0 || secondOpen < 0)
        {
            mode = MODE_PLAYING;
            return;
        }

        if (values[firstOpen] == values[secondOpen])
        {
            states[firstOpen] = MATCHED;
            states[secondOpen] = MATCHED;
            ++pairs;
            audio.playSE(&Audio::SE::NO_3, 0.65f);

            if (pairs == TILE_COUNT / 2)
            {
                mode = MODE_CLEAR;
                audio.stopMusic();
                bgmStarted = false;
                audio.playSE(&Audio::SE::NO_8, 0.85f);
            }
            else
            {
                mode = MODE_PLAYING;
            }
        }
        else
        {
            states[firstOpen] = HIDDEN;
            states[secondOpen] = HIDDEN;
            audio.playSE(&Audio::SE::NO_2, 0.55f);
            mode = MODE_PLAYING;
        }

        firstOpen = -1;
        secondOpen = -1;
        dirty = true;
    }

    void drawSymbol(
        Graphics& graphics,
        uint8_t value,
        int16_t cx,
        int16_t cy)
    {
        const Graphics::Color color = valueColor(value);

        switch (value)
        {
            case 0:
                graphics.fillCircle(cx, cy, 9, color);
                break;

            case 1:
                graphics.fillRect(cx - 9, cy - 9, 18, 18, color);
                break;

            case 2:
                graphics.fillTriangle(
                    cx, cy - 11,
                    cx - 11, cy + 9,
                    cx + 11, cy + 9,
                    color);
                break;

            case 3:
                graphics.drawCircle(cx, cy, 10, color);
                graphics.drawCircle(cx, cy, 6, color);
                break;

            case 4:
                graphics.drawLine(cx - 10, cy, cx + 10, cy, color);
                graphics.drawLine(cx, cy - 10, cx, cy + 10, color);
                break;

            case 5:
                graphics.drawLine(
                    cx - 9, cy - 9,
                    cx + 9, cy + 9,
                    color);
                graphics.drawLine(
                    cx + 9, cy - 9,
                    cx - 9, cy + 9,
                    color);
                break;

            case 6:
                graphics.fillCircle(cx - 6, cy, 5, color);
                graphics.fillCircle(cx + 6, cy, 5, color);
                break;

            default:
                graphics.fillRoundRect(
                    cx - 10, cy - 8,
                    20, 16, 5,
                    color);
                break;
        }
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;
        bgmStarted = false;
        reset();
    }

    void onUpdate(
        Input& input,
        Audio& audio,
        Storage& storage,
        float deltaSec) override
    {
        (void)storage;
        (void)deltaSec;

        if (!bgmStarted && mode != MODE_CLEAR && mode != MODE_PAUSED)
        {
            audio.playMusic(&MEMORY_BGM);
            bgmStarted = true;
        }

        if (input.justPressed(Input::START))
        {
            if (mode == MODE_PAUSED)
            {
                mode = MODE_PLAYING;
                audio.playMusic(&MEMORY_BGM);
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

        if (mode == MODE_CLEAR)
        {
            if (input.justPressed(Input::B))
            {
                reset();
            }
            return;
        }

        if (mode == MODE_WAITING)
        {
            if (Platform::elapsed(
                    Platform::getMsec(),
                    waitStart,
                    650))
            {
                resolvePair(audio);
            }
            return;
        }

        moveCursor(input, audio);

        if (input.justPressed(Input::A))
        {
            flipCurrent(audio);
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
            "MEMORY TILES",
            10, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        char hud[24];
        snprintf(
            hud,
            sizeof(hud),
            "PAIRS %u/8  MOVES %u",
            static_cast<unsigned>(pairs),
            static_cast<unsigned>(moves));

        graphics.drawString(
            hud,
            310, 17,
            Graphics::YELLOW,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);

        for (uint8_t i = 0; i < TILE_COUNT; ++i)
        {
            const int16_t x =
                BOARD_X + (i % COLUMNS) * (TILE_W + GAP);

            const int16_t y =
                BOARD_Y + (i / COLUMNS) * (TILE_H + GAP);

            const bool selected =
                i == cursor && mode == MODE_PLAYING;

            if (states[i] == HIDDEN)
            {
                graphics.fillRoundRect(
                    x, y, TILE_W, TILE_H, 6,
                    Graphics::DARKGRAY);

                graphics.drawRoundRect(
                    x, y, TILE_W, TILE_H, 6, 2,
                    selected ? Graphics::YELLOW : Graphics::LIGHTGRAY);

                graphics.drawString(
                    "?",
                    x + TILE_W / 2,
                    y + TILE_H / 2,
                    Graphics::WHITE,
                    Graphics::SIZE_25B,
                    Graphics::HorizontalAlign::CENTER,
                    Graphics::VerticalAlign::MIDDLE);
            }
            else
            {
                graphics.fillRoundRect(
                    x, y, TILE_W, TILE_H, 6,
                    states[i] == MATCHED ?
                        Graphics::GRAY :
                        Graphics::WHITE);

                graphics.drawRoundRect(
                    x, y, TILE_W, TILE_H, 6, 2,
                    selected ? Graphics::YELLOW : valueColor(values[i]));

                drawSymbol(
                    graphics,
                    values[i],
                    x + TILE_W / 2,
                    y + TILE_H / 2);
            }
        }

        if (mode == MODE_PAUSED)
        {
            graphics.fillRoundRect(
                95, 93, 130, 52, 8,
                Graphics::DARKGRAY);

            graphics.drawRoundRect(
                95, 93, 130, 52, 8, 2,
                Graphics::WHITE);

            graphics.drawString(
                "PAUSE",
                160, 119,
                Graphics::YELLOW,
                Graphics::SIZE_25B,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);
        }
        else if (mode == MODE_CLEAR)
        {
            graphics.fillRoundRect(
                68, 88, 184, 70, 8,
                Graphics::DARKGRAY);

            graphics.drawRoundRect(
                68, 88, 184, 70, 8, 2,
                Graphics::WHITE);

            graphics.drawString(
                "ALL CLEAR!",
                160, 108,
                Graphics::YELLOW,
                Graphics::SIZE_25B,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);

            char result[24];
            snprintf(
                result,
                sizeof(result),
                "%u MOVES",
                static_cast<unsigned>(moves));

            graphics.drawString(
                result,
                160, 135,
                Graphics::CYAN,
                Graphics::SIZE_18,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);

            graphics.drawString(
                "B: RESTART",
                160, 151,
                Graphics::WHITE,
                Graphics::SIZE_10,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);
        }

        graphics.drawString(
            "D-PAD: MOVE   A: FLIP   START: PAUSE",
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

MemoryTilesGame app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
