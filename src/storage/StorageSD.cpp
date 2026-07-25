#include "StorageSD.h"

#include <cstring>

namespace PRUZEAmini {

StorageSDFile::~StorageSDFile()
{
    close();
}

bool StorageSDFile::openRead(const char* path)
{
    close();
    file = SD.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    mode = OpenMode::READ;
    fileSize = static_cast<uint32_t>(file.size());
    return true;
}

bool StorageSDFile::openWrite(const char* path, bool append)
{
    close();
    if (!append && SD.exists(path)) SD.remove(path);

#if defined(ARDUINO_ARCH_ESP32)
    file = SD.open(path, append ? FILE_APPEND : FILE_WRITE);
#else
    file = SD.open(path, FILE_WRITE);
#endif
    if (!file) return false;

    mode = OpenMode::WRITE;
    fileSize = append ? static_cast<uint32_t>(file.size()) : 0;
    return true;
}

bool StorageSDFile::isOpen() const
{
    return mode != OpenMode::CLOSED && static_cast<bool>(file);
}

uint32_t StorageSDFile::size() const
{
    return isOpen() ? fileSize : 0;
}

uint32_t StorageSDFile::read(void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::READ || buffer == nullptr) return 0;
    return static_cast<uint32_t>(file.read(static_cast<uint8_t*>(buffer), bytes));
}

uint32_t StorageSDFile::write(const void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::WRITE || (buffer == nullptr && bytes > 0)) return 0;

    const uint32_t written = static_cast<uint32_t>(
        file.write(static_cast<const uint8_t*>(buffer), bytes));
    fileSize += written;
    return written;
}

void StorageSDFile::close()
{
    const bool wasOpen = mode != OpenMode::CLOSED;
    if (file) file.close();
    mode = OpenMode::CLOSED;
    fileSize = 0;
    if (wasOpen && owner != nullptr) owner->unmountCard();
}

bool StorageSDFile::closeWrite()
{
    if (mode != OpenMode::WRITE) return false;

    file.flush();
    file.close();
    mode = OpenMode::CLOSED;
    fileSize = 0;
    if (owner != nullptr) owner->unmountCard();
    return true;
}

StorageSD::StorageSD(const StorageSDConfig& value)
    : config(value), fileSlot(this)
{
}

StorageSD::~StorageSD()
{
    end();
}

bool StorageSD::begin()
{
    if (spiReady) return isAvailable();
    if (config.csPin < 0) return false;

#if defined(ARDUINO_ARCH_RP2040)
    SPIClassRP2040* rpSpi = config.spiHost == 1 ? &SPI1 : &SPI;
    if (config.misoPin >= 0 && !rpSpi->setRX(config.misoPin)) return false;
    if (config.mosiPin >= 0 && !rpSpi->setTX(config.mosiPin)) return false;
    if (config.sckPin >= 0 && !rpSpi->setSCK(config.sckPin)) return false;
    spi = rpSpi;
    spi->begin();
#elif defined(ARDUINO_ARCH_ESP32)
#if defined(VSPI) && defined(HSPI)
    const uint8_t bus = config.spiHost == 1 ? HSPI : VSPI;
#elif defined(FSPI)
    const uint8_t bus = FSPI;
#else
    const uint8_t bus = SPI2_HOST;
#endif
    spi = new SPIClass(bus);
    if (spi == nullptr) return false;

    ownsSpi = true;
    spi->begin(config.sckPin, config.misoPin, config.mosiPin, config.csPin);
#else
    spi = &SPI;
    spi->begin();
#endif

    spiReady = true;
    availabilityCached = false;
    return isAvailable();
}

bool StorageSD::mountCard()
{
    if (sdMounted) return true;
    if (!spiReady || spi == nullptr) return false;

#if defined(ARDUINO_ARCH_RP2040)
    sdMounted = SD.begin(config.csPin, config.baudRate, *spi);
#else
    sdMounted = SD.begin(config.csPin, *spi, config.baudRate);
#endif
    updateAvailabilityCache(sdMounted);
    return sdMounted;
}

void StorageSD::unmountCard()
{
    if (!sdMounted) return;
    SD.end();
    sdMounted = false;
}

void StorageSD::updateAvailabilityCache(bool available) const
{
    cachedAvailable = available;
    availabilityCached = true;
    lastAvailabilityCheckMsec = millis();
}

bool StorageSD::isAvailable() const
{
    const uint32_t now = millis();
    if (availabilityCached && static_cast<uint32_t>(now - lastAvailabilityCheckMsec) < AVAILABILITY_CACHE_MSEC)
    {
        return cachedAvailable;
    }

    StorageSD* self = const_cast<StorageSD*>(this);
    const bool wasMounted = self->sdMounted;
    const bool available = self->mountCard();
    if (!wasMounted && available) self->unmountCard();
    self->updateAvailabilityCache(available);
    return available;
}

void StorageSD::end()
{
    fileSlot.close();
    unmountCard();
    if (spi != nullptr) spi->end();
    if (ownsSpi) delete spi;

    spi = nullptr;
    ownsSpi = false;
    spiReady = false;
    availabilityCached = false;
    cachedAvailable = false;
    lastAvailabilityCheckMsec = 0;
}

bool StorageSD::isValidFileName(const char* fileName)
{
    if (fileName == nullptr || fileName[0] == '\0') return false;

    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(fileName); *p; ++p)
    {
        if (*p < 0x20 || *p == 0x7f || *p == '/' || *p == '\\' || *p == ':') return false;
    }
    return true;
}

bool StorageSD::makeDataPath(char* output, size_t outputSize,
                             const char* appId, const char* fileName) const
{
    if (output == nullptr || !isValidAppId(appId) || !isValidFileName(fileName)) return false;

    const int count = snprintf(output, outputSize, "%s/%s/%s", ROOT_DIR, appId, fileName);
    return count >= 0 && static_cast<size_t>(count) < outputSize;
}

bool StorageSD::ensureDirectory(const char* path)
{
    if (!sdMounted || path == nullptr || path[0] != '/') return false;

    char partial[PATH_MAX_LENGTH] = {};
    const size_t length = strlen(path);
    if (length >= sizeof(partial)) return false;

    for (size_t i = 1; i <= length; ++i)
    {
        if (path[i] == '/' || path[i] == '\0')
        {
            memcpy(partial, path, i);
            partial[i] = '\0';
            if (!SD.exists(partial) && !SD.mkdir(partial)) return false;
        }
    }
    return true;
}

Storage::File* StorageSD::openRead(const char* path)
{
    if (path == nullptr) return nullptr;
    fileSlot.close();
    if (!mountCard()) return nullptr;
    if (fileSlot.openRead(path)) return &fileSlot;
    unmountCard();
    return nullptr;
}

Storage::File* StorageSD::openRead(const char* appId, const char* fileName)
{
    char path[PATH_MAX_LENGTH];
    if (!makeDataPath(path, sizeof(path), appId, fileName)) return nullptr;
    return openRead(path);
}

StorageBaseFile* StorageSD::openWrite(
    const char* appId,
    const char* fileName,
    bool append)
{
    if (!isValidAppId(appId) || !isValidFileName(fileName)) return nullptr;

    fileSlot.close();
    if (!mountCard()) return nullptr;

    char directory[PATH_MAX_LENGTH];
    const int count = snprintf(directory, sizeof(directory), "%s/%s", ROOT_DIR, appId);
    if (count < 0 || static_cast<size_t>(count) >= sizeof(directory) ||
        !ensureDirectory(directory))
    {
        unmountCard();
        return nullptr;
    }

    char path[PATH_MAX_LENGTH];
    if (!makeDataPath(path, sizeof(path), appId, fileName))
    {
        unmountCard();
        return nullptr;
    }

    if (fileSlot.openWrite(path, append)) return &fileSlot;
    unmountCard();
    return nullptr;
}

bool StorageSD::directoryExists(const char* path)
{
    if (path == nullptr) return false;
    fileSlot.close();
    if (!mountCard()) return false;

    ::File entry = SD.open(path, FILE_READ);
    const bool result = entry && entry.isDirectory();
    if (entry) entry.close();
    unmountCard();
    return result;
}

bool StorageSD::fileExists(const char* path)
{
    if (path == nullptr) return false;
    fileSlot.close();
    if (!mountCard()) return false;

    ::File entry = SD.open(path, FILE_READ);
    const bool result = entry && !entry.isDirectory();
    if (entry) entry.close();
    unmountCard();
    return result;
}

} // namespace PRUZEAmini
