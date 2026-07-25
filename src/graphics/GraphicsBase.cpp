#include "GraphicsBase.h"
#include <algorithm>

using namespace PRUZEAmini;

uint16_t GraphicsBase::getTextHeight(const char* text, Font font)
{
    uint16_t size = 0;
    switch (font)
    {
        case Font::SIZE_10:  return 10;
        case Font::SIZE_13:  return 13;
        case Font::SIZE_18:  return 18;
        case Font::SIZE_22B: return 22;
        case Font::SIZE_25:  return 25;
        case Font::SIZE_25B: return 25;
        case Font::SIZE_32:  return 32;
        case Font::SIZE_32B: return 32;
        case Font::SIZE_42:  return 42;
        case Font::SIZE_42B: return 42;
        default: return 0;
     }
}

void GraphicsBase::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t t, Graphics::Color color)
{
    if (w <= 0 || h <= 0 || t <= 0) return;

    const int thickness = std::min(t, std::min(w, h));
    fillRect(x, y, w, thickness, color);
    fillRect(x, y + h - thickness, w, thickness, color);
    fillRect(x, y, thickness, h, color);
    fillRect(x + w - thickness, y, thickness, h, color);
}

void GraphicsBase::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t t, Graphics::Color color)
{
    if (w <= 0 || h <= 0 || t <= 0) return;

    const int thickness = std::min(t, std::min(w, h));
    for (int i = 0; i < thickness; ++i)
    {
        if (w - i*2 <= 0) break;

        drawRoundRect(
            static_cast<int16_t>(x + i),
            static_cast<int16_t>(y + i),
            static_cast<uint16_t>(w - i * 2),
            static_cast<uint16_t>(h - i * 2),
            static_cast<uint16_t>(std::max(0, static_cast<int>(r) - i)),
            color
        );
    }
}

void GraphicsBase::drawString(const char* str, int16_t cx, int16_t cy, Color color, Font font, HorizontalAlign ha, VerticalAlign va)
{
    if (str == nullptr) return;
    int x = cx;
    int y = cy;
    const int w = getTextWidth(str, font);
    const int h = getTextHeight(str, font);

    switch (ha)
    {
    case HorizontalAlign::CENTER:
        x -= w / 2;
        break;
    case HorizontalAlign::RIGHT:
        x -= w;
        break;
    default:
        break;
    }
    switch (va)
    {
    case VerticalAlign::MIDDLE:
        y -= h / 2;
        break;
    case VerticalAlign::BOTTOM:
        y -= h;
        break;
    default:
        break;
    }

    drawString(str, x, y, color, font);
}

void GraphicsBase::setViewport(int16_t x, int16_t y)
{
    if (viewportX == x && viewportY == y) return;
    viewportX = x;
    viewportY = y;
    screenDirty = true;
}

void GraphicsBase::resetViewport()
{
    setViewport(0, 0);
}
