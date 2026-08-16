/*
===============================================================================
 PRUZEAmini Playable Game
 Apex Climb
===============================================================================

A complete mountain-road racing game with four courses, analog steering,
drifting, tuning, rankings, engine sound, and sound effects.

Controls:
- D-PAD / Left stick : Steering / Menu selection
- A / Right stick up : Accelerator / Confirm
- B / Right stick down: Brake / Back
- Y                  : Tuning
- START              : Start / Pause

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>
#include "ApexClimb.h"

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
        .LEFT     = -1, // STEER LEFT
        .RIGHT    = -1, // STEER RIGHT
        .A        = -1, // ACCELERATOR / CONFIRM
        .B        = -1, // BRAKE / BACK
        .Y        = -1, // TUNING
        .START    = -1, // START / PAUSE

        .VOL_UP   = -1,
        .VOL_DOWN = -1,
        .MUTE     = -1,
    },
};

AudioConfig audioConfig = AudioPWMConfig{
    .pwmPin = -1,
};

StorageConfig storageConfig = StorageEEPROMConfig{};

ApexClimb app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
