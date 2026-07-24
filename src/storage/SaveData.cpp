#include "../PLAMIOmini.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace PLAMIOmini
{

namespace
{
    constexpr size_t LOAD_KEY_BUF_SIZE = 64;
    constexpr size_t LOAD_VALUE_BUF_SIZE = 256;

    bool isValidKey(const char* key)
    {
        if (key == nullptr || key[0] == '\0')
        {
            return false;
        }
        for (const char* p = key; *p != '\0'; ++p)
        {
            if (*p == '=' || *p == '\n' || *p == '\r')
            {
                return false;
            }
        }
        return true;
    }

    bool isValidValue(const char* value)
    {
        if (value == nullptr)
        {
            return false;
        }
        for (const char* p = value; *p != '\0'; ++p)
        {
            if (*p == '\n' || *p == '\r')
            {
                return false;
            }
        }
        return true;
    }

    struct LoadContext
    {
        SaveData* self;
        bool failed;
    };

    bool loadLineCallback(const char* line, void* arg)
    {
        LoadContext* ctx = static_cast<LoadContext*>(arg);
        if (line == nullptr)
        {
            return true;
        }

        const char* eq = strchr(line, '=');
        if (eq == nullptr || eq == line)
        {
            return true;
        }

        const size_t keyLen = static_cast<size_t>(eq - line);
        if (keyLen >= LOAD_KEY_BUF_SIZE)
        {
            return true;
        }
        char keyBuf[LOAD_KEY_BUF_SIZE];
        memcpy(keyBuf, line, keyLen);
        keyBuf[keyLen] = '\0';

        const char* rawValue = eq + 1;
        size_t valueLen = strlen(rawValue);
        while (valueLen > 0 && (rawValue[valueLen - 1] == '\r' || rawValue[valueLen - 1] == '\n'))
        {
            --valueLen;
        }
        if (valueLen >= LOAD_VALUE_BUF_SIZE)
        {
            valueLen = LOAD_VALUE_BUF_SIZE - 1;
        }
        char valueBuf[LOAD_VALUE_BUF_SIZE];
        memcpy(valueBuf, rawValue, valueLen);
        valueBuf[valueLen] = '\0';

        if (!ctx->self->setString(keyBuf, valueBuf))
        {
            ctx->failed = true;
            return false;
        }
        return true;
    }
} // namespace

SaveData::SaveData() : entryCount(0), usedBytes(0), dirty(false), saveCursor(0)
{
}

void SaveData::clear()
{
    entryCount = 0;
    usedBytes = 0;
    dirty = true;
}

int16_t SaveData::findEntry(const char* key) const
{
    if (key == nullptr)
    {
        return -1;
    }
    for (uint8_t i = 0; i < entryCount; ++i)
    {
        if (strcmp(buffer + entries[i].keyOffset, key) == 0)
        {
            return static_cast<int16_t>(i);
        }
    }
    return -1;
}

bool SaveData::appendString(const char* text, uint16_t& offset)
{
    const size_t len = strlen(text) + 1;
    if (usedBytes + len > BUFFER_SIZE)
    {
        return false;
    }
    memcpy(buffer + usedBytes, text, len);
    offset = usedBytes;
    usedBytes = static_cast<uint16_t>(usedBytes + len);
    return true;
}

bool SaveData::contains(const char* key) const
{
    return findEntry(key) >= 0;
}

bool SaveData::remove(const char* key)
{
    const int16_t idx = findEntry(key);
    if (idx < 0)
    {
        return false;
    }
    entries[idx] = entries[entryCount - 1];
    entryCount--;
    dirty = true;
    return true;
}

bool SaveData::getString(const char* key, char* outValue, size_t outSize, const char* defaultValue) const
{
    if (outValue == nullptr || outSize == 0)
    {
        return false;
    }

    const int16_t idx = findEntry(key);
    if (idx < 0)
    {
        size_t i = 0;
        if (defaultValue != nullptr)
        {
            for (; i < outSize - 1 && defaultValue[i] != '\0'; ++i)
            {
                outValue[i] = defaultValue[i];
            }
        }
        outValue[i] = '\0';
        return false;
    }

    const char* value = buffer + entries[idx].valueOffset;
    size_t i = 0;
    for (; i < outSize - 1 && value[i] != '\0'; ++i)
    {
        outValue[i] = value[i];
    }
    const bool fits = (value[i] == '\0');
    outValue[i] = '\0';
    return fits;
}

int32_t SaveData::getInt32(const char* key, int32_t defaultValue) const
{
    const int16_t idx = findEntry(key);
    if (idx < 0)
    {
        return defaultValue;
    }
    const char* value = buffer + entries[idx].valueOffset;
    if (value[0] == '\0')
    {
        return defaultValue;
    }
    char* endPtr = nullptr;
    const long long result = strtoll(value, &endPtr, 10);
    if (endPtr == value || *endPtr != '\0')
    {
        return defaultValue; // trailing garbage or empty
    }
    if (result < static_cast<long long>(INT32_MIN) || result > static_cast<long long>(INT32_MAX))
    {
        return defaultValue; // out of range
    }
    return static_cast<int32_t>(result);
}

uint32_t SaveData::getUInt32(const char* key, uint32_t defaultValue) const
{
    const int16_t idx = findEntry(key);
    if (idx < 0)
    {
        return defaultValue;
    }
    const char* value = buffer + entries[idx].valueOffset;
    if (value[0] == '\0' || value[0] == '-')
    {
        return defaultValue; // empty or negative is not a valid uint32
    }
    char* endPtr = nullptr;
    const unsigned long long result = strtoull(value, &endPtr, 10);
    if (endPtr == value || *endPtr != '\0')
    {
        return defaultValue;
    }
    if (result > static_cast<unsigned long long>(UINT32_MAX))
    {
        return defaultValue;
    }
    return static_cast<uint32_t>(result);
}

bool SaveData::getBool(const char* key, bool defaultValue) const
{
    const int16_t idx = findEntry(key);
    if (idx < 0)
    {
        return defaultValue;
    }
    const char* value = buffer + entries[idx].valueOffset;
    if (strcmp(value, "1") == 0)
    {
        return true;
    }
    if (strcmp(value, "0") == 0)
    {
        return false;
    }
    return defaultValue;
}

bool SaveData::setString(const char* key, const char* value)
{
    if (!isValidKey(key) || !isValidValue(value))
    {
        return false;
    }

    const int16_t idx = findEntry(key);
    if (idx >= 0)
    {
        const char* currentValue = buffer + entries[idx].valueOffset;
        if (strcmp(currentValue, value) == 0)
        {
            return true;
        }
        uint16_t newValueOffset = 0;
        if (!appendString(value, newValueOffset))
        {
            return false;
        }
        entries[idx].valueOffset = newValueOffset;
        dirty = true;
        return true;
    }

    if (entryCount >= MAX_ENTRIES)
    {
        return false;
    }

    const uint16_t savedUsedBytes = usedBytes;
    uint16_t keyOffset = 0;
    uint16_t valueOffset = 0;
    if (!appendString(key, keyOffset))
    {
        return false;
    }
    if (!appendString(value, valueOffset))
    {
        usedBytes = savedUsedBytes;
        return false;
    }

    entries[entryCount].keyOffset = keyOffset;
    entries[entryCount].valueOffset = valueOffset;
    entryCount++;
    dirty = true;
    return true;
}

bool SaveData::setInt32(const char* key, int32_t value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", static_cast<long>(value));
    return setString(key, buf);
}

bool SaveData::setUInt32(const char* key, uint32_t value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
    return setString(key, buf);
}

bool SaveData::setBool(const char* key, bool value)
{
    return setString(key, value ? "1" : "0");
}

bool SaveData::isDirty() const
{
    return dirty;
}

uint8_t SaveData::getEntryCount() const
{
    return entryCount;
}

uint16_t SaveData::getUsedBytes() const
{
    return usedBytes;
}

uint16_t SaveData::getFreeBytes() const
{
    return static_cast<uint16_t>(BUFFER_SIZE - usedBytes);
}

bool SaveData::load(Storage& storage, const char* appId, const char* fileName)
{
    clear();
    dirty = false;

    if (!storage.isAvailable())
    {
        return false;
    }

    LoadContext ctx{ this, false };
    const bool ok = storage.readUserFile(appId, fileName, &loadLineCallback, &ctx);

    if (!ok)
    {
        if (ctx.failed)
        {
            dirty = false;
            return false;
        }

        clear();
        dirty = false;
        return true;
    }

    dirty = false;
    return true;
}

bool SaveData::writeLineHandler(std::string& line, void* arg)
{
    SaveData* self = static_cast<SaveData*>(arg);
    if (self->saveCursor >= self->entryCount)
    {
        return false;
    }

    const Entry& entry = self->entries[self->saveCursor];
    const char* key = self->buffer + entry.keyOffset;
    const char* value = self->buffer + entry.valueOffset;

    char lineBuf[BUFFER_SIZE + 2];
    snprintf(lineBuf, sizeof(lineBuf), "%s=%s", key, value);
    line.assign(lineBuf);

    self->saveCursor++;
    return true;
}

bool SaveData::save(Storage& storage, const char* appId, const char* fileName)
{
    if (!dirty)
    {
        return true;
    }
    if (!storage.isAvailable())
    {
        return false;
    }

    saveCursor = 0;
    const bool ok = storage.writeSaveDataInternal(appId, fileName, &SaveData::writeLineHandler, this);
    if (ok)
    {
        dirty = false;
    }
    return ok;
}

} // namespace PLAMIOmini
