# PLAMIOmini Game Generation Guidelines for AI

## Purpose

This document defines the rules for generating `.ino` file for **PLAMIOmini**, the Arduino version of PLAMIO.

PLAMIOmini is a small, statically configured game runtime for Arduino-compatible RP2040, RP2350, and ESP32 boards. A game derives from `PLAMIOmini::App` and supplies hardware configuration objects to the runtime.

These rules are intended for AI-generated game code. Follow them strictly.

---

## Source of Truth

For every generation task, use the supplied files as the only authoritative specification.

Priority order:

1. `PLAMIOmini.h`
2. The supplied board example
3. This `PLAMIOmini_AI_GUIDELINES.md`
4. The user's game design document

Do not rely on:

- Online documentation
- GitHub repositories
- Cached knowledge
- Previous PLAMIO or PLAMIOmini versions
- APIs remembered from another framework
- Arduino libraries not included or approved by the user

Never invent an API. If a requested feature is not provided by PLAMIOmini, implement it inside the game class using fixed-size data structures.

Before generating code, verify all overridden function signatures directly against the supplied `App` declaration.

---

## Hardware Configuration

Read `PLAMIOmini_HARDWARE_CONFIG.md` before generating code.

Rules

- Use only hardware marked with `[x]`.
- Ignore hardware marked with `[ ]`.
- Never generate code for disabled hardware.
- Never mix multiple hardware implementations in the same category.
- Never invent hardware or pin assignments.
- Use the selected hardware exactly as described.
- `PLAMIOmini_HARDWARE_CONFIG.md` is the only source of truth for hardware selection and pin assignments.

The Board section identifies the target Arduino platform.

The Display, Input, Audio and Storage sections determine which configuration objects must be generated.

---

## Hardware Mapping

The selected hardware determines which public configuration type must be used.

### Display

| Selected Hardware | Configuration |
|-------------------|----------------|
| ILI9341 | `GraphicsILI9341Config` |
| SSD1306 | `GraphicsSSD1306Config` |

### Input

| Selected Hardware | Configuration |
|-------------------|----------------|
| SNES Controller | `InputSnesConfig` |
| GPIO Buttons | `InputGpioButtonsConfig` |

### Audio

| Selected Hardware | Configuration |
|-------------------|----------------|
| PWM | `AudioPWMConfig` |
| I2S | `AudioI2SConfig` |
| None | `AudioStubConfig` |

### Storage

| Selected Hardware | Configuration |
|-------------------|----------------|
| SD Card | `StorageSDConfig` |
| EEPROM | `StorageEEPROMConfig` |
| None | `StorageStubConfig` |

The generated sketch shall:

- Create one configuration object for each selected hardware category.
- Do not include or instantiate concrete hardware driver classes. The runtime owns them.
- Create one App-derived game instance.
- Pass the four configuration objects and game to `PLAMIOmini::start()` from `setup()`.
- Leave Arduino `loop()` empty.

Object declaration order:

- Display
- Input
- Storage
- Audio
- Game

`InputGpioButtonsConfig` contains its `ButtonMapping` as `buttonMapping`.
- `AudioStub` does not require a configuration object.
- `StorageStub` does not require a configuration object.
- `StorageEEPROM` may be constructed without a configuration object when default settings are sufficient.

---

## PLAMIOmini Is Not Full PLAMIO

Do not copy assumptions from the full PLAMIO framework.

PLAMIOmini uses:

- Namespace: `PLAMIOmini`
- Base class: `App`
- Arduino sketch entry point
- A fixed game instance passed to `PLAMIOmini::start()`
- `void onUpdate(...)`
- Internal game modes defined by the game itself

PLAMIOmini does **not** provide these full-PLAMIO concepts unless they appear in the supplied header:

- `PLAMIO::Game`
- `GameState`
- `getName()` / `getMenuName()` virtual functions
- Logical-screen virtual functions on the game class
- A launcher-controlled game transition returned from `onUpdate()`

Never generate those APIs for PLAMIOmini.

---

## Standard Arduino Sketch Structure

Unless the user explicitly requests multiple files, generate **one complete `.ino` file**.

The normal structure is:

- Required includes
- `using namespace PLAMIOmini;`
- Hardware configuration
- Game class derived from `App`
- Global hardware configuration objects
- Global game object
- `setup()`
- Empty `loop()`

Typical structure:

```cpp
#include <PLAMIOmini.h>

using namespace PLAMIOmini;

// Hardware configuration functions
// Game class
// Global hardware configuration objects
// Game instance

void setup()
{
    PLAMIOmini::start(graphicsConfig, inputConfig, storageConfig, audioConfig, game);
}

void loop()
{
}
```

`PLAMIOmini::start()` owns the main execution loop and normally does not return. Do not place game processing in Arduino `loop()`.

---

## Hardware Configuration Rules

Hardware configuration is board-specific.

- Copy the configuration types, pin assignments, SPI host, frequencies, rotation, and buffer sizes from the supplied board example.
- Do not guess pins.
- Do not silently change hardware settings while implementing a game.
- Do not replace a storage, input, audio, or graphics driver unless the user requests it.
- Preserve known-working board configuration code exactly whenever practical.

When the user supplies a working board template, normally modify only:

- The game class
- Game-specific constants
- Game-specific static assets
- The game object type/name

Do not move hardware initialization into the game class.

---

## Required App Overrides

A game class must derive from `App` and override all four functions with the exact signatures below:

```cpp
class MyGame : public App
{
protected:
    void onInit(Storage& storage) override;
    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override;
    bool onDraw(Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(Storage& storage) override;
public:
    const char* getId() const override;
};
```

In a one-file Arduino sketch, defining the functions directly inside the class is allowed and often preferred.

Never call these system entry points from game code:

- `init()`
- `update()`
- `draw()`
- `terminate()`

The runtime calls them automatically. To restart gameplay, create a private function such as `resetGame()` or `startRound()`.

---

## Internal Game Modes

`onUpdate()` returns `void`; therefore, detailed game state must be managed inside the game class.

Use a private enum such as:

```cpp
enum Mode : uint8_t
{
    MODE_TITLE,
    MODE_PLAYING,
    MODE_PAUSED,
    MODE_GAME_OVER
};
```

Recommended flow for action games:

- Title
- Playing
- Paused
- Game over
- Return to title or restart

Do not invent a PLAMIOmini `GameState` return value.

---

## Input Rules

Available buttons are defined by `Input::Button` in the supplied header.

Important rules:

- Pass exactly one button to each input function.
- Never combine buttons with bitwise OR in one call.
- To detect combinations, call the function separately for each button.
- Do not use system-reserved buttons for gameplay.

Correct:

```cpp
if (input.pressed(Input::LEFT) && input.justPressed(Input::A))
{
    // combination
}
```

Wrong:

```cpp
input.pressed(static_cast<Input::Button>(Input::LEFT | Input::A));
```

System-reserved buttons:

- `Input::HOME`
- `Input::VOL_UP`
- `Input::VOL_DOWN`
- `Input::MUTE`

Use these input methods only as declared:

- `pressed()`
- `justPressed()`
- `released()`
- `justReleased()`
- `held()`
- `holdMillis()`
- `repeat()`
- `setRepeatSettings()`

Use `justPressed()` for one-shot actions and menu decisions. Use `repeat()` for cursor movement or grid movement that should repeat after DAS/ARR delay.

---

## Time and Movement

PLAMIOmini targets approximately 30 FPS, but the actual frame rate is not guaranteed.

Never assume one frame equals exactly 1/30 second.

### Real-time gameplay

Use `deltaSec` for:

- Continuous movement
- Velocity and acceleration
- Physics
- Smooth camera movement

Example:

```cpp
playerX += playerSpeed * deltaSec;
```

Use `Platform::getMsec()` and unsigned elapsed-time subtraction for:

- Countdown timers
- Invincibility duration
- Cooldowns
- Result-screen delays
- Blinking based on real time
- Spawn intervals

Example:

```cpp
const uint32_t now = Platform::getMsec();
if (static_cast<uint32_t>(now - startMsec) >= durationMsec)
{
    // elapsed
}
```

Frame counters may be used only for decorative animation where precise real time is unimportant.

For fast objects, split `deltaSec` into bounded physics substeps when necessary. Never create an unbounded loop.

---

## Dirty Rendering

PLAMIOmini transfers the graphics buffer only when `onDraw()` returns `true`.

Follow this exact pattern:

```cpp
bool onDraw(Graphics& graphics, bool requestFullRedraw) override
{
    if (!requestFullRedraw && !dirty)
    {
        return false;
    }

    // Perform all drawing here.

    dirty = false;
    return true;
}
```

Rules:

- Check `requestFullRedraw` and `dirty` at the beginning.
- Set `dirty = true` in `onUpdate()` whenever visible state changes.
- Draw everything before clearing `dirty`.
- Set `dirty = false` only at the very end of a successful draw.
- Return `false` when no drawing is performed.
- Continuously animated games may set `dirty = true` every update while animation is active.
- Time passing alone does not request redraw; active blinking, particles, countdowns, and other timed visual effects must set `dirty = true` while visible.

---

## Graphics Rules

Use only methods and enum values declared in the supplied `Graphics` class.

### Visible screen sizes

- ILI9341: `320 x 240`
- SSD1306: `128 x 64`

Use `Display::ILI9341_SCREEN_W`, `Display::ILI9341_SCREEN_H`, `Display::SSD1306_SCREEN_W`, and `Display::SSD1306_SCREEN_H` when helpful.

Each graphics driver allocates one sprite at startup using the fixed size above. Runtime screen-size changes and larger off-screen buffers are not supported.

### Drawing safety

Graphics primitives clip safely. Manual visibility checks are usually unnecessary.

### Colors

- Store and pass colors as `Graphics::Color`.
- Use predefined colors for common values.
- Use `Graphics::rgb565(r, g, b)` for custom colors.
- Do not use raw `uint16_t` color literals except for imported RGB565 image data.

For SSD1306-oriented games, use:

- `Graphics::SSD1306_OFF`
- `Graphics::SSD1306_ON`

Do not rely on arbitrary RGB colors for monochrome gameplay meaning.

### Text

- Use only fonts declared in `Graphics::Font`.
- Never invent font names.
- Measure text with `getTextWidth()` and `getTextHeight()` when fitting it into a panel or centering it precisely.
- Prefer readable text. On ILI9341, normally prefer `SIZE_18` or larger for primary UI.
- `SIZE_10` is suitable for compact HUD or debug-style text, especially on SSD1306.
- Keep Japanese text short.
- Japanese-capable fonts are only `SIZE_16J`, `SIZE_20J`, and `SIZE_32J`.

### System volume overlay

On the standard ILI9341 runtime, the system may draw its volume OSD near the bottom of the visible screen. Draw the game background to the bottom edge, but avoid placing important text or gauges at or below approximately `Y = 225`.

### Sprites and large assets

- Define bitmap and sprite arrays as `static const`.
- Do not create large mutable image arrays on the stack.
- Keep data alive for every API that stores or uses a pointer asynchronously.
- Use `drawSprite()` only with the exact signature declared in the supplied header.

---

## Audio Rules

Request sound only from `onUpdate()`. Do not call audio playback from `onDraw()`.

### Sound effects

Pass a pointer to a sound preset:

```cpp
audio.playSE(&Audio::SE::NO_1, 1.0f);
```

Do not omit the `&`.

- Valid gain is documented as `0.1f` to `1.0f`.
- Trigger sound only once per event.
- Never call `playSE()` continuously every frame for the same event.
- Multiple SE requests are not queued; a newer request may replace the previous one.
- Custom `SoundStep` and `Sound` data must be `static const` and remain valid until playback finishes.

### Music

- Custom note and music arrays must be `static const`.
- Call `playMusic()` once when music should start.
- Do not call `playMusic()` every frame.
- Call `stopMusic()` when entering pause or when music must end.
- When resuming, call `playMusic()` once to restart the BGM if appropriate.
- Do not call `playMusic(nullptr)` unless the supplied API explicitly permits it.

Music should remain clear on simple tone audio. Prefer short repeating phrases, moderate BPM, limited octave jumps, and rests between phrases.

---

## Storage Rules

PLAMIOmini provides three storage roles.

### SaveData

Use `SaveData` for ordinary key-value save data:

- High score
- Unlocked stages
- Progress
- Game settings

Use `getId()` for all application-specific saved data.

```cpp
SaveData saveData;
saveData.load(storage, getId(), "save.ini");
saveData.setUInt32("high_score", highScore);
saveData.save(storage, getId(), "save.ini");
```

Always check whether storage is available when failure matters. Saving may legitimately fail on `StorageStub` or unavailable hardware.

### UserFile

Use `readUserFile()` / `writeUserFile()` only for custom writable formats.

- Never construct the physical save path manually.
- Supply only the game ID and relative file name.
- For writes of 200 bytes or less, prefer the direct `const char*` overload.
- In writer callbacks, do not use string concatenation.
- Use direct assignment, `std::string::assign()`, or `snprintf()` into a local buffer.

### File

Use `openRead()` only for packaged, read-only resources.

- Open and read resources in `onInit()`.
- Never perform file I/O in `onUpdate()` or `onDraw()`.
- Always call `File::close()` after reading.
- Never use `openRead()` for save data.
- Never attempt to write to a `File` resource.

### Save timing

Do not save every frame.

Mark progress as changed and save at safe points such as:

- Game over
- Stage clear
- Return to title
- `onTerminate()`

---

## Memory and C++ Rules

PLAMIOmini targets memory-constrained microcontrollers.

Prefer:

- Fixed-size arrays
- Plain structs
- `enum` / `enum class`
- `static const` assets
- Stack buffers with explicit size
- `snprintf()`
- `memset()` / `memcpy()`
- `sinf()`, `cosf()`, `fabsf()`, `sqrtf()`, `atan2f()`
- `rand()`

Avoid unless already required by the supplied API:

- `new` / `delete`
- `std::vector`
- `std::map`
- `std::unordered_map`
- Large `std::string` manipulation
- String concatenation
- `std::to_string()`
- `sprintf()`
- `itoa()`
- Custom formatting helpers when `snprintf()` is enough

Include required standard headers explicitly, such as:

- `<cstdint>`
- `<cstdio>`
- `<cstdlib>`
- `<cstring>`
- `<cmath>`

Use the C functions directly (`snprintf`, `memset`, `memcpy`), matching the style of the supplied framework.

The PLAMIOmini runtime initializes random state. Do not call `srand()` unless the supplied version explicitly requires it.


Avoid using common Arduino macro names as identifiers,
  including PI, HALF_PI, TWO_PI, DEG_TO_RAD, RAD_TO_DEG,
  HIGH, LOW, INPUT, OUTPUT, and board-specific pin names.
  Macro substitution may cause unexpected compilation errors.

---

## Gameplay Tuning

Place gameplay tuning values together near the top of the game class or file as clearly named constants.

Examples:

- Player speed
- Enemy speed
- Object sizes
- Spawn interval
- Time limit
- Score rewards
- Gravity
- Jump velocity
- Damage duration

Do not scatter unexplained numeric literals throughout gameplay logic.

Keep hardware constants separate from gameplay constants.

---

## Game Feel and Usability

When appropriate, add modest feedback:

- One-shot sound effects
- Brief screen flash
- Small screen shake using the viewport where supported
- Hit particles
- Score popups
- Short transitions
- Clear game-over feedback

Prioritize:

- Simple controls
- Readable UI
- Immediate feedback
- Stable frame-rate-independent behavior
- A complete playable loop

Do not add effects that obscure gameplay or exceed the target hardware.

---

## Output Rules

When generating a complete game:

- Output compile-ready code, not fragments.
- Default to one `.ino` file.
- Preserve the supplied board configuration.
- Include every required include.
- Define all referenced constants, fields, and functions.
- Do not leave placeholders such as `TODO` unless the user explicitly requests a skeleton.
- Do not omit `setup()` or `loop()` from a complete sketch.
- Do not generate APIs absent from the supplied headers.
- Keep the result suitable for direct placement in an Arduino sketch folder.

When the user asks for game logic only inside an existing template, output only the requested replacement section and do not duplicate the hardware setup.

---

## Arduino Sketch Sample

The generated game must be a complete Arduino `.ino` sketch.

The hardware configuration objects and pin assignments must be generated from
`PLAMIOmini_HARDWARE_CONFIG.md`.

Do not copy hardware values from this sample.

```ino
#include <PLAMIOmini.h>

using namespace PLAMIOmini;

// ============================================================================
// Hardware Configuration
// ============================================================================

// Generate the selected hardware configuration here.
//
// Use only the values from PLAMIOmini_HARDWARE_CONFIG.md.
// Do not invent or reuse pin assignments from another board or sample.

// ============================================================================
// Game
// ============================================================================

class SampleGame : public App
{
protected:
    void onInit(Storage& storage) override
    {
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override 
    {
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }
        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
    }
};

// ============================================================================
// Hardware Configuration Objects
// ============================================================================

// Create exactly one display configuration object.
// Create exactly one input configuration object.
// Create exactly one storage configuration object.
// Create exactly one audio configuration object.
//
// Their types and values must come from PLAMIOmini_HARDWARE_CONFIG.md.
// Declare all hardware configuration objects at global scope.

// ============================================================================
// Game Object
// ============================================================================

SampleGame game;

// ============================================================================
// Arduino Entry Points
// ============================================================================

void setup()
{
    PLAMIOmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
```

The final generated sketch must replace every hardware-related comment with
real code based on `PLAMIOmini_HARDWARE_CONFIG.md`.

Never leave hardware placeholders in the final output.

### Sketch Generation Rules

When generating a complete Arduino sketch:

- Do not remove required `#include` directives.
- Do not replace framework header files with alternative headers.
- Do not rename framework classes.
- Do not modify the generated `setup()` function except for user-requested changes.
- Do not modify the generated `loop()` function.
- Do not move hardware initialization into the game class.
- Do not construct hardware configuration objects inside `setup()`.
- Do not dynamically allocate framework objects.
- Declare all hardware configuration objects and the game object at global scope.
- Pass the global hardware configuration objects and game object to `PLAMIOmini::start()` from `setup()`.

---

## Generation Priorities

1. The generated code must compile against the supplied headers.
2. The game must be complete and playable.
3. The implementation should follow the requested design.
4. Visual polish and balance may be refined after hardware testing.

---

## Verification Checklist

Before presenting generated code, verify:

1. The namespace is `PLAMIOmini`.
2. The game derives from `App`.
3. All four override signatures match exactly.
4. `onUpdate()` returns `void`.
5. No `GameState` or full-PLAMIO virtual metadata functions were invented.
6. Every input call receives exactly one button.
7. Reserved volume/home buttons are not used for gameplay.
8. Movement uses `deltaSec` where required.
9. Real-time timers use milliseconds rather than frame assumptions.
10. `onDraw()` follows the dirty-rendering contract.
11. Sound is requested only from `onUpdate()` and only once per event.
12. Preset SE values are passed by pointer using `&`.
13. Static audio and image data remain valid for their required lifetime.
14. Resource file I/O occurs only during initialization.
15. Save paths are not manually constructed.
16. No dynamic containers or unnecessary heap allocation were introduced.
17. All fonts, colors, and graphics calls exist in the supplied header.
18. The selected hardware configuration matches the supplied working example.
19. The sketch includes global hardware configuration objects and one game instance.
20. `setup()` calls `PLAMIOmini::start(graphicsConfig, inputConfig, storageConfig, audioConfig, game)` and `loop()` is empty.

---

## Mandatory Compile-Safety Verification

Before presenting the final code, inspect every PLAMIOmini symbol used in the
generated sketch and verify that it exists exactly in the supplied headers.

Verify at minimum:

- Every `#include` path
- Every class name
- Every configuration type and field
- Every configuration value
- Every overridden function signature
- Every `Graphics::Font` value
- Every `Graphics::Color` value
- Every `Graphics` method and argument list
- Every `Input::Button` value
- Every `Audio::SE` value
- Every Storage API call

Never infer a symbol from naming patterns.

For example, the existence of `SIZE_22B` and `SIZE_25B` does not imply that
`SIZE_24B` exists.

---

## Recommended Workflow

1. Read `PLAMIOmini.h` and the selected board example.
2. Read the user's game design.
3. Resolve the screen, controls, game loop, scoring, and save behavior.
4. Present or confirm the final compact specification when needed.
5. Generate the complete Arduino sketch.
6. Perform the verification checklist before final output.




Do not redesign PLAMIOmini itself while generating a game. Implement the game using the framework as supplied.
