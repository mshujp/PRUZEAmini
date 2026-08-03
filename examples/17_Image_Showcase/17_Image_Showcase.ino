/*
===============================================================================
 PRUZEAmini Example
 17_Image_Showcase
===============================================================================

This example demonstrates embedded JPEG and PNG image handling.

Features:
- Decodes an embedded JPEG image and reuses it as a background.
- Decodes an embedded PNG image and draws it as an RGB565 sprite.
- Uses magenta color-key transparency instead of PNG alpha transparency.
- Demonstrates SpriteOptions::angle with continuous rotation.
- Keeps image binary data in ImageAssets.h to keep this sketch readable.
- Closes decoded Image objects in onTerminate().

Controls:
- D-PAD : Move the character
- A     : Toggle color-key transparency
- X     : Rotate while held
- B     : Reset position, angle, and transparency

Memory notes:
- Decoded images require contiguous RGB565 memory.
- JPEG background: 128 x 96 x 2 bytes
- PNG character :  64 x 64 x 2 bytes
- Images are decoded only once and then reused.
- The small image sizes are intended to remain practical on RP2040.

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Change lcdRotate if necessary to match the display orientation.
===============================================================================
*/

#include <PRUZEAmini.h>
#include "ImageAssets.h"

using namespace PRUZEAmini;

// -----------------------------------------------------------------------------
// Hardware configuration
// Set the pins for the board and modules being used.
// Unused or unconnected pins may remain -1.
// -----------------------------------------------------------------------------
GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost = 0,
    .spiWriteFreq = 62500000,
    .clkPin = -1,
    .dataPin = -1,
    .dcPin = -1,
    .csPin = -1,
    .resetPin = -1,
    .backlightPin = -1,
    .lcdRotate = 1
};

InputConfig inputConfig = InputGpioButtonsConfig{
    .gpioButtonPins = {
        .UP = -1,
        .DOWN = -1,
        .LEFT = -1,
        .RIGHT = -1,
        .A = -1,
        .B = -1,
        .X = -1,
        .Y = -1,
        .L = -1,
        .R = -1,
        .L2 = -1,
        .R2 = -1,
        .L3 = -1,
        .R3 = -1,
        .START = -1,
        .SELECT = -1,
        .VOL_UP = -1,
        .VOL_DOWN = -1,
        .MUTE = -1
    }
};

AudioConfig audioConfig = AudioStubConfig{};
StorageConfig storageConfig = StorageStubConfig{};

class ImageShowcaseApp : public App
{
public:
    const char* getId() const override
    {
        return "image_showcase";
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;

        closeImages();
        characterX = CHARACTER_START_X;
        characterY = CHARACTER_START_Y;
        characterAngle = 0.0f;
        transparencyEnabled = true;

        backgroundImage = Graphics::Image::loadJpeg(
            sampleBackgroundJpeg,
            sampleBackgroundJpegSize,
            BACKGROUND_W,
            BACKGROUND_H,
            Graphics::Image::Fit::STRETCH);

        characterImage = Graphics::Image::loadPng(
            sampleCharacterPng,
            sampleCharacterPngSize,
            CHARACTER_W,
            CHARACTER_H,
            Graphics::Image::Fit::STRETCH);

        loadSucceeded = backgroundImage != nullptr && characterImage != nullptr;
        if (!loadSucceeded) closeImages();
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)storage;

        bool changed = false;
        float moveX = 0.0f;
        float moveY = 0.0f;

        if (input.pressed(Input::LEFT))  moveX -= MOVE_SPEED * deltaSec;
        if (input.pressed(Input::RIGHT)) moveX += MOVE_SPEED * deltaSec;
        if (input.pressed(Input::UP))    moveY -= MOVE_SPEED * deltaSec;
        if (input.pressed(Input::DOWN))  moveY += MOVE_SPEED * deltaSec;

        if (moveX != 0.0f || moveY != 0.0f)
        {
            characterX = Math::clamp(characterX + moveX, CHARACTER_MIN_X, CHARACTER_MAX_X);
            characterY = Math::clamp(characterY + moveY, CHARACTER_MIN_Y, CHARACTER_MAX_Y);
            changed = true;
        }

        // Hold X to rotate continuously.
        if (input.pressed(Input::X))
        {
            characterAngle += ROTATION_SPEED * deltaSec;
            while (characterAngle >= 360.0f)
            {
                characterAngle -= 360.0f;
            }
            changed = true;
        }

        if (input.justPressed(Input::A))
        {
            transparencyEnabled = !transparencyEnabled;
            audio.playSE(&Audio::SE::NO_1, 0.5f);
            changed = true;
        }

        if (input.justPressed(Input::B))
        {
            characterX = CHARACTER_START_X;
            characterY = CHARACTER_START_Y;
            characterAngle = 0.0f;
            transparencyEnabled = true;
            audio.playSE(&Audio::SE::NO_2, 0.5f);
            changed = true;
        }

        if (changed)
        {
            dirty = true;
        }
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }

        drawScreen(graphics);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
        closeImages();
    }

private:
    static constexpr int16_t SCREEN_W = 320;
    static constexpr int16_t SCREEN_H = 240;

    static constexpr uint16_t BACKGROUND_W = 128;
    static constexpr uint16_t BACKGROUND_H = 96;
    static constexpr uint16_t CHARACTER_W = 64;
    static constexpr uint16_t CHARACTER_H = 64;

    static constexpr float MOVE_SPEED = 110.0f;
    static constexpr float ROTATION_SPEED = 180.0f; // degrees per second

    static constexpr float CHARACTER_START_X = 128.0f;
    static constexpr float CHARACTER_START_Y = 92.0f;
    static constexpr float CHARACTER_MIN_X = 0.0f;
    static constexpr float CHARACTER_MAX_X = SCREEN_W - CHARACTER_W;
    static constexpr float CHARACTER_MIN_Y = 34.0f;
    static constexpr float CHARACTER_MAX_Y = 224.0f - CHARACTER_H;

    Graphics::Image* backgroundImage = nullptr;
    Graphics::Image* characterImage = nullptr;

    float characterX = CHARACTER_START_X;
    float characterY = CHARACTER_START_Y;
    float characterAngle = 0.0f;
    bool transparencyEnabled = true;
    bool loadSucceeded = false;

    void closeImages()
    {
        if (characterImage != nullptr)
        {
            characterImage->close();
            characterImage = nullptr;
        }

        if (backgroundImage != nullptr)
        {
            backgroundImage->close();
            backgroundImage = nullptr;
        }
    }

    void drawScreen(Graphics& graphics)
    {
        graphics.fillScreen(Graphics::rgb565(12, 24, 38));
        graphics.fillRect(0, 34, SCREEN_W, 190, Graphics::rgb565(20, 44, 54));

        for (int16_t y = 34; y < 224; y += 24)
        {
            for (int16_t x = 0; x < SCREEN_W; x += 24)
            {
                if (((x / 24) + (y / 24)) & 1)
                {
                    graphics.fillRect(x, y, 24, 24, Graphics::rgb565(24, 52, 62));
                }
            }
        }

        graphics.fillRect(0, 0, SCREEN_W, 34, Graphics::rgb565(5, 15, 26));
        graphics.drawString(
            "IMAGE SHOWCASE",
            SCREEN_W / 2,
            8,
            Graphics::CYAN,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::TOP);

        if (!loadSucceeded)
        {
            graphics.drawString(
                "IMAGE LOAD FAILED",
                SCREEN_W / 2,
                112,
                Graphics::RED,
                Graphics::SIZE_18,
                Graphics::HorizontalAlign::CENTER,
                Graphics::VerticalAlign::MIDDLE);
            return;
        }

        const int16_t backgroundX = (SCREEN_W - BACKGROUND_W) / 2;
        const int16_t backgroundY = 72;

        graphics.drawRect(backgroundX - 2, backgroundY - 2, BACKGROUND_W + 4, BACKGROUND_H + 4, Graphics::WHITE);
        graphics.drawImage(*backgroundImage, backgroundX, backgroundY);

        Graphics::SpriteOptions options;
        options.scale = 1;
        options.angle = Math::degToRad(characterAngle);
        options.flipX = false;
        options.flipY = false;
        options.transparent = transparencyEnabled;
        options.transparentColor = Graphics::MAGENTA;

        graphics.drawSprite(
            characterImage->getBitmap(),
            static_cast<int16_t>(characterX),
            static_cast<int16_t>(characterY),
            characterImage->getWidth(),
            characterImage->getHeight(),
            options);

        graphics.drawString(
            transparencyEnabled ? "A: KEY ON" : "A: KEY OFF",
            8,
            205,
            transparencyEnabled ? Graphics::GREEN : Graphics::MAGENTA,
            Graphics::SIZE_13);

        graphics.drawString(
            "X:HOLD ROTATE  B:RESET",
            312,
            205,
            Graphics::WHITE,
            Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::TOP);
    }
};

ImageShowcaseApp app;

void setup()
{
    PRUZEAmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
