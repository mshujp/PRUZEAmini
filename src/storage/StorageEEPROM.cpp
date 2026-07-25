#include "StorageEEPROM.h"

#include <EEPROM.h>
#include <algorithm>
#include <cstring>

namespace PRUZEAmini {

StorageEEPROM::StorageEEPROM(const StorageEEPROMConfig& config, const char* appId)
    : config(config), currentAppIdHash(hashString(appId))
{
}

uint32_t StorageEEPROM::hashString(const char* value)
{
    constexpr uint32_t FNV_OFFSET_BASIS = 2166136261u;
    constexpr uint32_t FNV_PRIME = 16777619u;

    uint32_t hash = FNV_OFFSET_BASIS;
    if (value == nullptr) return hash;

    while (*value != '\0')
    {
        hash ^= static_cast<uint8_t>(*value++);
        hash *= FNV_PRIME;
    }
    return hash;
}

bool StorageEEPROMFile::isOpen() const
{
    return mode != OpenMode::CLOSED && data != nullptr && length != nullptr;
}

uint32_t StorageEEPROMFile::size() const
{
    return isOpen() ? *length : 0;
}

void StorageEEPROMFile::openRead(char* value, uint16_t* valueLength, uint16_t valueCapacity)
{
    close();
    data = value;
    length = valueLength;
    capacity = valueCapacity;
    position = 0;
    mode = OpenMode::READ;
}

void StorageEEPROMFile::openWrite(StorageEEPROM* owner, char* value,
                                  uint16_t* valueLength, uint16_t valueCapacity)
{
    close();
    storage = owner;
    data = value;
    length = valueLength;
    capacity = valueCapacity;
    position = 0;
    *length = 0;
    mode = OpenMode::WRITE;
}

uint32_t StorageEEPROMFile::read(void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::READ || buffer == nullptr || position >= *length) return 0;

    const uint32_t count = std::min<uint32_t>(bytes, *length - position);
    memcpy(buffer, data + position, count);
    position += static_cast<uint16_t>(count);
    return count;
}

uint32_t StorageEEPROMFile::write(const void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::WRITE || (buffer == nullptr && bytes > 0) ||
        position + bytes > capacity) return 0;

    memcpy(data + position, buffer, bytes);
    position += static_cast<uint16_t>(bytes);
    *length = position;
    return bytes;
}

void StorageEEPROMFile::close()
{
    mode = OpenMode::CLOSED;
    storage = nullptr;
    data = nullptr;
    length = nullptr;
    capacity = 0;
    position = 0;
}

bool StorageEEPROMFile::closeWrite()
{
    if (mode != OpenMode::WRITE || storage == nullptr) return false;

    StorageEEPROM* owner = storage;
    close();
    return owner->commit();
}

int StorageEEPROM::find(const char* appId, const char* fileName) const
{
    if (appId == nullptr || fileName == nullptr) return -1;

    for (int i = 0; i < SLOT_COUNT; ++i)
    {
        const Slot& slot = image.slots[i];
        if (slot.appId[0] != '\0' && strcmp(slot.appId, appId) == 0 &&
            strcmp(slot.fileName, fileName) == 0) return i;
    }
    return -1;
}

int StorageEEPROM::allocate(const char* appId, const char* fileName)
{
    int index = find(appId, fileName);
    if (index >= 0) return index;

    for (index = 0; index < SLOT_COUNT; ++index)
    {
        Slot& slot = image.slots[index];
        if (slot.appId[0] == '\0')
        {
            strncpy(slot.appId, appId, sizeof(slot.appId) - 1);
            slot.appId[sizeof(slot.appId) - 1] = '\0';
            strncpy(slot.fileName, fileName, sizeof(slot.fileName) - 1);
            slot.fileName[sizeof(slot.fileName) - 1] = '\0';
            slot.length = 0;
            return index;
        }
    }
    return -1;
}

bool StorageEEPROM::begin()
{
    if (config.eepromSize < sizeof(Image)) return false;

#if defined(ARDUINO_ARCH_ESP32)
    if (!EEPROM.begin(config.eepromSize)) return false;
#else
    EEPROM.begin(config.eepromSize);
#endif

    EEPROM.get(0, image);
    if (image.magic != config.magic ||
        image.version != config.version ||
        image.appIdHash != currentAppIdHash)
    {
        memset(&image, 0, sizeof(image));
        image.magic = config.magic;
        image.version = config.version;
        image.appIdHash = currentAppIdHash;
        if (!commit()) return false;
    }

    ready = true;
    return true;
}

void StorageEEPROM::end()
{
    fileSlot.close();
    ready = false;
}

bool StorageEEPROM::commit()
{
    EEPROM.put(0, image);
    return EEPROM.commit();
}

Storage::File* StorageEEPROM::openRead(const char* path)
{
    return nullptr;
}

Storage::File* StorageEEPROM::openRead(const char* appId, const char* fileName)
{
    if (!ready || !isValidAppId(appId)) return nullptr;

    const int index = find(appId, fileName);
    if (index < 0) return nullptr;

    Slot& slot = image.slots[index];
    fileSlot.openRead(slot.data, &slot.length, DATA_SIZE);
    return &fileSlot;
}

StorageBaseFile* StorageEEPROM::openWrite(
    const char* appId,
    const char* fileName,
    bool append)
{
    if (append) return nullptr;
    if (!ready || !isValidAppId(appId) || fileName == nullptr || fileName[0] == '\0') return nullptr;

    const int index = allocate(appId, fileName);
    if (index < 0) return nullptr;

    Slot& slot = image.slots[index];
    fileSlot.openWrite(this, slot.data, &slot.length, DATA_SIZE);
    return &fileSlot;
}

bool StorageEEPROM::directoryExists(const char* path)
{
    return false;
}

bool StorageEEPROM::fileExists(const char* path)
{
    return false;
}

} // namespace PRUZEAmini
