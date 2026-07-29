/*
===============================================================================
 PRUZEAmini Example
 00B_Hardware_Setup
===============================================================================

Copy the configuration for the hardware you want to use.
Change each value to match your hardware.

Only one configuration from each section should be used.

This file is a copy-and-paste hardware configuration reference.
One configuration in each category is enabled. Alternative configurations
are commented out.
*/

#include <PRUZEAmini.h>
using namespace PRUZEAmini;

/*
===============================================================================
 Graphics
===============================================================================
*/

// -----------------------------------------------------------------------------
// GraphicsILI9341
// -----------------------------------------------------------------------------

GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost = 0,  // 0 or 1. Select the SPI host that matches the pins.
                   // When using an SD card, SPI0 for SD and SPI1 for the display is recommended.
    .spiWriteFreq    = 62500000,
    .clkPin          = -1,      // or SCK
    .dataPin         = -1,      // or MOSI
    .dcPin           = -1,
    .csPin           = -1,
    .resetPin        = -1,
    .backlightPin    = -1,
    .lcdRotate  = 0,  // 0: Normal  3: Rotated 180 degrees
};

// -----------------------------------------------------------------------------
// GraphicsILI9341Parallel
// RP2040/RP2350: dataPinBase through dataPinBase + 7 must be consecutive.
// Classic ESP32: Uses LovyanGFX's native 8-bit parallel bus.
// -----------------------------------------------------------------------------

/*
GraphicsConfig graphicsConfig = GraphicsILI9341ParallelConfig{
    .writeFreq       = 10000000,
    .dataPinBase     = -1,  // The eight data pins must be connected consecutively, starting from dataPinBase
    .wrPin           = -1,
    .rdPin           = -1,
    .dcPin           = -1,
    .csPin           = -1,
    .resetPin        = -1,
    .backlightPin    = -1,
    .lcdRotate       = 1,
};
*/

// -----------------------------------------------------------------------------
// GraphicsSSD1306
// -----------------------------------------------------------------------------

/*
GraphicsConfig graphicsConfig = GraphicsSSD1306Config{
    .i2cPort    = 0,            // 0 or 1
    .i2cAddr    = 0x3C,         // 0x3C or 0x3D, depending on the module
    .sdaPin     = -1,
    .sclPin     = -1,
    .resetPin   = -1,
    .oledRotate = 0,  // 0: Normal  2: Rotated 180 degrees
};
*/

/*
===============================================================================
 Input
===============================================================================
*/

// -----------------------------------------------------------------------------
// InputGpioButtons
// -----------------------------------------------------------------------------

InputConfig inputConfig = InputGpioButtonsConfig{
    .gpioButtonPins = {
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
    },
};


// -----------------------------------------------------------------------------
// InputSnes
// -----------------------------------------------------------------------------

/*
InputConfig inputConfig = InputSnesConfig{
    .clkPin  = -1,
    .latPin  = -1,
    .dataPin = -1,

    .extraGpioButtonPins = {
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
    },
};
*/

/*
===============================================================================
 Audio
===============================================================================
*/

// -----------------------------------------------------------------------------
// AudioI2S
// -----------------------------------------------------------------------------

/*
AudioConfig audioConfig = AudioI2SConfig{
    .bclkPin = -1,
    .wsPin = -1, // LRCLK/WS; on Pico(rp2040/rp2350), wsPin must equal bclkPin + 1.
    .dataPin = -1,
};
*/

// -----------------------------------------------------------------------------
// AudioPWM
// -----------------------------------------------------------------------------

/*
AudioConfig audioConfig = AudioPWMConfig{
    .pwmPin = -1,
};
*/

// -----------------------------------------------------------------------------
// AudioStub
//
// Use when audio hardware is not supported or not used.
// -----------------------------------------------------------------------------

AudioConfig audioConfig = AudioStubConfig{};


/*
===============================================================================
 Storage
===============================================================================
*/

// -----------------------------------------------------------------------------
// StorageSD
// -----------------------------------------------------------------------------

/*
StorageConfig storageConfig = StorageSDConfig{
    .spiHost = 0, // 0 or 1. Select the SPI host that matches the pins.
                  // When using a display, SPI0 for SD and SPI1 for the display is recommended.
    .misoPin  = -1,
    .sckPin   = -1,
    .mosiPin  = -1,
    .csPin    = -1,
    .baudRate = 12000000,
};
*/

// -----------------------------------------------------------------------------
// StorageEEPROM  Default values are recommended
// -----------------------------------------------------------------------------

/*
StorageConfig storageConfig = StorageEEPROMConfig{
    .magic      = 0x504d,
    .version    = 1,
    .eepromSize = 4096,
};
*/

// -----------------------------------------------------------------------------
// StorageStub
//
// Use when storage hardware is not supported or not used.
// -----------------------------------------------------------------------------

StorageConfig storageConfig = StorageStubConfig{};




// ----------------------------------------
// ----------------------------------------

class MyApp : public App
{
public:
    const char* getId() const override { return "hardware_setup"; }
    void onInit(Storage& storage) override {}
    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override {}
    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }
        graphics.clearScreen();

        dirty = false;
        return true;
    }
    void onTerminate(Storage& storage) override {}
};

MyApp app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
