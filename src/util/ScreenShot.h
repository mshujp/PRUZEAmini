#pragma once

#include "../storage/StorageBase.h"

namespace PRUZEAmini {

class GraphicsILI9341;
class GraphicsSSD1306;
class StorageSD;

class ScreenShot
{
public:
    static bool save(GraphicsILI9341& graphics, StorageSD& storage, const char* fileName = nullptr);
    static bool save(GraphicsSSD1306& graphics, StorageSD& storage, const char* fileName = nullptr);
};

} // namespace PRUZEAmini
