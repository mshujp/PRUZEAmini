#include "StorageBase.h"

#include <stdio.h>
#include <cstring>

using namespace PLAMIOmini;

bool StorageBase::isValidAppId(const char* appId)
{
    if (appId == nullptr || appId[0] == '\0') return false;

    uint8_t len = 0;
    for (const char* p = appId; *p != '\0'; ++p)
    {
        if (++len > APP_ID_MAX_LENGTH) return false;

        const char c = *p;
        const bool lowerAlpha = c >= 'a' && c <= 'z';
        const bool digit = c >= '0' && c <= '9';
        const bool underscore = c == '_';
        const bool hyphen = c == '-';
        if (!lowerAlpha && !digit && !underscore && !hyphen) return false;
    }

    return true;
}

bool StorageBase::userFileExists(const char* appId, const char* fileName)
{
    if (!isAvailable() || !isValidAppId(appId) || fileName == nullptr || fileName[0] == '\0') return false;

    File* file = openRead(appId, fileName);
    if (file == nullptr) return false;

    const bool exists = file->isOpen();
    file->close();
    return exists;
}

bool StorageBase::writeBinaryFile(const char* appId, const char* fileName, BinaryFileWriterHandler writer, void* arg)
{
    if (!isAvailable() || !isValidAppId(appId) || fileName == nullptr || fileName[0] == '\0' || writer == nullptr)
    {
        return false;
    }

    StorageBaseFile* file = openWrite(appId, fileName, false);
    if (file == nullptr) return false;

    const bool writeResult = writer(*file, arg);
    const bool closeResult = file->closeWrite();
    return writeResult && closeResult;
}

bool StorageBase::writeUserFile(const char* appId, const char* fileName, Storage::UserFileLineWriterHandler writer, void* arg)
{
    if (!supportsUserFileWrite()) return false;
    return writeLines(appId, fileName, writer, arg, false);
}

bool StorageBase::writeSaveDataInternal(
    const char* appId,
    const char* fileName,
    Storage::UserFileLineWriterHandler writer,
    void* arg)
{
    return writeLines(appId, fileName, writer, arg, false);
}

bool StorageBase::writeLines(const char* appId, const char* fileName, Storage::UserFileLineWriterHandler writer, void* arg, bool append)
{
    if (!isAvailable() || appId == nullptr || fileName == nullptr || writer == nullptr) return false;

    StorageBaseFile* file = openWrite(appId, fileName, append);
    if (file == nullptr) return false;

    bool ok = true;
    uint16_t lineCount = 0;
    std::string line;

    for (;;)
    {
        line.clear();
        if (!writer(line, arg)) break;

        if (lineCount >= Storage::USER_FILE_MAX_LINES)
        {
            ok = false;
            break;
        }

        if (file->write(line.data(), static_cast<uint32_t>(line.size())) != line.size() ||
            file->write("\n", 1) != 1)
        {
            ok = false;
            break;
        }

        ++lineCount;
    }

    if (!file->closeWrite()) ok = false;
    return ok;
}

bool StorageBase::writeUserFile(
    const char* appId,
    const char* fileName,
    const char* data,
    bool append)
{
    if (!supportsUserFileWrite() || data == nullptr) return false;

    constexpr size_t SIMPLE_WRITE_MAX = 200;
    if (std::strlen(data) > SIMPLE_WRITE_MAX) return false;

    struct SingleWriteState
    {
        const char* data;
        bool written;
    };

    SingleWriteState state{data, false};
    return writeLines(appId, fileName, [](std::string& line, void* arg) -> bool {
        auto* state = static_cast<SingleWriteState*>(arg);
        if (state->written) return false;

        line.assign(state->data);
        state->written = true;
        return true;
    }, &state, append);
}

bool StorageBase::readUserFile(const char* appId, const char* fileName, Storage::UserFileLineReaderHandler handler, void* arg)
{
    if (!isAvailable() || appId == nullptr || fileName == nullptr || handler == nullptr) return false;

    File* file = openRead(appId, fileName);
    if (file == nullptr) return false;

    char readBuffer[TEXT_READ_BUFFER_SIZE];
    char lineBuffer[TEXT_READ_BUFFER_SIZE];

    uint16_t lineLen = 0;
    bool discardingLongLine = false;

    uint32_t bytesRead = 0;
    while ((bytesRead = file->read(readBuffer, sizeof(readBuffer))) > 0)
    {
        for (uint32_t i = 0; i < bytesRead; ++i)
        {
            const char c = readBuffer[i];

            if (c == '\n')
            {
                if (!discardingLongLine)
                {
                    if (lineLen > 0 && lineBuffer[lineLen - 1] == '\r')
                    {
                        --lineLen;
                    }

                    lineBuffer[lineLen] = '\0';
                    if (!handler(lineBuffer, arg))
                    {
                        file->close();
                        return true;
                    }
                }

                lineLen = 0;
                discardingLongLine = false;
                continue;
            }

            if (discardingLongLine)
            {
                continue;
            }

            if (lineLen >= TEXT_READ_BUFFER_SIZE - 1)
            {
                lineLen = 0;
                discardingLongLine = true;
                continue;
            }

            lineBuffer[lineLen++] = c;
        }
    }

    if (!discardingLongLine && lineLen > 0)
    {
        if (lineLen > 0 && lineBuffer[lineLen - 1] == '\r')
        {
            --lineLen;
        }

        lineBuffer[lineLen] = '\0';
        if (!handler(lineBuffer, arg)){
            file->close();
            return true;
        }
    }

    file->close();
    return true;
}
