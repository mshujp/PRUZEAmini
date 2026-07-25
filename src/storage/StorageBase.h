#pragma once

#include "PRUZEAmini.h"

namespace PRUZEAmini {

class StorageBaseFile : public Storage::File
{
protected:
    enum class OpenMode : uint8_t
    {
        CLOSED,
        READ,
        WRITE
    };
    OpenMode mode = OpenMode::CLOSED;

public:
    virtual uint32_t write(const void* buffer, uint32_t bytes) = 0;
    virtual bool closeWrite() = 0;
    ~StorageBaseFile() override = default;
};

class StorageBase : public Storage
{
private:
    static constexpr uint8_t APP_ID_MAX_LENGTH = 32;
    static constexpr uint32_t TEXT_READ_BUFFER_SIZE = 256;

protected:
    static bool isValidAppId(const char* appId);
    virtual StorageBaseFile* openWrite(const char* appId, const char* fileName, bool append) = 0;
    virtual bool supportsUserFileWrite() const { return true; }

public:
    using BinaryFileWriterHandler = bool(*)(StorageBaseFile& file, void* arg);

    virtual ~StorageBase() = default;
    virtual bool begin() = 0; 
    virtual void end() = 0; 

    virtual File* openRead(const char* appId, const char* fileName) = 0;
    bool readUserFile(const char* appId, const char* fileName, Storage::UserFileLineReaderHandler handler, void* arg) override;
    bool writeUserFile(const char* appId, const char* fileName, Storage::UserFileLineWriterHandler writer, void* arg) override;
    bool writeUserFile(const char* appId, const char* fileName, const char* data, bool append) override;

    bool userFileExists(const char* appId, const char* fileName);
    bool writeBinaryFile(const char* appId, const char* fileName, BinaryFileWriterHandler writer, void* arg);

private:
    bool writeSaveDataInternal(const char* appId, const char* fileName, Storage::UserFileLineWriterHandler writer, void* arg) override;
    bool writeLines(const char* appId, const char* fileName, Storage::UserFileLineWriterHandler writer, void* arg, bool append);
};

} // namespace
