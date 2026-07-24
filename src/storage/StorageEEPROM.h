#pragma once

#include "StorageBase.h"

namespace PLAMIOmini {

class StorageEEPROM;

class StorageEEPROMFile : public StorageBaseFile
{
public:
    bool isOpen() const override;
    uint32_t size() const override;
    uint32_t read(void* buffer, uint32_t bytes) override;
    uint32_t write(const void* buffer, uint32_t bytes) override;
    void close() override;
    bool closeWrite() override;

private:
    friend class StorageEEPROM;

    void openRead(char* data, uint16_t* length, uint16_t capacity);
    void openWrite(StorageEEPROM* storage, char* data, uint16_t* length, uint16_t capacity);

    StorageEEPROM* storage = nullptr;
    char* data = nullptr;
    uint16_t* length = nullptr;
    uint16_t capacity = 0;
    uint16_t position = 0;
};

class StorageEEPROM : public StorageBase
{
public:
    StorageEEPROM() = default;
    StorageEEPROM(const StorageEEPROMConfig& config, const char* appId);

    bool begin() override;
    void end() override;
    bool isAvailable() const override { return ready; }

    File* openRead(const char* path) override;
    File* openRead(const char* gameId, const char* fileName) override;
    bool directoryExists(const char* path) override;
    bool fileExists(const char* path) override;

protected:
    StorageBaseFile* openWrite(const char* gameId, const char* fileName, bool append) override;
    bool supportsUserFileWrite() const override { return false; }

private:
    friend class StorageEEPROMFile;

    static constexpr uint8_t SLOT_COUNT = 4;
    static constexpr uint16_t DATA_SIZE = 512;

    struct Slot
    {
        char gameId[33];
        char fileName[33];
        uint16_t length;
        char data[DATA_SIZE];
    };

    struct Image
    {
        uint16_t magic;
        uint8_t version;
        uint32_t appIdHash;
        Slot slots[SLOT_COUNT];
    };

    StorageEEPROMConfig config;
    uint32_t currentAppIdHash = 0;
    Image image = {};
    bool ready = false;
    StorageEEPROMFile fileSlot;

    int find(const char* gameId, const char* fileName) const;
    int allocate(const char* gameId, const char* fileName);
    static uint32_t hashString(const char* value);
    bool commit();
};

} // namespace PLAMIOmini
