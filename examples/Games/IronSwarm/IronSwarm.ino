/*
===============================================================================
 PRUZEAmini Playable Game
 IronSwarm
===============================================================================

A complete arena action game with missions, upgrades, bosses, music, and save data.

Controls:
- D-PAD / Left stick : Move
- A                  : Confirm
- B                  : Special targeting
- SELECT             : Records / mission information
- START              : Start / Pause

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>
#include "IronSwarm.h"

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
        .A        = -1, // CONFIRM
        .B        = -1, // SPECIAL TARGET
        .START    = -1, // START / PAUSE
        .SELECT   = -1, // RECORDS / INFO

        .VOL_UP   = -1,
        .VOL_DOWN = -1,
        .MUTE     = -1,
    },
};

AudioConfig audioConfig = AudioPWMConfig{
    .pwmPin = -1,
};

StorageConfig storageConfig = StorageEEPROMConfig{};

IronSwarm app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
