/*
===============================================================================
 PRUZEAmini Example
 19_SpriteSheet
===============================================================================

This example shows how to use a Graphics::SpriteSheet to store several
sprites inside a single bitmap, and how to pick one sprite out of the sheet
with drawSprite(sheet, column, row, x, y, options).

The sprite sheet bitmap used in this example is the same 64x64 / 4x4 dummy
sheet used by the PRUZEA (Pico SDK) API showcase. It is reused here as-is,
adapted to the PRUZEAmini Arduino API.

What this example demonstrates:
- Defining a Graphics::SpriteSheet from a single flat bitmap array.
- Drawing the whole sheet at once (for reference / debugging).
- Drawing a single cell of the sheet, selected at runtime.
- Using the D-Pad (with button repeat) to move a selection cursor over the
  sheet's grid, so the user can browse every sprite the sheet contains.

Controls:
- D-Pad : Move the cell cursor (UP / DOWN / LEFT / RIGHT), wraps around.
- A     : Cycle preview zoom (x4 / x6 / x8)
- B     : Toggle transparency (magenta color-key) on/off

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
*/

#include <PRUZEAmini.h>

using namespace PRUZEAmini;


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
StorageConfig storageConfig = StorageStubConfig{};


// =============================================================================
// Sprite sheet resource
// -----------------------------------------------------------------------------
// - Sprite bitmap data must always be `static const uint16_t` at file/class
//   scope so it is stored in flash instead of SRAM.
// - This 64x64 bitmap packs a 4 (columns) x 4 (rows) grid of 16x16 sprites.
// - Magenta (0xF81F) is used as the color-key (transparent) color throughout
//   this sheet.
// =============================================================================

constexpr Graphics::Color _BK = Graphics::rgb565(0, 0, 0);          // BLACK
constexpr Graphics::Color _WT = Graphics::rgb565(255, 255, 255);    // WHITE
constexpr Graphics::Color _GR = Graphics::rgb565(132, 132, 130);    // GRAY
constexpr Graphics::Color _DG = Graphics::rgb565(66, 66, 66);       // DARKGRAY
constexpr Graphics::Color _RD = Graphics::rgb565(248, 0, 0);        // RED
constexpr Graphics::Color _YL = Graphics::rgb565(255, 255, 0);      // YELLOW
constexpr Graphics::Color _BL = Graphics::rgb565(0, 0, 248);        // BLUE
constexpr Graphics::Color _GN = Graphics::rgb565(0, 248, 0);        // GREEN
constexpr Graphics::Color _CN = Graphics::rgb565(0, 255, 255);      // CYAN
constexpr Graphics::Color _MG = Graphics::rgb565(248, 0, 248);      // MAGENTA (transparent key)
constexpr Graphics::Color _OR = Graphics::rgb565(250, 160, 0);      // ORANGE
constexpr Graphics::Color _BR = Graphics::rgb565(160, 80, 40);      // BROWN

static const uint16_t SPRITE_SHEET_DATA[64 * 64] = {
    _MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_DG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_DG,_MG,_MG,_MG,  _MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_YL,_YL,_MG,_MG,_YL,_YL,_MG,_MG,_YL,_YL,_MG,_MG,_MG,  _MG,_MG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_MG,_MG,  _MG,_MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,
    _MG,_MG,_YL,_YL,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_YL,_YL,_MG,_MG,  _MG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_MG,  _MG,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,  _MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,_MG,
    _MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,  _MG,_DG,_WT,_WT,_DG,_WT,_WT,_WT,_WT,_WT,_WT,_DG,_WT,_WT,_DG,_MG,  _MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,  _MG,_DG,_WT,_WT,_WT,_WT,_WT,_DG,_DG,_WT,_WT,_WT,_WT,_WT,_DG,_MG,  _MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_YL,_YL,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _DG,_DG,_WT,_DG,_WT,_WT,_DG,_DG,_DG,_DG,_WT,_WT,_DG,_WT,_DG,_DG,  _MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,  _MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_YL,_YL,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _DG,_DG,_WT,_DG,_WT,_WT,_DG,_DG,_DG,_DG,_WT,_WT,_DG,_WT,_DG,_DG,  _MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_BK,_BK,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _MG,_DG,_WT,_WT,_WT,_WT,_WT,_DG,_DG,_WT,_WT,_WT,_WT,_WT,_DG,_MG,  _MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_BK,_BK,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _MG,_DG,_WT,_WT,_DG,_WT,_WT,_WT,_WT,_WT,_WT,_DG,_WT,_WT,_DG,_MG,  _MG,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_BK,_BK,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _MG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_MG,  _MG,_MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_BK,_BK,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _MG,_MG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_MG,_MG,  _MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_BK,_BK,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _MG,_MG,_MG,_DG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_DG,_MG,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_WT,_WT,_MG,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,
    _MG,_YL,_WT,_WT,_WT,_YL,_YL,_BK,_BK,_YL,_YL,_WT,_WT,_WT,_YL,_MG,  _MG,_MG,_MG,_MG,_DG,_WT,_WT,_DG,_DG,_WT,_WT,_DG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_WT,_WT,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,

    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_OR,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_BL,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_MG,_MG,  _MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,
    _MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_WT,_WT,_WT,_WT,_WT,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_DG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_DG,_MG,_MG,_MG,
    _MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_BL,_MG,_MG,  _MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,

    _MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_RD,_RD,_RD,_RD,_RD,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,  _MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,  _MG,_MG,_YL,_YL,_YL,_MG,_MG,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,  _MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_CN,_CN,_WT,_WT,_WT,_WT,_WT,_WT,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,_MG,  _MG,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_DG,_DG,_DG,_DG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,

    _MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_MG,_MG,  _RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,  _RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,
    _MG,_MG,_MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,  _MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,
    _MG,_MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,
    _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,
    _GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,  _MG,_MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_GN,_GN,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG
};

// Sheet layout: 16x16 px per sprite, 4 columns x 4 rows (16 sprites total).
static const Graphics::SpriteSheet SPRITE_SHEET(
    SPRITE_SHEET_DATA,
    16,  // sprite width
    16,  // sprite height
    4,   // columns
    4    // rows
);


// =============================================================================
// Application
// =============================================================================

class SpriteSheetGame : public App
{
public:
    const char* getId() const override { return "sprite_sheet"; }

private:
    // Cursor position on the sheet grid.
    uint16_t cursorColumn = 0;
    uint16_t cursorRow = 0;

    // Preview zoom cycles between these scale values with the A button.
    static constexpr uint8_t ZOOM_LEVELS[3] = {4, 5, 6};
    uint8_t zoomIndex = 1;

    bool transparentPreview = true;

    // Layout constants.
    static constexpr int16_t SHEET_X = 22;
    static constexpr int16_t SHEET_Y = 56;
    static constexpr uint8_t SHEET_SCALE = 2;                 // overview scale (64x64 -> 128x128 on screen)
    static constexpr int16_t CELL_PX = 16 * SHEET_SCALE;      // on-screen size of one grid cell

    // The preview panel is centered on this point regardless of the current
    // zoom level, so the panel does not run off the right/bottom edge of
    // the screen at the largest zoom level.
    static constexpr int16_t PREVIEW_CENTER_X = 238;
    static constexpr int16_t PREVIEW_CENTER_Y = 128;

    void drawHeader(Graphics& graphics)
    {
        graphics.fillRect(0, 0, 320, 34, Graphics::DARKGRAY);
        graphics.drawString(
            "SPRITE SHEET",
            12, 17,
            Graphics::WHITE,
            Graphics::SIZE_22B,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            "4 x 4 CELLS",
            308, 17,
            Graphics::CYAN,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::MIDDLE);
    }

    void drawFooter(Graphics& graphics)
    {
        graphics.drawString(
            "D-Pad: Select Cell   A: Zoom   B: Transparency",
            160, 220,
            Graphics::LIGHTGRAY,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

    // Draws the entire sheet (as a plain bitmap) so the user can see every
    // sprite at once, then overlays grid lines and a cursor box on top of
    // the cell that is currently selected.
    void drawSheetOverview(Graphics& graphics)
    {
        graphics.drawSprite(
            SPRITE_SHEET_DATA,
            SHEET_X, SHEET_Y,
            64, 64,
            {
                .scale = SHEET_SCALE,
                .transparent = true,
                .transparentColor = Graphics::MAGENTA
            });

        graphics.drawRect(SHEET_X, SHEET_Y, 64 * SHEET_SCALE, 64 * SHEET_SCALE, 1, Graphics::GRAY);

        // Grid lines separating the 4 columns / 4 rows.
        for (uint8_t i = 1; i < SPRITE_SHEET.columns; ++i)
        {
            int16_t x = SHEET_X + i * CELL_PX;
            graphics.drawLine(x, SHEET_Y, x, SHEET_Y + 64 * SHEET_SCALE, Graphics::DARKGRAY);
        }
        for (uint8_t i = 1; i < SPRITE_SHEET.rows; ++i)
        {
            int16_t y = SHEET_Y + i * CELL_PX;
            graphics.drawLine(SHEET_X, y, SHEET_X + 64 * SHEET_SCALE, y, Graphics::DARKGRAY);
        }

        // Cursor box around the selected cell.
        int16_t cx = SHEET_X + cursorColumn * CELL_PX;
        int16_t cy = SHEET_Y + cursorRow * CELL_PX;
        graphics.drawRect(cx, cy, CELL_PX, CELL_PX, 2, Graphics::CYAN);
    }

    // Draws a zoomed-in preview of just the selected sprite, using the
    // SpriteSheet overload of drawSprite() (column/row based lookup).
    void drawSelectedPreview(Graphics& graphics)
    {
        const uint8_t scale = ZOOM_LEVELS[zoomIndex];
        const int16_t previewSize = 16 * scale;
        const int16_t previewX = PREVIEW_CENTER_X - previewSize / 2;
        const int16_t previewY = PREVIEW_CENTER_Y - previewSize / 2;

        graphics.fillRoundRect(
            previewX - 10, previewY - 10,
            previewSize + 20, previewSize + 20,
            8, Graphics::rgb565(14, 28, 48));
        graphics.drawRoundRect(
            previewX - 10, previewY - 10,
            previewSize + 20, previewSize + 20,
            8, 2, Graphics::CYAN);

        graphics.drawSprite(
            SPRITE_SHEET,
            cursorColumn, cursorRow,
            previewX, previewY,
            {
                .scale = scale,
                .transparent = transparentPreview,
                .transparentColor = Graphics::MAGENTA
            });

        char info[32];
        snprintf(info, sizeof(info), "COL:%u  ROW:%u", cursorColumn, cursorRow);
        graphics.drawString(
            info,
            PREVIEW_CENTER_X, previewY + previewSize + 20,
            Graphics::WHITE,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        char zoomText[16];
        snprintf(zoomText, sizeof(zoomText), "x%u", scale);
        graphics.drawString(
            zoomText,
            PREVIEW_CENTER_X, previewY - 20,
            Graphics::YELLOW,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        cursorColumn = 0;
        cursorRow = 0;
        zoomIndex = 1;
        transparentPreview = true;
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)audio;
        (void)storage;
        (void)deltaSec;

        // D-Pad moves the cursor across the sheet grid, wrapping around at
        // the edges. repeat() fires immediately on press and then again at
        // a fixed rate while the button is held, which feels natural for
        // browsing a grid of cells.
        if (input.repeat(Input::RIGHT))
        {
            cursorColumn = (cursorColumn + 1) % SPRITE_SHEET.columns;
            dirty = true;
        }
        if (input.repeat(Input::LEFT))
        {
            cursorColumn = (cursorColumn + SPRITE_SHEET.columns - 1) % SPRITE_SHEET.columns;
            dirty = true;
        }
        if (input.repeat(Input::DOWN))
        {
            cursorRow = (cursorRow + 1) % SPRITE_SHEET.rows;
            dirty = true;
        }
        if (input.repeat(Input::UP))
        {
            cursorRow = (cursorRow + SPRITE_SHEET.rows - 1) % SPRITE_SHEET.rows;
            dirty = true;
        }

        if (input.justPressed(Input::A))
        {
            zoomIndex = (zoomIndex + 1) % 3;
            dirty = true;
        }

        if (input.justPressed(Input::B))
        {
            transparentPreview = !transparentPreview;
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

        drawHeader(graphics);
        drawSheetOverview(graphics);
        drawSelectedPreview(graphics);
        drawFooter(graphics);

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

SpriteSheetGame app;


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
