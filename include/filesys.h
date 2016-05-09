#ifndef __FILESYS__
#define __FILESYS__

/**
	@file
	@date		2012-08-27 09:04
	@author		libla
	@version	1.0
	@brief		
	
 */

#include "config.h"

namespace external
{
	class FileLoader;
	class FileHandle;
	class rc4key;
	class sstr;
}

using namespace external;

EXPORTS unsigned int ext_fnv(const char *path);
EXPORTS FileLoader * ext_mountraw(const char *prefix);
EXPORTS FileLoader * ext_mountzip(const char *path, const char *prefix);
EXPORTS void ext_unmount(FileLoader *loader);
EXPORTS int ext_mountexist(FileLoader *loader, const char *path);
EXPORTS sstr * ext_mountpath(FileLoader *loader, const char *str);
EXPORTS unsigned int ext_mountlength(FileLoader *loader, const char *path);
EXPORTS FileHandle * ext_mountopen(FileLoader *loader, const char *path, rc4key *key);
EXPORTS FileHandle * ext_mountopencompress(FileLoader *loader, const char *path, rc4key *key);
EXPORTS unsigned int ext_mountread(FileHandle *handle, void *buffer, unsigned int len);
EXPORTS void ext_mountclose(FileHandle *handle);

#endif // __FILESYS__
