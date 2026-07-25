#pragma once

#include "StorageBase.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

namespace PRUZEAmini {

class StorageSD;

class StorageSDFile : public StorageBaseFile
{
public:
    explicit StorageSDFile(StorageSD* owner) : owner(owner) {}
    ~StorageSDFile() override;

    bool openRead(const char* path);
    bool openWrite(const char* path, bool append);
    bool isOpen() const override;
    uint32_t size() const override;
    uint32_t read(void* buffer, uint32_t bytes) override;
    uint32_t write(const void* buffer, uint32_t bytes) override;
    void close() override;
    bool closeWrite() override;

private:
    StorageSD* owner = nullptr;
    ::File file;
    uint32_t fileSize = 0;
};

class StorageSD : public StorageBase
{
public:
    explicit StorageSD(const StorageSDConfig& config);
    ~StorageSD() override;

    bool begin() override;
    void end() override;
    bool isAvailable() const override;

    File* openRead(const char* path) override;
    File* openRead(const char* appId, const char* fileName) override;
    bool directoryExists(const char* path) override;
    bool fileExists(const char* path) override;

protected:
    StorageBaseFile* openWrite(const char* appId, const char* fileName, bool append) override;

private:
    friend class StorageSDFile;

    static constexpr size_t PATH_MAX_LENGTH = 128;
    static constexpr const char* ROOT_DIR = "/PRUZEA_Apps";
    static constexpr uint32_t AVAILABILITY_CACHE_MSEC = 1000;

    StorageSDConfig config;
    bool spiReady = false;
    bool sdMounted = false;
    SPIClass* spi = nullptr;
    bool ownsSpi = false;
    StorageSDFile fileSlot;
    mutable bool availabilityCached = false;
    mutable bool cachedAvailable = false;
    mutable uint32_t lastAvailabilityCheckMsec = 0;

    bool mountCard();
    void unmountCard();
    void updateAvailabilityCache(bool available) const;
    bool makeDataPath(char* output, size_t outputSize, const char* appId, const char* fileName) const;
    bool ensureDirectory(const char* path);
    static bool isValidFileName(const char* fileName);
};

} // namespace PRUZEAmini
