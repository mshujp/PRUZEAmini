/*
===============================================================================
 PRUZEAmini Example
 18_MIDI_Music_Box
===============================================================================

A music-box-style player for embedded Standard MIDI File data.

Demonstrates:
- Playing embedded SMF Format 1 data
- Selecting from multiple MIDI songs
- Starting, stopping, and switching songs
- Animating the screen while MIDI music is playing

Controls:
- UP / DOWN : Select a song
- A / START : Play / Stop
- B         : Stop

MIDI data:
  https://www.ne.jp/asahi/music/myuu/midi/midi.htm

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
===============================================================================
*/

#include <PRUZEAmini.h>
#include "MidiAssets.h"

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


// =============================================================================
// MIDI music data
// =============================================================================

namespace
{
constexpr int16_t SCREEN_W = 320;
constexpr int16_t SCREEN_H = 240;
constexpr uint8_t SONG_COUNT = 3;

struct SongInfo
{
    const char* title;
    const char* subtitle;
};

static const SongInfo SONGS[SONG_COUNT] = {
    { "FURUSATO", "JAPANESE SONG" },
    { "CANON", "PACHELBEL" },
    { "HIMAWARI", "myuu" }
};

static const Audio::Midi MIDI_SONGS[SONG_COUNT] = {
    { FURUSATO_MIDI_DATA, FURUSATO_MIDI_SIZE, 0, 0.72f },
    { CANON_MIDI_DATA, CANON_MIDI_SIZE, 0, 0.68f },
    { HIMAWARI_MIDI_DATA, HIMAWARI_MIDI_SIZE, 0, 0.70f }
};

void drawMusicNote(
    Graphics& graphics,
    int16_t x,
    int16_t y,
    int16_t height,
    Graphics::Color color)
{
    graphics.fillCircle(x, y, 4, color);
    graphics.fillRect(x + 3, y - height, 2, height, color);
    graphics.drawLine(x + 5, y - height, x + 11, y - height + 4, color);
}

} // namespace


// =============================================================================
// App
// =============================================================================

class MidiMusicBoxApp : public App
{
public:
    const char* getId() const override
    {
        return "midi_music_box";
    }

private:
    uint8_t selectedSong = 0;
    bool playing = false;
    uint32_t animationStartMsec = 0;

    void selectPrevious(Audio& audio)
    {
        selectedSong = static_cast<uint8_t>(
            (selectedSong + SONG_COUNT - 1) % SONG_COUNT);

        if (playing)
        {
            playSelected(audio);
        }

        dirty = true;
    }

    void selectNext(Audio& audio)
    {
        selectedSong = static_cast<uint8_t>((selectedSong + 1) % SONG_COUNT);

        if (playing)
        {
            playSelected(audio);
        }

        dirty = true;
    }

    void playSelected(Audio& audio)
    {
        audio.playMidi(&MIDI_SONGS[selectedSong]);
        playing = true;
        animationStartMsec = Platform::getMsec();
        dirty = true;
    }

    void stop(Audio& audio)
    {
        if (!playing)
        {
            return;
        }

        audio.stopMusic();
        playing = false;
        dirty = true;
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        selectedSong = 0;
        playing = false;
        animationStartMsec = Platform::getMsec();
        dirty = true;
    }

    void onUpdate(
        Input& input,
        Audio& audio,
        Storage& storage,
        float deltaSec) override
    {
        (void)storage;
        (void)deltaSec;

        if (input.justPressed(Input::UP))
        {
            selectPrevious(audio);
        }

        if (input.justPressed(Input::DOWN))
        {
            selectNext(audio);
        }

        if (input.justPressed(Input::A) ||
            input.justPressed(Input::START))
        {
            if (playing)
            {
                stop(audio);
            }
            else
            {
                playSelected(audio);
            }
        }

        if (input.justPressed(Input::B))
        {
            stop(audio);
        }

        if (playing)
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

        const Graphics::Color night = Graphics::rgb565(20, 30, 48);
        const Graphics::Color deepBlue = Graphics::rgb565(31, 48, 72);
        const Graphics::Color panel = Graphics::rgb565(42, 61, 84);
        const Graphics::Color gold = Graphics::rgb565(225, 181, 83);
        const Graphics::Color paleGold = Graphics::rgb565(250, 224, 157);
        const Graphics::Color ivory = Graphics::rgb565(245, 238, 218);
        const Graphics::Color muted = Graphics::rgb565(157, 174, 188);
        const Graphics::Color accent = Graphics::rgb565(221, 143, 72);
        const Graphics::Color sunflower = Graphics::rgb565(243, 190, 52);

        graphics.fillScreen(night);

        // Subtle Japanese seigaiha-inspired arcs.
        for (int16_t x = -16; x < SCREEN_W + 32; x += 32)
        {
            graphics.drawCircle(x, 232, 24, 12, deepBlue);
            graphics.drawCircle(x + 16, 232, 24, 12, deepBlue);
        }

        graphics.drawString(
            "MIDI MUSIC BOX",
            SCREEN_W / 2,
            12,
            paleGold,
            Graphics::SIZE_25B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::TOP);

        graphics.drawString(
            "EMBEDDED FORMAT 1",
            SCREEN_W / 2,
            40,
            muted,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::TOP);

        graphics.fillRoundRect(42, 60, 236, 80, 12, panel);
        graphics.drawRoundRect(42, 60, 236, 80, 12, 2, gold);

        graphics.fillRoundRect(72, 78, 176, 28, 13, deepBlue);
        graphics.drawRoundRect(72, 78, 176, 28, 13, 2, paleGold);

        const uint32_t now = Platform::getMsec();
        const uint32_t animation =
            playing ? (now - animationStartMsec) / 80u : 0u;

        for (uint8_t i = 0; i < 11; ++i)
        {
            const int16_t pinX = static_cast<int16_t>(82 + i * 15);
            const int16_t pinY = static_cast<int16_t>(
                90 + ((i + animation) % 3) * 3);
            graphics.fillCircle(pinX, pinY, 2, gold);
        }

        // A small sunflower motif for HIMAWARI.
        graphics.fillCircle(260, 70, 9, sunflower);
        graphics.fillCircle(260, 70, 4, accent);

        for (uint8_t i = 0; i < 8; ++i)
        {
            const float radians = static_cast<float>(i) * 0.785398f;
            const int16_t px = static_cast<int16_t>(
                260 + Math::cos(radians) * 13.0f);
            const int16_t py = static_cast<int16_t>(
                70 + Math::sin(radians) * 13.0f);
            graphics.fillCircle(px, py, 3, sunflower);
        }

        if (playing)
        {
            drawMusicNote(
                graphics,
                static_cast<int16_t>(58 + animation % 18),
                static_cast<int16_t>(76 - animation % 8),
                13,
                paleGold);

            drawMusicNote(
                graphics,
                static_cast<int16_t>(266 - animation % 14),
                static_cast<int16_t>(122 - animation % 10),
                10,
                gold);
        }

        for (uint8_t i = 0; i < SONG_COUNT; ++i)
        {
            const int16_t y = static_cast<int16_t>(151 + i * 24);
            const bool selected = i == selectedSong;

            if (selected)
            {
                graphics.fillRoundRect(45, y - 5, 230, 22, 6, deepBlue);
                graphics.drawRoundRect(45, y - 5, 230, 22, 6, 1, gold);
            }

            char number[4];
            std::snprintf(
                number,
                sizeof(number),
                "%02u",
                static_cast<unsigned>(i + 1));

            graphics.drawString(
                number,
                58,
                y,
                selected ? gold : muted,
                Graphics::SIZE_13,
                Graphics::HorizontalAlign::LEFT,
                Graphics::VerticalAlign::TOP);

            graphics.drawString(
                SONGS[i].title,
                92,
                y,
                selected ? ivory : muted,
                Graphics::SIZE_13,
                Graphics::HorizontalAlign::LEFT,
                Graphics::VerticalAlign::TOP);

            graphics.drawString(
                SONGS[i].subtitle,
                262,
                y + 2,
                selected ? paleGold : deepBlue,
                Graphics::SIZE_10,
                Graphics::HorizontalAlign::RIGHT,
                Graphics::VerticalAlign::TOP);
        }

        graphics.fillRect(0, 224, SCREEN_W, 16, deepBlue);

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

MidiMusicBoxApp app;


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
