/*
===============================================================================
 PRUZEAmini Playable Game
 AquaDrop
===============================================================================

A complete falling-block puzzle game with chains, scoring, music, and save data.

Controls:
- LEFT / RIGHT : Move
- DOWN         : Soft drop
- UP / A       : Rotate
- B            : Alternate rotate
- SELECT       : Return / Cancel
- START        : Start / Pause

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>
#include "AquaDrop.h"

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
        .UP       = -1, // ROTATE
        .DOWN     = -1, // SOFT DROP
        .LEFT     = -1, // MOVE
        .RIGHT    = -1, // MOVE
        .A        = -1, // ROTATE / CONFIRM
        .B        = -1, // ALTERNATE ROTATE
        .START    = -1, // START / PAUSE
        .SELECT   = -1, // RETURN / CANCEL

        .VOL_UP   = -1,
        .VOL_DOWN = -1,
        .MUTE     = -1,
    },
};

AudioConfig audioConfig = AudioPWMConfig{
    .pwmPin = -1,
};

StorageConfig storageConfig = StorageEEPROMConfig{};

AquaDrop app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
