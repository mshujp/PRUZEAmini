#pragma once

#include "StorageBase.h"

namespace PLAMIOmini {

class StorageStub : public StorageBase
{
protected:
    StorageBaseFile* openWrite(const char* appId, const char* fileName, bool append) override;

public:
    bool begin() override;
    void end() override;
    bool isAvailable() const override { return false; }
    File* openRead(const char* path) override;
    File* openRead(const char* appId, const char* fileName) override;
    bool directoryExists(const char* path) override;
    bool fileExists(const char* path) override;
};

} // namespace
