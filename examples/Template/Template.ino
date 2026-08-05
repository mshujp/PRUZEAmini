/*
===============================================================================
 PRUZEAmini
 Application Template
===============================================================================
*/

#include <PRUZEAmini.h>

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

class TemplateApp : public App
{
public:
    const char* getId() const override { return "my_app"; }

private:

protected:
    void onInit(Storage& storage) override
    {
        // Initialize app resources.
        // (Graphics, Input, Audio, and Storage are already initialized.)

        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        // Read input.
        // Update the app state.
        // Set dirty = true if the screen needs to be redrawn.
        // Save data if necessary.
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }

        // Draw the app.
        // e.g. drawLine(), drawString(), drawSprite()

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        // Reserved for future use.
    }
};


// =============================================================================
// Application instance
// =============================================================================

TemplateApp app;


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
