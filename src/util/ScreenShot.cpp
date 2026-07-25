#include "ScreenShot.h"
#include "../graphics/GraphicsBase.h"
#include "../graphics/GraphicsILI9341.h"
#include "../graphics/GraphicsSSD1306.h"
#include "../storage/StorageSD.h"

#include <cstdio>
#include <memory>

namespace PRUZEAmini {
namespace {

constexpr const char* SCREENSHOT_APP_ID = "screenshots";

struct ScreenShotContext
{
    GraphicsBase& graphics;
    uint16_t width;
    uint16_t height;
};

bool write16(StorageBaseFile& file, uint16_t value)
{
    const uint8_t bytes[] =
    {
        static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8)
    };
    return file.write(bytes, sizeof(bytes)) == sizeof(bytes);
}

bool write32(StorageBaseFile& file, uint32_t value)
{
    const uint8_t bytes[] =
    {
        static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 24)
    };
    return file.write(bytes, sizeof(bytes)) == sizeof(bytes);
}

bool writeBitmap(StorageBaseFile& file, void* arg)
{
    auto* context = static_cast<ScreenShotContext*>(arg);
    GraphicsBase& graphics = context->graphics;
    const uint16_t width = context->width;
    const uint16_t height = context->height;
    if (width == 0 || height == 0) return false;

    const uint32_t rowBytes = (static_cast<uint32_t>(width) * 3u + 3u) & ~3u;
    const uint32_t pixelBytes = rowBytes * height;
    std::unique_ptr<uint16_t[]> pixels(new (std::nothrow) uint16_t[width]);
    std::unique_ptr<uint8_t[]> row(new (std::nothrow) uint8_t[rowBytes]);
    if (!pixels || !row) return false;

    bool ok = file.write("BM", 2) == 2;
    ok = ok && write32(file, 54u + pixelBytes);
    ok = ok && write16(file, 0) && write16(file, 0) && write32(file, 54);
    ok = ok && write32(file, 40) && write32(file, width) && write32(file, height);
    ok = ok && write16(file, 1) && write16(file, 24) && write32(file, 0);
    ok = ok && write32(file, pixelBytes) && write32(file, 2835) && write32(file, 2835);
    ok = ok && write32(file, 0) && write32(file, 0);

    for (int32_t y = static_cast<int32_t>(height) - 1; ok && y >= 0; --y)
    {
        if (!graphics.readScreenLine(static_cast<uint16_t>(y), pixels.get(), width)) return false;

        uint32_t cursor = 0;
        for (uint16_t x = 0; x < width; ++x)
        {
            const uint16_t color = pixels[x];
            row[cursor++] = static_cast<uint8_t>((color & 0x001f) << 3);
            row[cursor++] = static_cast<uint8_t>((color & 0x07e0) >> 3);
            row[cursor++] = static_cast<uint8_t>((color & 0xf800) >> 8);
        }
        while (cursor < rowBytes) row[cursor++] = 0;
        ok = file.write(row.get(), rowBytes) == rowBytes;
    }
    return ok;
}

} // namespace

bool saveScreenShot(GraphicsBase& graphics, uint16_t width, uint16_t height,
                    StorageSD& storage, const char* fileName)
{
    if (!storage.isAvailable()) return false;

    char generatedName[16];
    if (fileName == nullptr)
    {
        bool found = false;
        for (uint16_t index = 0; index < 1000; ++index)
        {
            snprintf(generatedName, sizeof(generatedName), "SCREEN%03u.BMP", index);
            if (!storage.userFileExists(SCREENSHOT_APP_ID, generatedName))
            {
                found = true;
                break;
            }
        }
        if (!found) return false;
        fileName = generatedName;
    }

    ScreenShotContext context{graphics, width, height};
    return storage.writeBinaryFile(
        SCREENSHOT_APP_ID, fileName, &writeBitmap, &context);
}

bool ScreenShot::save(GraphicsILI9341& graphics, StorageSD& storage, const char* fileName)
{
    return saveScreenShot(graphics, Display::ILI9341_SCREEN_W, Display::ILI9341_SCREEN_H, storage, fileName);
}

bool ScreenShot::save(GraphicsSSD1306& graphics, StorageSD& storage, const char* fileName)
{
    return saveScreenShot(graphics, Display::SSD1306_SCREEN_W, Display::SSD1306_SCREEN_H, storage, fileName);
}

} // namespace PRUZEAmini
