#define UTIL_CORE
#include <cctype>
#include <cstdio>
#include "filesys.h"
#include "loader.h"
#include "handle.h"

unsigned int ext_fnv(const char *path)
{
	unsigned int num = 2166136261U;
	while (true)
	{
		unsigned char byte = *path++;
		if (byte == 0)
			break;
		if (byte == '\\')
			byte = '/';
		else
			byte = tolower(byte);
		num ^= byte;
		num *= 16777619U;
	}
	return num;
}

FileLoader * ext_mountraw(const char *prefix)
{
	return new RawFileLoader(prefix);
}

FileLoader * ext_mountzip(const char *path, const char *prefix)
{
	return new ZipFileLoader(path, prefix);
}

void ext_unmount(FileLoader *loader)
{
	delete loader;
}

int ext_mountexist(FileLoader *loader, const char *path)
{
	return loader->exist(path) ? 1 : 0;
}

sstr * ext_mountpath(FileLoader *loader, const char *str)
{
	return loader->path(str);
}

unsigned int ext_mountlength(FileLoader *loader, const char *path)
{
	return (unsigned int)loader->length(path);
}

FileHandle * ext_mountopen(FileLoader *loader, const char *path, rc4key *key)
{
	FileHandle *handle = loader->open(path);
	return key == NULL ? handle : new EncryptHandle(handle, key);
}

FileHandle * ext_mountopencompress(FileLoader *loader, const char *path, rc4key *key)
{
	FileHandle *handle = loader->open(path);
	return new CompressHandle(key == NULL ? handle : new EncryptHandle(handle, key));
}

unsigned int ext_mountread(FileHandle *handle, void *buffer, unsigned int len)
{
	return (unsigned int)handle->read(buffer, len);
}

void ext_mountclose(FileHandle *handle)
{
	delete handle;
}
