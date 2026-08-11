/*
===============================================================================
 PRUZEAmini Example
 16_MP3_Deck
===============================================================================

A DENON-inspired champagne-gold MP3 deck UI.

This example intentionally bypasses PRUZEAmini Audio and Storage:
- AudioStubConfig: the app controls I2S directly with ESP8266Audio.
- StorageStubConfig: the app mounts and reads the SD card directly.
- RP2040/RP2350: MP3 decoding runs in setup1()/loop1().
- ESP32: MP3 decoding runs in a FreeRTOS task.
- UI supports both XPT2046 touch input and ordinary PRUZEAmini buttons.

Required libraries:
- ESP8266Audio by Earle F. Philhower, III

SD card layout:
  /music/*.mp3

Recommended RP2040/RP2350 SPI arrangement:
- ILI9341 LCD : SPI1
- SD card     : SPI0

Before compiling:
- Replace all required -1 pin values.
- Set ENABLE_TOUCH to false when no touch panel is connected.
- AudioStub must leave Core 1 unused in the PRUZEAmini runtime.
===============================================================================
*/

#include <PRUZEAmini.h>
#include <SPI.h>
#include <SD.h>
#include <atomic>
#include <ctype.h>

#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#define ENABLE_TOUCH true

#if defined(ARDUINO_ARCH_RP2040)
#include "pico/critical_section.h"
#elif defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

using namespace PRUZEAmini;

// =============================================================================
// Hardware configuration
// =============================================================================

// ILI9341 (recommended: SPI1 on RP2040/RP2350)
GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost       = 0,
    .spiWriteFreq  = 62500000,
    .clkPin        = -1,
    .dataPin       = -1,
    .dcPin         = -1,
    .csPin         = -1,
    .resetPin      = -1,
    .backlightPin  = -1,
    .lcdRotate     = 1,
};

// Optional physical buttons.
constexpr ButtonPins GPIO_BUTTON_PINS {
    .UP       = -1,
    .DOWN     = -1,
    .LEFT     = -1,
    .RIGHT    = -1,
    .A        = -1,
    .B        = -1,
    .X        = -1,
    .Y        = -1,
    .L        = -1,
    .R        = -1,
    .START    = -1,
    .SELECT   = -1,
    .VOL_UP   = -1,
    .VOL_DOWN = -1,
    .MUTE     = -1,
};

#if ENABLE_TOUCH
InputConfig inputConfig = InputTouchConfig<InputGpioButtonsConfig>{
    .input = {
        .gpioButtonPins = GPIO_BUTTON_PINS,
    },
    .touch = {
        // Share the LCD SPI host. Keep the SD card on another SPI host.
        .spiHost       = 1,
        .spiFreq       = 2000000,
        .clkPin        = -1, // Same pin as graphicsConfig.clkPin.
        .mosiPin       = -1, // Same pin as graphicsConfig.dataPin.
        .misoPin       = -1,
        .csPin         = -1,
        .irqPin        = -1,
        .minX          = 250,
        .maxX          = 3850,
        .minY          = 250,
        .maxY          = 3850,
        .minZ          = 2048,
        .offsetRotation = 0,
    },
};
#else
InputConfig inputConfig = InputGpioButtonsConfig{
    .gpioButtonPins = GPIO_BUTTON_PINS,
};
#endif

AudioConfig audioConfig = AudioStubConfig{};
StorageConfig storageConfig = StorageStubConfig{};

// SD card (recommended: SPI0 on RP2040/RP2350)
static constexpr uint8_t SD_SPI_HOST = 0;
static constexpr int8_t SD_MISO_PIN = -1;
static constexpr int8_t SD_SCK_PIN  = -1;
static constexpr int8_t SD_MOSI_PIN = -1;
static constexpr int8_t SD_CS_PIN   = -1;
static constexpr uint32_t SD_FREQ_HZ = 12000000;

// I2S output
static constexpr int8_t I2S_BCLK_PIN = -1;
static constexpr int8_t I2S_WS_PIN   = -1;
static constexpr int8_t I2S_DATA_PIN = -1;

// =============================================================================
// Player data shared between the UI core and audio worker
// =============================================================================

static constexpr char MUSIC_DIRECTORY[] = "/music";
static constexpr uint16_t MAX_TRACKS = 64;
static constexpr uint16_t MAX_PATH_LENGTH = 128;
static constexpr uint16_t MAX_TITLE_LENGTH = 64;

enum class PlayerCommand : uint8_t
{
    NONE,
    PLAY_PAUSE,
    STOP,
    PREVIOUS_TRACK,
    NEXT_TRACK,
    VOLUME_DOWN,
    VOLUME_UP,
    RESCAN
};

enum class PlayerState : uint8_t
{
    STARTING,
    NO_SD,
    SCANNING,
    READY,
    PLAYING,
    PAUSED,
    STOPPED,
    ERROR_STATE
};

struct TrackInfo
{
    char path[MAX_PATH_LENGTH];
    char title[MAX_TITLE_LENGTH];
};

struct PlayerView
{
    PlayerState state = PlayerState::STARTING;
    uint16_t selectedIndex = 0;
    uint16_t trackCount = 0;
    uint8_t volume = 70;
    uint32_t elapsedMsec = 0;
    char title[MAX_TITLE_LENGTH] = "INITIALIZING";
    char message[48] = "PLEASE WAIT";
};

static TrackInfo tracks[MAX_TRACKS];
static uint16_t trackCount = 0;
static uint16_t selectedIndex = 0;
static uint8_t playerVolume = 70;
static uint32_t playbackStartedMsec = 0;
static uint32_t pausedElapsedMsec = 0;

static std::atomic<PlayerCommand> pendingCommand{PlayerCommand::NONE};
static std::atomic<bool> workerEnabled{false};
static PlayerView sharedView;

#if defined(ARDUINO_ARCH_RP2040)
static critical_section_t viewLock;
#elif defined(ARDUINO_ARCH_ESP32)
static portMUX_TYPE viewLock = portMUX_INITIALIZER_UNLOCKED;
#endif

static AudioFileSourceSD* audioFile = nullptr;
static AudioGeneratorMP3* mp3 = nullptr;
static AudioOutputI2S* audioOutput = nullptr;

#if defined(ARDUINO_ARCH_RP2040)
static SPIClassRP2040& sdSpi = SPI;
#elif defined(ARDUINO_ARCH_ESP32)
static SPIClass sdSpi(SD_SPI_HOST == 0 ? HSPI : VSPI);
#endif

// =============================================================================
// Shared-state helpers
// =============================================================================

static void lockView()
{
#if defined(ARDUINO_ARCH_RP2040)
    critical_section_enter_blocking(&viewLock);
#elif defined(ARDUINO_ARCH_ESP32)
    portENTER_CRITICAL(&viewLock);
#endif
}

static void unlockView()
{
#if defined(ARDUINO_ARCH_RP2040)
    critical_section_exit(&viewLock);
#elif defined(ARDUINO_ARCH_ESP32)
    portEXIT_CRITICAL(&viewLock);
#endif
}

static PlayerView readPlayerView()
{
    lockView();
    PlayerView copy = sharedView;
    unlockView();
    return copy;
}

static void publishView(PlayerState state, const char* message = nullptr)
{
    PlayerView view;
    view.state = state;
    view.selectedIndex = selectedIndex;
    view.trackCount = trackCount;
    view.volume = playerVolume;

    if (state == PlayerState::PLAYING)
    {
        view.elapsedMsec = pausedElapsedMsec + (millis() - playbackStartedMsec);
    }
    else
    {
        view.elapsedMsec = pausedElapsedMsec;
    }

    if (trackCount > 0)
    {
        snprintf(view.title, sizeof(view.title), "%s", tracks[selectedIndex].title);
    }
    else
    {
        snprintf(view.title, sizeof(view.title), "NO TRACK");
    }

    if (message)
    {
        snprintf(view.message, sizeof(view.message), "%s", message);
    }
    else
    {
        view.message[0] = '\0';
    }

    lockView();
    sharedView = view;
    unlockView();
}

static bool sendCommand(PlayerCommand command)
{
    PlayerCommand expected = PlayerCommand::NONE;
    return pendingCommand.compare_exchange_strong(expected, command);
}

// =============================================================================
// Audio worker
// =============================================================================

static bool hasMp3Extension(const char* name)
{
    if (!name) return false;
    const size_t length = strlen(name);
    if (length < 4) return false;
    const char* ext = name + length - 4;
    return ext[0] == '.' &&
           tolower(static_cast<unsigned char>(ext[1])) == 'm' &&
           tolower(static_cast<unsigned char>(ext[2])) == 'p' &&
           ext[3] == '3';
}

static void makeDisplayTitle(const char* path, char* output, size_t outputSize)
{
    const char* name = strrchr(path, '/');
    name = name ? name + 1 : path;
    snprintf(output, outputSize, "%s", name);

    const size_t length = strlen(output);
    if (length >= 4 && output[length - 4] == '.')
    {
        output[length - 4] = '\0';
    }
}

static bool beginSDCard()
{
    if (SD_MISO_PIN < 0 || SD_SCK_PIN < 0 ||
        SD_MOSI_PIN < 0 || SD_CS_PIN < 0)
    {
        return false;
    }

    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

#if defined(ARDUINO_ARCH_RP2040)
    sdSpi.setRX(SD_MISO_PIN);
    sdSpi.setSCK(SD_SCK_PIN);
    sdSpi.setTX(SD_MOSI_PIN);
    sdSpi.begin();
    return SD.begin(SD_CS_PIN, SD_FREQ_HZ, sdSpi);
#elif defined(ARDUINO_ARCH_ESP32)
    sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    return SD.begin(SD_CS_PIN, sdSpi, SD_FREQ_HZ);
#else
    return false;
#endif
}

static bool scanTracks()
{
    trackCount = 0;
    selectedIndex = 0;
    publishView(PlayerState::SCANNING, "READING DIRECTORY");

    File directory = SD.open(MUSIC_DIRECTORY);
    if (!directory || !directory.isDirectory())
    {
        if (directory) directory.close();
        publishView(PlayerState::ERROR_STATE, "NO /music DIRECTORY");
        return false;
    }

    while (trackCount < MAX_TRACKS)
    {
        File entry = directory.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory() && hasMp3Extension(entry.name()))
        {
            const char* name = entry.name();
            if (name[0] == '/')
            {
                snprintf(tracks[trackCount].path,
                         sizeof(tracks[trackCount].path), "%s", name);
            }
            else
            {
                snprintf(tracks[trackCount].path,
                         sizeof(tracks[trackCount].path),
                         "%s/%s", MUSIC_DIRECTORY, name);
            }

            makeDisplayTitle(tracks[trackCount].path,
                             tracks[trackCount].title,
                             sizeof(tracks[trackCount].title));
            ++trackCount;
        }
        entry.close();
    }

    directory.close();

    if (trackCount == 0)
    {
        publishView(PlayerState::READY, "NO MP3 FILES");
        return true;
    }

    publishView(PlayerState::READY, "READY");
    return true;
}

static void destroyPlaybackObjects()
{
    if (mp3)
    {
        if (mp3->isRunning()) mp3->stop();
        delete mp3;
        mp3 = nullptr;
    }

    if (audioFile)
    {
        delete audioFile;
        audioFile = nullptr;
    }
}

static void applyVolume()
{
    if (audioOutput)
    {
        audioOutput->SetGain(static_cast<float>(playerVolume) / 100.0f);
    }
}

static bool ensureAudioOutput()
{
    if (audioOutput) return true;
    if (I2S_BCLK_PIN < 0 || I2S_WS_PIN < 0 || I2S_DATA_PIN < 0) return false;

    audioOutput = new AudioOutputI2S();
    if (!audioOutput) return false;

    if (!audioOutput->SetPinout(I2S_BCLK_PIN, I2S_WS_PIN, I2S_DATA_PIN))
    {
        delete audioOutput;
        audioOutput = nullptr;
        return false;
    }

    applyVolume();
    return true;
}

static bool startSelectedTrack()
{
    if (trackCount == 0) return false;
    if (!ensureAudioOutput())
    {
        publishView(PlayerState::ERROR_STATE, "I2S INIT FAILED");
        return false;
    }

    destroyPlaybackObjects();
    pausedElapsedMsec = 0;

    audioFile = new AudioFileSourceSD(tracks[selectedIndex].path);
    mp3 = new AudioGeneratorMP3();

    if (!audioFile || !mp3 || !mp3->begin(audioFile, audioOutput))
    {
        destroyPlaybackObjects();
        publishView(PlayerState::ERROR_STATE, "MP3 OPEN FAILED");
        return false;
    }

    playbackStartedMsec = millis();
    publishView(PlayerState::PLAYING, "PLAY");
    return true;
}

static void stopPlayback()
{
    destroyPlaybackObjects();
    pausedElapsedMsec = 0;
    publishView(PlayerState::STOPPED, "STOP");
}

static void selectRelativeTrack(int direction, bool startPlaying)
{
    if (trackCount == 0) return;

    int32_t index = static_cast<int32_t>(selectedIndex) + direction;
    if (index < 0) index = trackCount - 1;
    if (index >= trackCount) index = 0;
    selectedIndex = static_cast<uint16_t>(index);

    if (startPlaying)
    {
        startSelectedTrack();
    }
    else
    {
        pausedElapsedMsec = 0;
        publishView(PlayerState::READY, "SELECTED");
    }
}

static void processCommand(PlayerCommand command)
{
    const PlayerView current = readPlayerView();

    switch (command)
    {
        case PlayerCommand::PLAY_PAUSE:
            if (current.state == PlayerState::PLAYING)
            {
                pausedElapsedMsec += millis() - playbackStartedMsec;
                publishView(PlayerState::PAUSED, "PAUSE");
            }
            else if (current.state == PlayerState::PAUSED && mp3)
            {
                playbackStartedMsec = millis();
                publishView(PlayerState::PLAYING, "PLAY");
            }
            else
            {
                startSelectedTrack();
            }
            break;

        case PlayerCommand::STOP:
            stopPlayback();
            break;

        case PlayerCommand::PREVIOUS_TRACK:
            selectRelativeTrack(-1, current.state == PlayerState::PLAYING);
            break;

        case PlayerCommand::NEXT_TRACK:
            selectRelativeTrack(1, current.state == PlayerState::PLAYING);
            break;

        case PlayerCommand::VOLUME_DOWN:
            playerVolume = playerVolume >= 5 ? playerVolume - 5 : 0;
            applyVolume();
            publishView(current.state, "VOLUME");
            break;

        case PlayerCommand::VOLUME_UP:
            playerVolume = playerVolume <= 95 ? playerVolume + 5 : 100;
            applyVolume();
            publishView(current.state, "VOLUME");
            break;

        case PlayerCommand::RESCAN:
            stopPlayback();
            scanTracks();
            break;

        default:
            break;
    }
}

static void initializeAudioWorker()
{
    publishView(PlayerState::STARTING, "INITIALIZING SD");

    if (!beginSDCard())
    {
        publishView(PlayerState::NO_SD, "INSERT SD / RETRY");
        return;
    }

    scanTracks();
}

static void updateAudioWorker()
{
    const PlayerCommand command = pendingCommand.exchange(PlayerCommand::NONE);
    if (command != PlayerCommand::NONE)
    {
        processCommand(command);
    }

    const PlayerView current = readPlayerView();
    if (current.state == PlayerState::PLAYING && mp3)
    {
        if (!mp3->isRunning() || !mp3->loop())
        {
            destroyPlaybackObjects();
            if (trackCount > 0)
            {
                selectedIndex = (selectedIndex + 1) % trackCount;
                startSelectedTrack();
            }
            else
            {
                publishView(PlayerState::READY, "END");
            }
        }
        else
        {
            static uint32_t lastStatusMsec = 0;
            const uint32_t now = millis();
            if (now - lastStatusMsec >= 250)
            {
                lastStatusMsec = now;
                publishView(PlayerState::PLAYING, "PLAY");
            }
        }
    }
    else
    {
        delay(1);
    }
}

#if defined(ARDUINO_ARCH_RP2040)
void setup1()
{
    while (!workerEnabled.load()) delay(1);
    initializeAudioWorker();
}

void loop1()
{
    updateAudioWorker();
}
#elif defined(ARDUINO_ARCH_ESP32)
static void audioTask(void*)
{
    initializeAudioWorker();
    for (;;)
    {
        updateAudioWorker();
        taskYIELD();
    }
}
#endif

// =============================================================================
// DENON-inspired UI
// =============================================================================

class MP3DeckApp : public App
{
public:
    const char* getId() const override { return "mp3_deck"; }

private:
    static constexpr Graphics::Color PANEL = Graphics::rgb565(198, 179, 132);
    static constexpr Graphics::Color PANEL_LIGHT = Graphics::rgb565(235, 222, 184);
    static constexpr Graphics::Color PANEL_DARK = Graphics::rgb565(102, 82, 48);
    static constexpr Graphics::Color DISPLAY_BG = Graphics::rgb565(16, 12, 7);
    static constexpr Graphics::Color DISPLAY_EDGE = Graphics::rgb565(94, 66, 26);
    static constexpr Graphics::Color DISPLAY_TEXT = Graphics::rgb565(245, 184, 48);
    static constexpr Graphics::Color DISPLAY_DIM = Graphics::rgb565(125, 82, 22);
    static constexpr Graphics::Color LABEL = Graphics::rgb565(54, 45, 29);

    struct Rect
    {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;

        bool contains(int16_t px, int16_t py) const
        {
            return px >= x && py >= y && px < x + w && py < y + h;
        }
    };

    // Six controls, evenly spaced across the panel.
    // The visible buttons are approximately 1.2x larger than the previous version.
    static constexpr Rect VOL_DOWN_BUTTON = {5,   171, 46, 43};
    static constexpr Rect PREV_BUTTON     = {58,  171, 46, 43};
    static constexpr Rect PLAY_BUTTON     = {111, 171, 46, 43};
    static constexpr Rect STOP_BUTTON     = {164, 171, 46, 43};
    static constexpr Rect NEXT_BUTTON     = {217, 171, 46, 43};
    static constexpr Rect VOL_UP_BUTTON   = {270, 171, 46, 43};
    static constexpr Rect RETRY_BUTTON = {244, 18, 58, 28};

#if ENABLE_TOUCH
    void updateTouchControls(Input& input)
    {
        if (!input.justTouched()) return;

        const int16_t x = input.touchX();
        const int16_t y = input.touchY();
        if (PREV_BUTTON.contains(x, y)) sendCommand(PlayerCommand::PREVIOUS_TRACK);
        else if (PLAY_BUTTON.contains(x, y)) sendCommand(PlayerCommand::PLAY_PAUSE);
        else if (STOP_BUTTON.contains(x, y)) sendCommand(PlayerCommand::STOP);
        else if (NEXT_BUTTON.contains(x, y)) sendCommand(PlayerCommand::NEXT_TRACK);
        else if (VOL_DOWN_BUTTON.contains(x, y)) sendCommand(PlayerCommand::VOLUME_DOWN);
        else if (VOL_UP_BUTTON.contains(x, y)) sendCommand(PlayerCommand::VOLUME_UP);
        else if (RETRY_BUTTON.contains(x, y)) sendCommand(PlayerCommand::RESCAN);
    }
#endif

    PlayerView view;
    PlayerView previousView;
    uint32_t lastViewReadMsec = 0;
    uint32_t lastMarqueeMsec = 0;
    uint16_t marqueeOffset = 0;

    static const char* stateText(PlayerState state)
    {
        switch (state)
        {
            case PlayerState::STARTING: return "START";
            case PlayerState::NO_SD: return "NO SD";
            case PlayerState::SCANNING: return "SCAN";
            case PlayerState::READY: return "READY";
            case PlayerState::PLAYING: return "PLAY";
            case PlayerState::PAUSED: return "PAUSE";
            case PlayerState::STOPPED: return "STOP";
            case PlayerState::ERROR_STATE: return "ERROR";
        }
        return "";
    }

    static void formatTime(uint32_t msec, char* output, size_t outputSize)
    {
        const uint32_t totalSeconds = msec / 1000;
        snprintf(output, outputSize, "%02lu:%02lu",
                 static_cast<unsigned long>(totalSeconds / 60),
                 static_cast<unsigned long>(totalSeconds % 60));
    }

    void updateButtonControls(Input& input)
    {
        if (input.justPressed(Input::LEFT)) sendCommand(PlayerCommand::PREVIOUS_TRACK);
        if (input.justPressed(Input::RIGHT)) sendCommand(PlayerCommand::NEXT_TRACK);
        if (input.justPressed(Input::A)) sendCommand(PlayerCommand::PLAY_PAUSE);
        if (input.justPressed(Input::START)) sendCommand(PlayerCommand::PLAY_PAUSE);
        if (input.justPressed(Input::B)) sendCommand(PlayerCommand::STOP);
        if (input.justPressed(Input::L)) sendCommand(PlayerCommand::VOLUME_DOWN);
        if (input.justPressed(Input::R)) sendCommand(PlayerCommand::VOLUME_UP);
        if (input.justPressed(Input::SELECT)) sendCommand(PlayerCommand::RESCAN);
    }

    void drawPanel(Graphics& graphics)
    {
        graphics.fillScreen(PANEL);
        graphics.fillRect(0, 0, 320, 3, PANEL_LIGHT);
        graphics.fillRect(0, 237, 320, 3, PANEL_DARK);
        graphics.drawLine(8, 52, 311, 52, PANEL_LIGHT);
        graphics.drawLine(8, 53, 311, 53, PANEL_DARK);

        graphics.drawString("PRUZEA", 13, 12, LABEL, Graphics::SIZE_22B);
        graphics.drawString("PERSONAL DIGITAL AUDIO SYSTEM", 14, 36, LABEL, Graphics::SIZE_10);
        graphics.drawString("DMP-F01", 306, 8, LABEL, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::TOP);
    }

    void drawDisplay(Graphics& graphics)
    {
        graphics.fillRoundRect(30, 62, 260, 100, 5, PANEL_DARK);
        graphics.fillRoundRect(34, 66, 252, 92, 3, DISPLAY_EDGE);
        graphics.fillRect(38, 70, 244, 84, DISPLAY_BG);

        char trackText[24];
        if (view.trackCount > 0)
        {
            snprintf(trackText, sizeof(trackText), "TRACK %02u/%02u",
                     view.selectedIndex + 1, view.trackCount);
        }
        else
        {
            snprintf(trackText, sizeof(trackText), "TRACK --/--");
        }

        graphics.drawString(trackText, 46, 77, DISPLAY_DIM, Graphics::SIZE_13);
        graphics.drawString(stateText(view.state), 274, 77, DISPLAY_TEXT,
                            Graphics::SIZE_13,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::TOP);

        char visibleTitle[26];
        const size_t titleLength = strlen(view.title);
        if (titleLength <= 23)
        {
            snprintf(visibleTitle, sizeof(visibleTitle), "%s", view.title);
        }
        else
        {
            for (uint8_t i = 0; i < 23; ++i)
            {
                const size_t index = (marqueeOffset + i) % (titleLength + 3);
                visibleTitle[i] = index < titleLength ? view.title[index] : ' ';
            }
            visibleTitle[23] = '\0';
        }

        // Keep the scrolling title inside the display window.
        graphics.setClipRect(46, 100, 232, 20);
        graphics.drawString(visibleTitle, 50, 101, DISPLAY_TEXT, Graphics::SIZE_18);
        graphics.resetClipRect();

        char timeText[12];
        formatTime(view.elapsedMsec, timeText, sizeof(timeText));
        graphics.drawString(timeText, 46, 128, DISPLAY_TEXT, Graphics::SIZE_18);

        char formatText[32];
        snprintf(formatText, sizeof(formatText), "MP3   VOL %02u", view.volume);
        graphics.drawString(formatText, 274, 131, DISPLAY_DIM, Graphics::SIZE_13,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::TOP);
    }

    enum class ControlIcon : uint8_t
    {
        VOLUME_DOWN,
        PREVIOUS,
        PLAY_PAUSE,
        STOP,
        NEXT,
        VOLUME_UP
    };

    void drawControlIcon(Graphics& graphics, ControlIcon icon,
                         int16_t cx, int16_t cy)
    {
        switch (icon)
        {
            case ControlIcon::VOLUME_DOWN:
                graphics.fillRect(cx - 6, cy - 1, 12, 3, LABEL);
                break;

            case ControlIcon::VOLUME_UP:
                graphics.fillRect(cx - 6, cy - 1, 12, 3, LABEL);
                graphics.fillRect(cx - 1, cy - 6, 3, 12, LABEL);
                break;

            case ControlIcon::PREVIOUS:
                graphics.fillRect(cx - 8, cy - 6, 4, 12, LABEL);
                graphics.fillTriangle(cx - 5, cy, cx + 6, cy - 7, cx + 6, cy + 7, LABEL);
                break;

            case ControlIcon::NEXT:
                graphics.fillTriangle(cx - 6, cy - 7, cx + 5, cy, cx - 6, cy + 7, LABEL);
                graphics.fillRect(cx + 4, cy - 6, 4, 12, LABEL);
                break;

            case ControlIcon::PLAY_PAUSE:
                if (view.state == PlayerState::PLAYING)
                {
                    graphics.fillRect(cx - 6, cy - 7, 4, 14, LABEL);
                    graphics.fillRect(cx + 2, cy - 7, 4, 14, LABEL);
                }
                else
                {
                    graphics.fillTriangle(cx - 5, cy - 8, cx + 8, cy, cx - 5, cy + 8, LABEL);
                }
                break;

            case ControlIcon::STOP:
                graphics.fillRect(cx - 7, cy - 7, 14, 14, LABEL);
                break;
        }
    }

    void drawRoundButton(Graphics& graphics, const Rect& rect, ControlIcon icon, const char* label)
    {
        const int16_t cx = rect.x + rect.w / 2;
        const int16_t cy = rect.y + 15;
        graphics.fillCircle(cx, cy, 15, PANEL_DARK);
        graphics.fillCircle(cx - 1, cy - 1, 13, PANEL_LIGHT);
        graphics.drawCircle(cx, cy, 15, LABEL);
        drawControlIcon(graphics, icon, cx, cy);
        graphics.drawString(label, cx, rect.y + 34, LABEL, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::TOP);
    }

    void drawControls(Graphics& graphics)
    {
        drawRoundButton(graphics, VOL_DOWN_BUTTON, ControlIcon::VOLUME_DOWN, "VOL");
        drawRoundButton(graphics, PREV_BUTTON, ControlIcon::PREVIOUS, "PREV");
        drawRoundButton(graphics, PLAY_BUTTON, ControlIcon::PLAY_PAUSE, "PLAY");
        drawRoundButton(graphics, STOP_BUTTON, ControlIcon::STOP, "STOP");
        drawRoundButton(graphics, NEXT_BUTTON, ControlIcon::NEXT, "NEXT");
        drawRoundButton(graphics, VOL_UP_BUTTON, ControlIcon::VOLUME_UP, "VOL");

        graphics.drawString("L/R VOL   A/START PLAY   B STOP", 160, 221,
                            LABEL, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::TOP);

        graphics.drawRoundRect(RETRY_BUTTON.x, RETRY_BUTTON.y,
                               RETRY_BUTTON.w, RETRY_BUTTON.h,
                               4, LABEL);
        graphics.drawString("RETRY", RETRY_BUTTON.x + RETRY_BUTTON.w / 2,
                            RETRY_BUTTON.y + RETRY_BUTTON.h / 2,
                            LABEL, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
    }

public:
    void onInit(Storage& storage) override
    {
        (void)storage;
        view = readPlayerView();
        previousView = view;
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)audio;
        (void)storage;
        (void)deltaSec;

        updateButtonControls(input);
#if ENABLE_TOUCH
        updateTouchControls(input);
#endif

        const uint32_t now = millis();
        if (now - lastViewReadMsec >= 100)
        {
            lastViewReadMsec = now;
            view = readPlayerView();
            const bool changed =
                view.state != previousView.state ||
                view.selectedIndex != previousView.selectedIndex ||
                view.trackCount != previousView.trackCount ||
                view.volume != previousView.volume ||
                view.elapsedMsec != previousView.elapsedMsec ||
                strcmp(view.title, previousView.title) != 0 ||
                strcmp(view.message, previousView.message) != 0;

            if (changed)
            {
                if (strcmp(view.title, previousView.title) != 0) marqueeOffset = 0;
                previousView = view;
                dirty = true;
            }
        }

        if (strlen(view.title) > 23 && now - lastMarqueeMsec >= 150)
        {
            lastMarqueeMsec = now;
            ++marqueeOffset;
            dirty = true;
        }
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty) return false;

        drawPanel(graphics);
        drawDisplay(graphics);
        drawControls(graphics);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
        sendCommand(PlayerCommand::STOP);
    }
};

MP3DeckApp app;

// =============================================================================
// Arduino entry points
// =============================================================================

void setup()
{
    Serial.begin(115200);

#if defined(ARDUINO_ARCH_RP2040)
    critical_section_init(&viewLock);
#endif

    publishView(PlayerState::STARTING, "WAITING FOR AUDIO CORE");
    workerEnabled.store(true);

#if defined(ARDUINO_ARCH_ESP32)
    xTaskCreatePinnedToCore(audioTask, "mp3_audio", 8192, nullptr, 2, nullptr, 0);
#endif

    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
