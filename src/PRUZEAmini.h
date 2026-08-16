// ============================================================================
// PRUZEA mini
// AI-Friendly Game / UI Application Framework
//
// PRUZEA mini is a lightweight framework for creating games and UI applications with AI assistance.
//
// This header defines the public API available to application.
// The runtime implementation is provided by the PRUZEA mini system.
//
// Execution model
//   System (30 FPS)
//       ├─ Input update
//       ├─ App::onUpdate()
//       ├─ App::onDraw()
//       └─ Graphics::push()
// ============================================================================

#pragma once
#include <stdint.h>
#include <variant>
#include <string>

namespace PRUZEAmini
{

constexpr char PRUZEA_MINI_VERSION[] = "1.2.1";

// =========================================================================
// [PROVIDED BY SYSTEM]
// All APIs declared in this header are provided by the PRUZEA system.
// They are already implemented by the runtime and do not need to be
// implemented or redefined in your application.
// =========================================================================
namespace Platform {
    uint32_t getMsec(); // Returns the number of milliseconds since system startup.
    uint64_t getUsec(); // Returns the number of microseconds since system startup.
    bool elapsed(uint32_t now, uint32_t startMsec, uint32_t durationMsec);
}
namespace Math {
    template<typename T>
    T clamp(T value, T min, T max); // All arguments must have the same type.
    float lerp(float a, float b, float t);
    float moveTowards(float current, float target, float maxDelta);
    float moveTowardsAngle(float current, float target, float maxDelta);
    float length(float x, float y);
    float lengthSquared(float x, float y);
    float distance(float ax, float ay, float bx, float by);
    float distanceSquared(float ax, float ay, float bx, float by);
    float dot(float ax, float ay, float bx, float by);
    float sqrt(float value);
    float abs(float value);
    float round(float value);
    float floor(float value);
    float ceil(float value);
    float wrap(float value, float min, float max);
    float sin(float radians);
    float cos(float radians);
    float tan(float radians);
    float asin(float value);
    float acos(float value);
    float atan(float value);
    float atan2(float y, float x);
    void rotate(float x, float y, float radians, float& outX, float& outY);
    void normalize(float& x, float& y); // Normalize the vector. If the vector is zero, it is left unchanged.
    float angle(float x, float y); /// Returns the absolute angle of vector (x, y), in radians. Equivalent to atan2f(y, x). Return range: -PI to PI.
    float deltaAngle(float current, float target);
    float lerpAngle(float current, float target, float t);
    constexpr float Pi      = 3.14159265358979323846f;
    constexpr float HalfPi = Pi * 0.5f;
    constexpr float TwoPi  = Pi * 2.0f;
    float degToRad(float degrees);
    float radToDeg(float radians);
    int random(int max);          // [0, max)
    int random(int min, int max); // [min, max)
    float randomFloat();          // [0.0f, 1.0f)
    float randomFloat(float max); // [0.0f, max)
    float randomFloat(float min, float max); // [min, max)
    bool chance(float probability); // 0.0: 0%, 1.0: 100% (Returns true with the given probability (0.0f to 1.0f).)
    float map(float value, float inMin, float inMax, float outMin, float outMax);
    void reflect(float inX, float inY, float normalX, float normalY, float& outX, float& outY);
    float smoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaSec);
}
class Animation {
public:
    Animation(float duration, int totalFrames, bool loop = false);
    void start();
    void stop();
    void reset();
    void update(float deltaSec);
    bool isPlaying() const;
    float progress() const;
    bool isFinished() const;
    int frame() const;
private:
    float currentTime = 0.0f;
    float duration = 1.0f;
    int totalFrames = 1;
    bool playing = false;
    bool loop = false;
};
class Vector2 {
public:
    float x;
    float y;
    Vector2(float x = 0.0f, float y = 0.0f);
    float length() const;
    Vector2 normalized() const;
    float dot(const Vector2& other) const;
    float cross(const Vector2& other) const;
    float distance(const Vector2& other) const;

    Vector2 operator+(const Vector2& other) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2& operator+=(const Vector2& other);
    Vector2& operator-=(const Vector2& other);
    Vector2 operator*(float scalar) const;
    Vector2 operator/(float scalar) const;
    Vector2& operator*=(float scalar);
    Vector2& operator/=(float scalar);
};
Vector2 operator*(float scalar, const Vector2& vector);
namespace Collision {
    bool pointRect(float px, float py, float rx, float ry, float rw, float rh);
    bool rectRect(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh);
    bool rectRect(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh, Vector2& pushOut);
    bool circleCircle(float ax, float ay, float ar, float bx, float by, float br);
    bool circleCircle(float ax, float ay, float ar, float bx, float by, float br, Vector2& pushOut);
    bool circleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh);
    bool circleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh, Vector2& pushOut);
    bool pointCircle(float px, float py, float cx, float cy, float radius);
    bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    bool lineRect(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh);
    bool lineCircle(float x1, float y1, float x2, float y2, float cx, float cy, float radius);
    bool raycast(float x, float y, float dx, float dy, float rx, float ry, float rw, float rh, float& outHitX, float& outHitY);
}
namespace Display {
    // Physical display resolutions.
    static constexpr uint16_t SSD1306_SCREEN_W = 128;
    static constexpr uint16_t SSD1306_SCREEN_H = 64;
    static constexpr uint16_t ILI9341_SCREEN_W = 320;
    static constexpr uint16_t ILI9341_SCREEN_H = 240;
}

// --- ======================
// # Input
// [!IMPORTANT] CRITICAL AI RULE FOR BUTTON INPUT:
//  - NEVER combine multiple buttons in a single function call.
//  - Functions like pressed() or justPressed() accept EXACTLY ONE button.
//  - Bad:  input.pressed(Input::LEFT | Input::A);
//  - Good: input.pressed(Input::LEFT) && input.pressed(Input::A);
// =========================
class Input {
public:
   // ## Input Style button layout:
   // - "Nintendo Style"
   //
   //        X
   //    Y       A
   //        B
   //
   //   A = Confirm, B = Cancel
   //
   // - "XInput Style"
   //
   //        Y
   //    X       B
   //        A
   //
   //   A = Confirm, B = Cancel
   //
   // - "Simple Style"
   //
   //    B       A
   //
   //   A = Confirm, B = Cancel

    enum Button : uint32_t {
        UP       = 1u << 0,
        DOWN     = 1u << 1,
        LEFT     = 1u << 2,
        RIGHT    = 1u << 3,
        A        = 1u << 4,
        B        = 1u << 5,
        X        = 1u << 6,
        Y        = 1u << 7,
        L        = 1u << 8,
        R        = 1u << 9,
        START    = 1u << 10,
        SELECT   = 1u << 11,
        L2       = 1u << 12,
        R2       = 1u << 13,
        L3       = 1u << 14,
        R3       = 1u << 15,

        // System-reserved virtual buttons. Hardware implementations may map
        // physical buttons or shortcuts to these.
        VOL_UP   = 1u << 25,
        VOL_DOWN = 1u << 26,
        MUTE     = 1u << 27
    };
    virtual bool pressed(Button b) const = 0;
    virtual bool justPressed(Button b) const = 0;
    virtual bool released(Button b) const = 0;
    virtual bool justReleased(Button b) const = 0;
    virtual bool held(Button b) const = 0;
    virtual uint64_t holdMillis(Button b) const = 0;

    // ## Button repeat settings
    // Default values are applied before App::onInit() is called.
    static constexpr uint16_t DAS_DELAY_MSEC_DEFAULT = 150;
    static constexpr uint16_t ARR_DELAY_MSEC_DEFAULT = 50;

    // ## Button repeat settings
    // Default values are applied before App::onInit().
    // Call this only when changing the default repeat timing.
    virtual void setRepeatSettings(uint16_t dasDelayMsec, uint16_t arrDelayMsec) = 0;
    virtual bool repeat(Button b) const = 0;

    // ## Analog stick input
    enum Axis : uint8_t
    {
        LEFT_X,
        LEFT_Y,
        RIGHT_X,
        RIGHT_Y
    };
    virtual bool hasAnalogSticks() const { return false; }
    // Returns a normalized value from -1000 to 1000.
    // Returns 0 when the axis is unavailable or inside the dead zone.
    virtual int16_t axis(Axis axis) const { return 0; }

    // ## Touchscreen input
    // Coordinates are converted to the visible screen coordinate system.
    // Coordinates return -1 while the screen is not being touched.
    virtual bool touched() const { return false; }
    virtual bool justTouched() const { return false; }
    virtual bool justTouchReleased() const { return false; }
    virtual int16_t touchX() const { return -1; }
    virtual int16_t touchY() const { return -1; }

protected:
    virtual ~Input() {};
};

// GPIO pin numbers associated with PRUZEAmini logical buttons.
// Used by GPIO button input and optional extra GPIO buttons.
// Use -1 for buttons that are not connected.
struct ButtonPins
{
    int16_t UP       = -1;
    int16_t DOWN     = -1;
    int16_t LEFT     = -1;
    int16_t RIGHT    = -1;
    int16_t A        = -1;
    int16_t B        = -1;
    int16_t X        = -1;
    int16_t Y        = -1;
    int16_t L        = -1;
    int16_t R        = -1;
    int16_t L2       = -1;
    int16_t R2       = -1;
    int16_t L3       = -1;
    int16_t R3       = -1;
    int16_t START    = -1;
    int16_t SELECT   = -1;
    int16_t VOL_UP   = -1;
    int16_t VOL_DOWN = -1;
    int16_t MUTE     = -1;
};
struct InputGpioButtonsConfig {
    ButtonPins gpioButtonPins{};
};
struct InputSnesConfig {
    int8_t clkPin  = -1;
    int8_t latPin  = -1;
    int8_t dataPin = -1;
    ButtonPins extraGpioButtonPins{};
};
struct InputPSConfig
{
    int8_t clockPin = -1;
    int8_t commandPin = -1;
    int8_t attentionPin = -1;
    int8_t dataPin = -1;

    // Analog Stick
    uint8_t analogDeadZone = 24;
    uint8_t leftXCenter = 128;
    uint8_t leftYCenter = 128;
    uint8_t rightXCenter = 128;
    uint8_t rightYCenter = 128;

    ButtonPins extraGpioButtonPins{};
};
struct InputStubConfig
{
};
struct TouchXPT2046Config
{
    uint8_t spiHost = 0;
    uint32_t spiFreq = 2000000;
    int8_t clkPin = -1;
    int8_t mosiPin = -1;
    int8_t misoPin = -1;
    int8_t csPin = -1;
    int8_t irqPin = -1;

    int16_t minX = 250;
    int16_t maxX = 3850;
    int16_t minY = 250;
    int16_t maxY = 3850;
    int16_t minZ = 2048;
    uint8_t offsetRotation = 5;
};
template<typename BaseInputConfig>
struct InputTouchConfig
{
    BaseInputConfig input{};
    TouchXPT2046Config touch{};
};
using InputConfig = std::variant<
    InputGpioButtonsConfig,
    InputSnesConfig,
    InputPSConfig,
    InputTouchConfig<InputStubConfig>,
    InputTouchConfig<InputGpioButtonsConfig>,
    InputTouchConfig<InputSnesConfig>,
    InputTouchConfig<InputPSConfig>
>;

// --- =================================================================
// # Graphics
//   - All graphics operations must be performed through this class.
//   - Supported displays:
//     - ILI9341
//        - Graphics buffer size: 320x240 (same as the physical display)
//     - SSD1306
//       - Graphics buffer size: up to 128x64
//   - Drawing outside the visible screen is always safe. Graphics automatically clips all primitives internally.
//     The app does NOT need to perform screen boundary checks before drawing.
//     Prefer simple drawing code over manual visibility checks.
// ====================================================================
class Graphics {
public:
    // -------------------------------------------------------------------------
    // RGB565 color value.
    // Use these constants for common colors, or use Graphics::rgb565(r, g, b)
    // to create any 24-bit RGB color converted to RGB565.
    // -------------------------------------------------------------------------
    enum Color : uint16_t
    {
        BLACK     = 0x0000,
        DARKGRAY  = 0x4208,
        GRAY      = 0x8410,
        LIGHTGRAY = 0xC618,
        WHITE     = 0xFFFF,
        RED       = 0xF800,
        GREEN     = 0x07E0,
        BLUE      = 0x001F,
        YELLOW    = 0xFFE0,
        CYAN      = 0x07FF,
        MAGENTA   = 0xF81F,
        ORANGE    = 0xFD20,
        PURPLE    = 0x8010,
        PINK      = 0xFC18,
        SSD1306_OFF = BLACK,
        SSD1306_ON  = WHITE
    };
    constexpr static Color rgb565(uint8_t r, uint8_t g, uint8_t b)
    {
        return static_cast<Color>( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3) );
    }

    enum class HorizontalAlign : uint8_t {
        LEFT,
        CENTER,
        RIGHT
    };
    enum class VerticalAlign : uint8_t {
        TOP,
        MIDDLE,
        BOTTOM
    };

    virtual uint16_t getWidth() const = 0;
    virtual uint16_t getHeight() const = 0;
    virtual void clearScreen() = 0;
    virtual void fillScreen(Color color) = 0;
    virtual void drawPixel(int16_t x, int16_t y, Color color) = 0;
    virtual void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color)= 0;
    virtual void drawWideLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, Color color) = 0;
    virtual void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) = 0;
    virtual void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t thickness, Color color) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t thickness, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Color color) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t thickness, Color color) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t thickness, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color) = 0;
    virtual void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color) = 0; // alpha: 255 = fully visible
    virtual void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color, HorizontalAlign ha, VerticalAlign va) = 0; // alpha: 255 = fully visible
    virtual void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Color color) = 0;
    virtual void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Color color, HorizontalAlign ha, VerticalAlign va) = 0;   
    virtual void drawCircle(int16_t x, int16_t y, uint16_t r, Color color) = 0;
    virtual void drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color) = 0;
    virtual void fillCircle(int16_t x, int16_t y, uint16_t r, Color color) = 0;
    virtual void fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color) = 0;

    // ## drawArc / fillArc
    // - angle0 / angle1: Start and end angles in radians. 0 radians is to the right.
    // - rx / ry: Horizontal and vertical radius.
    // - w: Arc thickness.
    virtual void drawArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) = 0;
    virtual void drawArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float angle0, float angle1, Color color) = 0;
    virtual void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) = 0;
    virtual void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float angle0, float angle1, Color color) = 0;
    virtual void fillArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) = 0;
    virtual void fillArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float angle0, float angle1, Color color) = 0;
    virtual void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) = 0;
    virtual void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float angle0, float angle1, Color color) = 0;

    enum FillStyle : uint8_t {
        HORIZONRAL_LINEAR,
        VERTICAL_LINEAR,
        RADIAL_CENTER
    };
    virtual void fillRectGradient(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color0, Color color1, FillStyle style) = 0;
    virtual void fillRectGradient(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color0, Color color1, FillStyle style, HorizontalAlign ha, VerticalAlign va) = 0;     

    // Draws a quadratic Bezier curve using three control points.
    virtual void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) = 0;
    // Draws a cubic Bezier curve using four control points.
    virtual void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color) = 0;
    
    // [!IMPORTANT] AI WARNING:
    //   Use only the enum values defined below.
    //   Never invent or guess font names.
    //   e.g., Do not use SIZE_14, SIZE_24. SIZE_18J, SIZE_25J, or SIZE_42J.
    //   Undefined font names will not compile.
    enum Font : uint8_t {
        // When the text width is required, always use `getTextWidth()` to obtain it.
        SIZE_10,  // very small / debug                   (height: 10px) English only
        SIZE_13,  // HUD                                  (height: 13px) English only
        SIZE_18,  // normal menu text                     (height: 18px) English only
        SIZE_22,  // normal menu text                     (height: 22px) English only
        SIZE_22B, // Bold normal menu text                (height: 22px) English only
        SIZE_25,  // large text / pause / game over       (height: 25px) English only
        SIZE_25B, // Bold  large text / pause / game over (height: 25px) English only
        SIZE_32,  // title                                (height: 32px) English only
        SIZE_32B, // Bold title                           (height: 32px) English only
        SIZE_42,  // big title                            (height: 40px) English only
        SIZE_42B, // Bold big title                       (height: 42px) English only
    };
    virtual uint16_t getTextWidth(const char* text, Font font) = 0;
    virtual uint16_t getTextHeight(const char* text, Font font) = 0;
    virtual void drawString(const char* str, int16_t x, int16_t y, Color color, Font font) = 0;
    virtual void drawString(const char* str, int16_t x, int16_t y, Color color, Font font, HorizontalAlign ha, VerticalAlign va) = 0;

    // ## Sprites (bitmap images)
    //   - `bitmap` must point to RGB565 (16-bit) pixel data.
    //   - [!IMPORTANT] CRITICAL AI RULE FOR BITMAP DATA
    //     - Define all sprite bitmap arrays as `static const uint16_t` at global or class scope.
    //     - Never declare bitmap arrays inside functions.
    //     - Mutable bitmap arrays waste SRAM and may cause Stack Overflow or Out of Memory.
    //       SRAM is limited.Large bitmap arrays must be defined as `static const uint16_t`
    //       so they are stored in flash memory instead of SRAM.
    //     Bad:
    //       uint16_t player[] = { ... };
    //     Good:
    //       static const uint16_t player_sprite[] = { ... };
    //   - Recommended sprite size: 32x32 pixels or smaller.
    //   - Enlarge sprites using `scale` instead of larger bitmap data.
    virtual void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h) = 0;
    struct SpriteOptions
    {
        uint8_t scale = 1;
        float angle = 0.0f;  // Clockwise rotation angle in degrees around the sprite center.
        bool flipX = false;
        bool flipY = false;

        bool transparent = false;
        Color transparentColor = MAGENTA;
    };
    virtual void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options) = 0;
    struct SpriteSheet
    {
        SpriteSheet(const uint16_t* bitmap, uint16_t spriteWidth, uint16_t spriteHeight, uint16_t columns, uint16_t rows)
            : bitmap(bitmap), spriteWidth(spriteWidth), spriteHeight(spriteHeight), columns(columns), rows(rows) {}
        const uint16_t* bitmap = nullptr;
        uint16_t spriteWidth = 0;
        uint16_t spriteHeight = 0;
        uint16_t columns = 0;
        uint16_t rows = 0;
    };
    virtual void drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options) = 0;

    // Draws an image created by this Graphics backend.
    // Passing an image created by another backend is not supported.
    // Image loading may be unsupported by some display backends.
    // drawImage() uses the transparency information stored in the Image.
    // PNG alpha transparency is not supported; PNG images are drawn as opaque.
    // To specify a different transparent color, scale, or flip direction, use drawSprite() with Image::getBitmap(), getWidth(), and getHeight().
    // Images are created by the graphics backend. Never construct or subclass Image in game code.
    // Games only ever obtain instances via loadJpeg()/loadPng(); never construct or subclass Image directly.
    // Always call Image::close() when finished with the image. Forgetting to call close() will cause a memory leak.
    class Image
    {
    public:
        // Returns a decoded reusable image.
        // Returns nullptr when:
        // - the active graphics backend does not support this image format,
        // - the image data is invalid,
        // - the output size is invalid, or
        // - memory allocation or decoding fails.
        // The returned image must be explicitly closed by calling Image::close().
        enum class Fit : uint8_t
        {
            STRETCH,
            CONTAIN,
            COVER
        };
        static Image* loadJpeg(const uint8_t* jpegData, uint32_t jpegSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit);
        static Image* loadPng(const uint8_t* pngData, uint32_t pngSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit);

        virtual const uint16_t* getBitmap() const = 0;
        virtual uint16_t getWidth() const = 0;
        virtual uint16_t getHeight() const = 0;
        virtual void close();

    protected:
        Image() = default;
        virtual ~Image() = default;
     private:
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
    };
    virtual void drawImage(const Image& image, int16_t x, int16_t y) {}

    // ## Viewport control
    //   - The default viewport position is (0, 0).
    //   - Use setViewport() to move the visible area within the graphics buffer.
    //   - This may be used for camera movement or temporary screen effects.
    //   - When moving the viewport, draw enough background around the visible area to avoid exposing undrawn regions.
    //   - [!IMPORTANT] AI WARNING FOR TEMPORARY VIEWPORT OFFSETS:
    //     - Always restore the viewport with resetViewport() when the effect ends.
    //     - Otherwise, the screen will remain offset.
    //   - setViewport() is always memory-safe.
    //     If no extra buffer margin is available, moving the viewport may expose
    //     undrawn or clipped areas.
    //     The available margin depends on the active display backend.
    virtual void setViewport(int16_t viewportX, int16_t viewportY) = 0;
    virtual void resetViewport() = 0;

    struct Camera
    {
        // Camera position in world coordinates.
        // World coordinates are translated by (-x, -y).
        int16_t x = 0;
        int16_t y = 0;
        float zoom = 1.0f;
        // Zoom center in screen coordinates.
        int16_t zoomCenterX = 0;
        int16_t zoomCenterY = 0;
    };
    virtual void setCamera(const Camera& camera) = 0;
    virtual void resetCamera() = 0;

    virtual void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h) = 0;
    virtual void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void resetClipRect() = 0;

protected:
    virtual ~Graphics() {};
};

struct GraphicsILI9341Config
{
    uint8_t spiHost = 0;
    uint32_t spiWriteFreq = 60000000;
    int8_t clkPin   = -1;
    int8_t dataPin  = -1;
    int8_t dcPin    = -1;
    int8_t csPin    = -1;
    int8_t resetPin   = -1;
    int8_t backlightPin = -1;
    uint8_t lcdRotate = 1;
};
struct GraphicsILI9341ParallelConfig
{
    uint32_t writeFreq = 10000000;
    int8_t dataPinBase = -1;
    int8_t wrPin = -1;
    int8_t rdPin = -1;
    int8_t dcPin = -1;
    int8_t csPin = -1;
    int8_t resetPin = -1;
    int8_t backlightPin = -1;
    uint8_t lcdRotate = 1;
};
struct GraphicsSSD1306Config {
    uint8_t i2cPort = 0;
    uint8_t i2cAddr = 0x3C;
    int8_t sdaPin   = -1;
    int8_t sclPin   = -1;
    int8_t resetPin   = -1;
    uint8_t oledRotate = 0;
};
using GraphicsConfig = std::variant<
    GraphicsILI9341Config,
    GraphicsILI9341ParallelConfig,
    GraphicsSSD1306Config
>;

// --- =================================================================
// # Tween
// =====================================================================
class Tween {
public:
    enum Ease : uint8_t {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,
        EASE_OUT_BACK,
        EASE_OUT_BOUNCE
    };

    // t is automatically clamped to 0.0f - 1.0f.
    static float apply(float t, Ease ease);
    static float lerp(float from, float to, float t);
    static float value(float from, float to, float t,Ease ease);
};

// --- =================================================================
// # Audio
// =====================================================================
class Audio {
public:
    // ## Sound Effect
    struct SoundStep
    {
        uint16_t startFrequency;
        uint16_t endFrequency;
        uint16_t durationMsec;
        float startVolume;
        float endVolume;
    };
    struct Sound
    {
        const SoundStep* steps;
        uint16_t stepCount;
    };
    // ### Sound Effect Presets
    struct SE
    {
        static const Sound NO_1;  // 1200hz 30msec - 1800hz 20msec (e.g., Menu confirm, Cursor move, Laser shot)
        static const Sound NO_2;  // 300hz 40msec - 150hz 30msec (e.g., Cancel, Damage taken)
        static const Sound NO_3;  // 800hz 60msec - 1200hz 60msec - 1600hz 100msec (e.g., Item pickup, Power-up)
        static const Sound NO_4;  // 1000hz 50msec - 1300hz 50msec - 1500hz 50msec - 2000hz 150msec (e.g., Large explosion, Game over)
        static const Sound NO_5;  // 600hz 30msec - 400hz 30msec - 600hz 30msec (e.g., Jump, Message advance)
        static const Sound NO_6;  // 1500hz 40msec - 1000hz 40msec (e.g., Enemy defeat, Attack blocked)
        static const Sound NO_7;  // 2500hz 20msec - 2500hz 20msec (10msec silence) (e.g., Low health alert, Time warning)
        static const Sound NO_8;  // 400hz 50msec - 600hz 50msec - 800hz 50msec - 1000hz 50msec - 1200hz 50msec (e.g., Stage start, Victory fanfare)
        static const Sound NO_9;  // 2000hz 20msec -> 1500hz 20msec - 2000hz 20msec - 1500hz 20msec (e.g., Invincibility, Star effect)
        static const Sound NO_10; // 100hz 80msec - 80hz 120msec (e.g., Earthquake, Heavy footsteps, Boss entrance)
        static const Sound NO_11; // 500hz 20msec - 1000hz 20msec - 1500hz 20msec - 2000hz 20msec - 2500hz 40msec (e.g., Charge attack, Secret discovered)
        static const Sound NO_12; // 1800hz 100msec - 1400hz 100msec - 1100hz 100msec - 900hz 200msec (e.g., Stage failed, Sad game over jingle)
        static const Sound NO_13; // 200hz 80msec - 0hz 50msec - 200hz 120msec (e.g., Error, Invalid operation)
    };
    // ### Sound playback
    //   - Playback runs on a separate core and returns immediately.
    //   - Valid gain range: 0.1-1.0.
    //     Use different gain values to match the intended effect.
    //   - Sound and SoundStep data passed to playSE() must remain valid until playback finishes.
    //     Define custom sound data as `static const`. Do not pass pointers to local variables.
    // [!IMPORTANT] AI WARNING:
    //   - The `sound` parameter requires a pointer (`const Sound*`).
    //   - Always pass the address of a preset using the `&` operator.
    //   - Bad:  audio.playSE(Audio::SE::NO_1, 1.0f);
    //   - Good: audio.playSE(&Audio::SE::NO_1, 1.0f);
    virtual void playSE(const Sound* sound, float gain) = 0;

    // ## Music
    struct ToneNote
    {
        static constexpr uint16_t REST = 0; // Silence.
        static constexpr uint16_t C2 = 65,  CS2 = 69,  D2 = 73,  DS2 = 78,  E2 = 82,  F2 = 87,  FS2 = 92,  G2 = 98,  GS2 = 104, A2 = 110, AS2 = 117, B2 = 123;
        static constexpr uint16_t C3 = 131, CS3 = 139, D3 = 147, DS3 = 156, E3 = 165, F3 = 175, FS3 = 185, G3 = 196, GS3 = 208, A3 = 220, AS3 = 233, B3 = 247;
        static constexpr uint16_t C4 = 262, CS4 = 277, D4 = 294, DS4 = 311, E4 = 330, F4 = 349, FS4 = 370, G4 = 392, GS4 = 415, A4 = 440, AS4 = 466, B4 = 494;
        static constexpr uint16_t C5 = 523, CS5 = 554, D5 = 587, DS5 = 622, E5 = 659, F5 = 698, FS5 = 740, G5 = 784, GS5 = 831, A5 = 880, AS5 = 932, B5 = 988;
        static constexpr uint16_t C6 = 1047,CS6 = 1109,D6 = 1175,DS6 = 1245,E6 = 1319,F6 = 1397,FS6 = 1480,G6 = 1568,GS6 = 1661,A6 = 1760,AS6 = 1865,B6 = 1976;
        uint16_t frequency;

        enum Duration : uint8_t {
        	// Note durations:
        	//   S  = sixteenth note, SD = dotted sixteenth note, E  = eighth note,    ED = dotted eighth note,
        	//   Q  = quarter note,   QD = dotted quarter note,   H  = half note,      HD = dotted half note, W  = whole note
            S  = 2,
            SD = 3,
            E  = 4,
            ED = 6,
            Q  = 8,
            QD = 12,
            H  = 16,
            HD = 24,
            W  = 32
        };
        Duration duration;
        bool tie;  // If true, continue into the next note without retriggering the tone.

        constexpr ToneNote() : frequency(REST), duration(Q), tie(false) {}
        constexpr ToneNote(uint16_t _frequency, Duration _duration, bool _tie = false) : frequency(_frequency), duration(_duration), tie(_tie) {}
    };
    struct Music
    {
        const ToneNote* notes;
        uint16_t bpm;
        uint16_t noteCount; // element count
        uint8_t playCount;  // 0 = infinite, 1 = play once
        float gain;         // Valid range: 0.1-1.0.
    };
    // ### Music playback
    //   - Music and ToneNote data passed to playMusic() must remain valid until playback stops.
    //   - Define custom music data as `static const`. Do not pass pointers to local variables.
    virtual void playMusic(const Music* music) = 0;

    // ## Embedded MIDI music
    //   - Supports lightweight playback of embedded Standard MIDI File data.
    //   - MIDI data must remain valid until playback stops.
    //   - Define MIDI byte arrays and Midi objects as `static const`.
    struct Midi
    {
        const uint8_t* data;
        uint32_t size;
        uint8_t playCount; // 0 = infinite, 1 = play once
        float gain;        // Valid range: 0.0-1.0.
    };
    virtual void playMidi(const Midi* midi) = 0;

    // Stops either ToneNote music or MIDI music.
    virtual void stopMusic() = 0;

protected:
    virtual ~Audio() {};
};

struct AudioPWMConfig
{
    int8_t pwmPin = -1;
};
struct AudioI2SConfig
{
    int8_t bclkPin = -1;
    int8_t wsPin = -1;
    int8_t dataPin = -1;
};
struct AudioStubConfig
{
};
using AudioConfig = std::variant<
    AudioPWMConfig,
    AudioI2SConfig,
    AudioStubConfig
>;

// --- =================================================================
// # Storage
// PRUZEA mini provides three storage APIs:
// 1. SaveData
//    Key-value persistent storage for ordinary app save data.
//    No file paths or file-system operations are required.
//    Use it for: high scores, unlocked content, app progress, app settings
// 2. UserFile
//    Writable per-load file storage for custom data formats.
//    The app supplies only its appId and a relative file name.
//    The system determines the actual storage path.
//    Apps must not construct save-data paths manually.
// 3. File
//    Read-only access to packaged app resources.
//
//    File cannot be used to create, write, or modify save data.
// =====================================================================
class Storage {
public:
    // Checks whether the storage is currently available.
    // The result may be cached briefly to avoid frequent hardware access.
    virtual bool isAvailable() const = 0;

    // ## Read-only resource file
    // File provides read-only access to packaged app assets.
    // It is not a writable save-data API.
    // Files opened with openRead() must be explicitly closed by the caller.
    // Resource files should normally be opened and read only inside App::init().
    class File {
    public:
        virtual bool isOpen() const = 0;
        virtual uint32_t size() const = 0;
        virtual uint32_t read(void* buffer, uint32_t bytes) = 0;
        virtual void close() = 0;
    protected:
       virtual ~File() {}
    };

    // ## UserFile
    // ### Read UserFile
    //   - The system automatically opens and closes the file.
    //   - The reader callback receives one null-terminated line at a time.
    //   - Return true from the callback to continue reading the next line.
    //   - Return false from the callback to stop reading early.
    //   - Returning false is a normal early termination and does not mean that readUserFile() failed.
    //   - readUserFile() returns false when the file could not be opened or read.
    //   - The `arg` parameter can be used to pass `this` to a non-capturing lambda or static callback.
    using UserFileLineReaderHandler = bool(*)(const char* line, void* arg);
    virtual bool readUserFile(const char* appId, const char* fileName, UserFileLineReaderHandler reader, void* arg) = 0;
    // ### Write UserFile
    //   - The callback uses std::string to prevent buffer overflows.
    //   - Inside this callback, ALWAYS use direct assignment (e.g., line = "value";)
    //      or std::string::assign(). Never use string concatenation ('+') to prevent dynamic memory allocation.
    //   - A single write operation may produce at most 256 lines.
    //   - The callback is called once for each output line.
    //     - If the callback continues beyond this limit, writing fails and writeUserFile() returns false.
    //     - The callback must return false after the final line.
    //   - [!IMPORTANT] CRITICAL AI RULE:
    //     - Never use string concatenation ('+').
    //     - Use direct assignment or std::string::assign().
    //     - Use snprintf() with a local buffer instead of std::to_string().
     static constexpr uint16_t USER_FILE_MAX_LINES = 256;
    using UserFileLineWriterHandler = bool(*)(std::string& line, void* arg);
    virtual bool writeUserFile(const char* appId, const char* fileName, UserFileLineWriterHandler writer, void* arg) = 0;
    // ### Lightweight UserFile writing
    //   - For writes of 200 bytes or less, use this function instead of UserFileLineWriterHandler.
    //   - Maximum size: 200 bytes.
    //   - Larger writes must use UserFileLineWriterHandler.
    //   - Newline characters contained in data are preserved.
    //   - One newline character is automatically appended after the provided data.
    //   - If append is false, the existing file is replaced.
    //   - If append is true, the data is appended to the existing file.
    virtual bool writeUserFile(const char* appId, const char* fileName, const char* data, bool append = false) = 0;

    // ## Read-only resource files
    // Opens a packaged read-only resource.
    //   - Do not use openRead() for save data or user settings.
    //   - Use UserFile for writable persistent data.
    //   - Always call File::close() when finished.
    virtual File* openRead(const char* path) = 0;

    virtual bool directoryExists(const char* path) = 0;
    virtual bool fileExists(const char* path) = 0;

protected:
    virtual ~Storage() {};
private:
    friend class SaveData;
    virtual bool writeSaveDataInternal(const char* appId, const char* fileName, UserFileLineWriterHandler writer, void* arg ) = 0;
};

struct StorageEEPROMConfig
{
    uint16_t magic = 0x504d;
    uint8_t version = 1;
    uint16_t eepromSize = 4096;
};
struct StorageSDConfig
{
    uint8_t spiHost = 0;
    int8_t misoPin = -1;
    int8_t sckPin = -1;
    int8_t mosiPin = -1;
    int8_t csPin = -1;
    uint32_t baudRate = 12 * 1000 * 1000;
};
struct StorageStubConfig
{
};
using StorageConfig = std::variant<
    StorageEEPROMConfig,
    StorageSDConfig,
    StorageStubConfig
>;


// --- =================================================================
// # SaveData: Storage helper
//   SaveData provides small key-value persistent storage.
//   It handles file formatting and UserFile access internally.
//   Use SaveData for ordinary save data instead of implementing a custom key-value file format.
// =====================================================================
class SaveData {
public:
    SaveData();
    ~SaveData();

    void clear();
    bool load(Storage& storage, const char* gameId, const char* fileName);
    bool save(Storage& storage, const char* gameId, const char* fileName);

    bool contains(const char* key) const;
    bool remove(const char* key);
    bool getString(const char* key, char* outValue, size_t outSize, const char* defaultValue = "") const;
    int32_t getInt32(const char* key, int32_t defaultValue = 0) const;
    uint32_t getUInt32(const char* key, uint32_t defaultValue = 0) const;
    bool getBool(const char* key, bool defaultValue = false) const;
    bool setString(const char* key, const char* value);
    bool setInt32(const char* key, int32_t value);
    bool setUInt32(const char* key, uint32_t value);
    bool setBool(const char* key, bool value);
    bool isDirty() const;

private:
    void* impl;
    SaveData(const SaveData&) = delete;
    SaveData& operator=(const SaveData&) = delete;
    SaveData(SaveData&&) = delete;
    SaveData& operator=(SaveData&&) = delete;
};


// --- =================================================================
// # App
//   Derive your app class from this class to run it on PRUZEA mini.
//   - Target update rate: 30 FPS
//   - deltaSec is provided every frame.
//   [!IMPORTANT] Never call init(), update(), draw(), or terminate() directly.
//   The PRUZEA mini runtime calls them automatically.
//   [!IMPORTANT] Override all pure virtual functions.
// =====================================================================
class App {
private:
    // ## Typical game mode example:
    // enum InternalMode {
    //     MODE_TITLE,
    //     MODE_PLAYING,
    //     MODE_PAUSED,
    //     MODE_GAME_OVER
    // };
    // Real-time games are encouraged to implement MODE_PAUSED and toggle it with the START button.
    // When entering the pause state, you MUST explicitly call `audio.stopMusic()`.
    // When resuming the game to `MODE_RUNNING` or `MODE_PLAYING`, you MUST call `audio.playMusic()` exactly once to restart the BGM.

protected:
    // ## Redraw flag
    // `dirty` indicates whether the screen needs to be redrawn.
    // - The initial value is true so the first frame is always drawn.
    // - Set dirty = true in update() whenever any visible state changes.
    // - Examples:
    //     player movement
    //     score changes
    //     animation changes
    //     state transitions
    //     viewport or camera changes
    // - Continuously animated action games may set dirty every update while running.
    bool dirty = true;

    // ## Called once before the app starts.
    //   - Load save data, initialize variables, and prepare resources.
    //   - Normally, rendering should be performed in draw().
    //   - After onInit() returns, the system calls draw() with requestFullRedraw = true.
    // onInit() has no call-order guard because it is called on every replay.
    // Therefore, it keeps its direct name.
    virtual void onInit(Storage& storage) = 0;

    // ## Frame-by-frame app state updating
    // Called once per frame to update app logic, physics, and animations.
    //   - Input has already been updated before onUpdate() is called.
    //   - [!IMPORTANT] Request sound playback only from onUpdate().
    // deltaSec:
    //   Elapsed time since the previous App::update(), in seconds.
    //   The system guarantees a value in the range 0.0f to 0.1f.
    //   The first update after init(), resume, or a system-side pause receives 0.0f.
    virtual void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) = 0;

    // ## Frame-by-frame app screen drawing
    // Called once per frame to draw the app screen.
     // - If requestFullRedraw is true, the app must draw regardless of dirty.
    // - If requestFullRedraw is false and dirty is false, return false without drawing.
    // - Return true only when drawing was performed.
    // - After drawing, set dirty = false before returning true.
    // - The system uses the return value to decide whether the graphics buffer must be transferred to the physical display.
    // - [!IMPORTANT] CRITICAL AI RULE FOR DIRTY RENDERING:
    //   - Check `dirty` at the very beginning of onDraw().
    //   - Perform ALL drawing operations FIRST.
    //   - Set `dirty = false` ONLY AT THE VERY END of the function, right before `return true;`.
    virtual bool onDraw(Graphics& graphics, bool requestFullRedraw) = 0;

    // ## Termination
    //   - Called by the runtime when the app is about to terminate.
    //   - Save persistent data or perform cleanup here if needed.
    //   - [!IMPORTANT] After onTerminate() is called, onUpdate() and onDraw() will never be called again.
    virtual void onTerminate(Storage& storage) = 0;

    virtual ~App() {};

public:
    // ## System entry points
    // [!IMPORTANT]
    // The following functions are called by the PRUZEA mini runtime.
    // Do not modify, hide, or redefine them.
    // Implement the corresponding on*() functions instead.
    void init(Storage& storage)
    {
        dirty = true;
        onInit(storage);
    }
    void update(Input& input, Audio& audio, Storage& storage, float deltaSec)
    {
        onUpdate(input, audio, storage, deltaSec);
    }
    bool draw(Graphics& graphics, bool requestFullRedraw)
    {
        return onDraw(graphics, requestFullRedraw);
    }
    void terminate(Storage& storage)
    {
        onTerminate(storage);
    }

    // Returns the app ID.
    // The ID may contain only lowercase letters (a-z), digits (0-9), underscores (_), and hyphens (-), up to 32 characters.
    // Used to identify the application's storage data.
    virtual const char* getId() const = 0;
};


void start(const GraphicsConfig& graphicsConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig, const StorageConfig& storageConfig, App& app);


} // namespace PRUZEA mini

