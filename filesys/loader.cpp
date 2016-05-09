#define UTIL_CORE
#include <cstring>
#include <unzip.h>
#include "loader.h"
#include "handle.h"
#include "filesys.h"
#include "sstr.h"

namespace external
{
	RawFileLoader::RawFileLoader(const char *prefix)
	{
		strprefix = prefix;
		if (strprefix.length() > 0 && strprefix[strprefix.length() - 1] != '/')
		{
			strprefix += '/';
		}
	}

	RawFileLoader::~RawFileLoader()
	{
	}

	bool RawFileLoader::exist(const char *path)
	{
		std::string str(strprefix);
		str += path;
		FILE *file = fopen(str.c_str(), "rb");
		if (file != NULL)
		{
			fclose(file);
			return true;
		}
		return false;
	}

	size_t RawFileLoader::length(const char *path)
	{
		std::string str(strprefix);
		str += path;
		FILE *file = fopen(str.c_str(), "rb");
		if (file != NULL)
		{
			fseek(file, 0, SEEK_END);
			size_t len = ftell(file);
			fclose(file);
			return len;
		}
		return 0;
	}

	FileHandle * RawFileLoader::open(const char *path)
	{
		std::string str(strprefix);
		str += path;
		return new RawFileHandle(str.c_str());
	}

	sstr * RawFileLoader::path(const char *str)
	{
		std::string s(strprefix);
		s += str;
		return ext_sstr_new(s.c_str(), (int)s.length());
	}

	ZipFileLoader::ZipFileLoader(const char *path, const char *prefix)
	{
		strpath = path;
		strprefix = prefix;
		if (strprefix.length() > 0 && strprefix[strprefix.length() - 1] != '/')
		{
			strprefix += '/';
		}
		unzFile file = unzOpen(path);
		unz_global_info global_info;
		if (UNZ_OK == unzGetGlobalInfo(file, &global_info))
		{
			char *name = NULL;
			size_t length = 0;
			do 
			{
				unz_file_info info;
				unzGetCurrentFileInfo(file, &info, NULL, 0, NULL, 0, NULL, 0);
				if (length < info.size_filename + 1)
				{
					free(name);
					length = info.size_filename + 1;
					name = (char *)malloc(length);
				}
				unzGetCurrentFileInfo(file, NULL, name, info.size_filename, NULL, 0, NULL, 0);
				name[info.size_filename] = 0;
				if (strncmp(name, strprefix.c_str(), strprefix.length()) == 0)
				{
					FileInfo &fileinfo = files[ext_fnv(name + strprefix.length())];
					fileinfo.size = info.uncompressed_size;
					unzGetFilePos(file, &(fileinfo.pos));
				}
			} while (UNZ_OK == unzGoToNextFile(file));
			free(name);
		}
		unzClose(file);
	}

	ZipFileLoader::~ZipFileLoader()
	{
	}

	bool ZipFileLoader::exist(const char *path)
	{
		return files.find(ext_fnv(path)) != files.end();
	}

	size_t ZipFileLoader::length(const char *path)
	{
		std::map<unsigned int, FileInfo>::iterator i = files.find(ext_fnv(path));
		if (i != files.end())
		{
			return i->second.size;
		}
		return 0;
	}

	FileHandle * ZipFileLoader::open(const char *path)
	{
		std::map<unsigned int, FileInfo>::iterator i = files.find(ext_fnv(path));
		if (i != files.end())
		{
			return new ArchiveHandle(strpath.c_str(), i->second.pos);
		}
		return new EmptyFileHandle();
	}

	sstr * ZipFileLoader::path(const char *str)
	{
		return NULL;
	}
}
