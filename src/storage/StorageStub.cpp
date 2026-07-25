#include "StorageStub.h"

using namespace PRUZEAmini;

bool StorageStub::begin()
{
    return true;
}

void StorageStub::end()
{
}

Storage::File* StorageStub::openRead(const char* path)
{
    return nullptr;
}

Storage::File* StorageStub::openRead(const char* appId, const char* fileName)
{
    return nullptr;
}

StorageBaseFile* StorageStub::openWrite(const char* appId, const char* fileName, bool append)
{
    return nullptr;
}

bool StorageStub::directoryExists(const char* path)
{
    return false;
}

bool StorageStub::fileExists(const char* path)
{
    return false;
}
