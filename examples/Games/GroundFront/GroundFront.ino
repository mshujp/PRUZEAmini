/*
===============================================================================
 PRUZEAmini Playable Game
 Ground Front
===============================================================================

A complete vertical shooter with five boss battles, weapons, bombs,
ranking save data, music, and sound effects.

Controls:
- D-PAD : Move
- A     : Shot / Confirm
- B     : Bomb / Back
- X     : Ranking
- START : Start / Pause

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>
#include "GroundFront.h"

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
        .A        = -1, // SHOT / CONFIRM
        .B        = -1, // BOMB / BACK
        .X        = -1, // RANKING
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

// =============================================================================
// PRUZEAmini objects
// =============================================================================

GroundFront app;

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
