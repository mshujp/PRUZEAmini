/*
===============================================================================
 PLAMIOmini Example
 04_Audio_Basics
===============================================================================

This example shows how to play sound effects and music.

Controls:
- LEFT / RIGHT : Select a sound effect
- A            : Play the selected sound effect
- B            : Start / Stop music
- START        : Stop music

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

AudioConfig audioConfig = AudioPWMConfig{
    .pwmPin = -1,
};

StorageConfig storageConfig = StorageStubConfig{};


// =============================================================================
// Music data
// =============================================================================

static const Audio::ToneNote MUSIC_NOTES[] = {
    { Audio::ToneNote::C4, Audio::ToneNote::E },
    { Audio::ToneNote::E4, Audio::ToneNote::E },
    { Audio::ToneNote::G4, Audio::ToneNote::E },
    { Audio::ToneNote::C5, Audio::ToneNote::Q },

    { Audio::ToneNote::G4, Audio::ToneNote::E },
    { Audio::ToneNote::E4, Audio::ToneNote::E },
    { Audio::ToneNote::C4, Audio::ToneNote::Q },

    { Audio::ToneNote::D4, Audio::ToneNote::E },
    { Audio::ToneNote::F4, Audio::ToneNote::E },
    { Audio::ToneNote::A4, Audio::ToneNote::E },
    { Audio::ToneNote::D5, Audio::ToneNote::Q },

    { Audio::ToneNote::A4, Audio::ToneNote::E },
    { Audio::ToneNote::F4, Audio::ToneNote::E },
    { Audio::ToneNote::D4, Audio::ToneNote::Q },
};

static const Audio::Music DEMO_MUSIC = {
    MUSIC_NOTES,
    132,
    static_cast<uint16_t>(sizeof(MUSIC_NOTES) / sizeof(MUSIC_NOTES[0])),
    0,
    0.45f,
};


// =============================================================================
// Game
// =============================================================================

class AudioBasicsGame : public GameMini
{
public:
    const char* getId() const override { return "audio_basics"; }

private:
    static constexpr uint8_t SE_COUNT = 13;

    uint8_t selectedSE = 0;
    bool musicPlaying = false;
    const char* statusText = "READY";

    const Audio::Sound* selectedSound() const
    {
        static const Audio::Sound* SOUNDS[SE_COUNT] = {
            &Audio::SE::NO_1,
            &Audio::SE::NO_2,
            &Audio::SE::NO_3,
            &Audio::SE::NO_4,
            &Audio::SE::NO_5,
            &Audio::SE::NO_6,
            &Audio::SE::NO_7,
            &Audio::SE::NO_8,
            &Audio::SE::NO_9,
            &Audio::SE::NO_10,
            &Audio::SE::NO_11,
            &Audio::SE::NO_12,
            &Audio::SE::NO_13,
        };

        return SOUNDS[selectedSE];
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        selectedSE = 0;
        musicPlaying = false;
        statusText = "READY";
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

        if (input.justPressed(Input::RIGHT))
        {
            selectedSE = (selectedSE + 1) % SE_COUNT;
            statusText = "SELECTED";
            dirty = true;
        }

        if (input.justPressed(Input::LEFT))
        {
            selectedSE = (selectedSE + SE_COUNT - 1) % SE_COUNT;
            statusText = "SELECTED";
            dirty = true;
        }

        if (input.justPressed(Input::A))
        {
            audio.playSE(selectedSound(), 0.8f);
            statusText = "PLAYING SE";
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            if (musicPlaying)
            {
                audio.stopMusic();
                musicPlaying = false;
                statusText = "MUSIC STOPPED";
            }
            else
            {
                audio.playMusic(&DEMO_MUSIC);
                musicPlaying = true;
                statusText = "MUSIC PLAYING";
            }

            dirty = true;
        }

        if (input.justPressed(Input::START))
        {
            audio.stopMusic();
            musicPlaying = false;
            statusText = "MUSIC STOPPED";
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

        graphics.fillRect(0, 0, 320, 36, Graphics::BLUE);

        graphics.drawString(
            "AUDIO BASICS",
            160, 18,
            Graphics::WHITE,
            Graphics::SIZE_25B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "SOUND EFFECT",
            160, 58,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawRoundRect(
            78, 72, 164, 62, 10, 3,
            Graphics::CYAN);

        char seText[12];
        snprintf(seText, sizeof(seText), "SE NO.%u", selectedSE + 1);

        graphics.drawString(
            seText,
            160, 103,
            Graphics::YELLOW,
            Graphics::SIZE_32B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "< LEFT       RIGHT >",
            160, 147,
            Graphics::WHITE,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            musicPlaying ? "BGM: PLAYING" : "BGM: STOPPED",
            160, 172,
            musicPlaying ? Graphics::GREEN : Graphics::GRAY,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            statusText,
            160, 196,
            Graphics::ORANGE,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "A: PLAY SE   B: MUSIC   START: STOP",
            160, 216,
            Graphics::WHITE,
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

AudioBasicsGame game;


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
