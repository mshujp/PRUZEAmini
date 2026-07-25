/*
===============================================================================
 PLAMIOmini Example
 05_Save_Data

 SaveData works with both Emulated EEPROM and SD storage.
 Emulated EEPROM is used here so the sample runs without an SD card.
===============================================================================

This example shows how to save and load ordinary game data with SaveData.

Controls:
- A     : Add 10 points
- B     : Save
- START : Load
- DOWN  : Reset the value in memory

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

StorageConfig storageConfig = StorageEEPROMConfig{
    .magic      = 0x504d,
    .version    = 1,
    .eepromSize = 4096,
};

// =============================================================================
// Game
// =============================================================================

class SaveDataGame : public App
{
public:
    const char* getId() const override { return APP_ID; }

private:
    static constexpr const char* APP_ID = "save_data";
    static constexpr const char* SAVE_FILE = "save.ini";
    static constexpr const char* SCORE_KEY = "score";

    SaveData saveData;

    uint32_t score = 0;
    uint32_t savedScore = 0;

    bool storageAvailable = false;
    const char* statusText = "READY";
    Graphics::Color statusColor = Graphics::WHITE;

    void loadScore(Storage& storage)
    {
        saveData.clear();

        if (!saveData.load(storage, getId(), SAVE_FILE))
        {
            score = 0;
            savedScore = 0;
            statusText = "NO SAVE DATA";
            statusColor = Graphics::YELLOW;
            return;
        }

        score = saveData.getUInt32(SCORE_KEY, 0);
        savedScore = score;
        statusText = "LOAD OK";
        statusColor = Graphics::GREEN;
    }

    void saveScore(Storage& storage)
    {
        saveData.clear();

        if (!saveData.setUInt32(SCORE_KEY, score))
        {
            statusText = "SET FAILED";
            statusColor = Graphics::RED;
            return;
        }

        if (!saveData.save(storage, getId(), SAVE_FILE))
        {
            statusText = "SAVE FAILED";
            statusColor = Graphics::RED;
            return;
        }

        savedScore = score;
        statusText = "SAVE OK";
        statusColor = Graphics::GREEN;
    }

protected:
    void onInit(Storage& storage) override
    {
        storageAvailable = storage.isAvailable();

        if (storageAvailable)
        {
            loadScore(storage);
        }
        else
        {
            score = 0;
            savedScore = 0;
            statusText = "STORAGE UNAVAILABLE";
            statusColor = Graphics::RED;
        }

        dirty = true;
    }

    void onUpdate(
        Input& input,
        Audio& audio,
        Storage& storage,
        float deltaSec) override
    {
        (void)audio;
        (void)deltaSec;

        if (input.justPressed(Input::A))
        {
            score += 10;
            statusText = "VALUE CHANGED";
            statusColor = Graphics::CYAN;
            dirty = true;
        }

        if (input.justPressed(Input::DOWN))
        {
            score = 0;
            statusText = "RAM RESET";
            statusColor = Graphics::CYAN;
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            if (storageAvailable)
            {
                saveScore(storage);
            }
            else
            {
                statusText = "SAVE FAILED";
                statusColor = Graphics::RED;
            }

            dirty = true;
        }

        if (input.justPressed(Input::START))
        {
            if (storageAvailable)
            {
                loadScore(storage);
            }
            else
            {
                statusText = "LOAD FAILED";
                statusColor = Graphics::RED;
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

        graphics.fillRect(0, 0, 320, 36, Graphics::BLUE);

        graphics.drawString(
            "SAVE DATA",
            160, 18,
            Graphics::WHITE,
            Graphics::SIZE_25B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            storageAvailable ? "STORAGE: AVAILABLE" : "STORAGE: UNAVAILABLE",
            160, 51,
            storageAvailable ? Graphics::GREEN : Graphics::RED,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawRoundRect(
            30, 68, 260, 88, 12, 3,
            Graphics::CYAN);

        graphics.drawString(
            "CURRENT SCORE",
            160, 84,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        char scoreText[16];
        snprintf(scoreText, sizeof(scoreText), "%lu",
                 static_cast<unsigned long>(score));

        graphics.drawString(
            scoreText,
            160, 119,
            Graphics::YELLOW,
            Graphics::SIZE_42B,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        char savedText[32];
        snprintf(savedText, sizeof(savedText), "SAVED: %lu",
                 static_cast<unsigned long>(savedScore));

        graphics.drawString(
            savedText,
            160, 170,
            Graphics::WHITE,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            statusText,
            160, 193,
            statusColor,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "A:+10   DOWN:RESET",
            160, 207,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "B:SAVE   START:LOAD",
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

SaveDataGame app;


// =============================================================================
// Arduino entry points
// =============================================================================

void setup()
{
    PLAMIOmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
