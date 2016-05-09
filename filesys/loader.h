#ifndef __LOADER__
#define __LOADER__

#include <stdio.h>
#include <string>
#include <map>
#include <unzip.h>

namespace external
{
	class FileHandle;
	class sstr;

	class FileLoader
	{
	public:
		virtual ~FileLoader() {}
		virtual bool			exist(const char *path) = 0;
		virtual size_t			length(const char *path) = 0;
		virtual FileHandle *	open(const char *path) = 0;
		virtual sstr *			path(const char *str) = 0;
	};

	class RawFileLoader : public FileLoader
	{
	public:
		RawFileLoader(const char *prefix);
		virtual ~RawFileLoader();
		virtual bool			exist(const char *path);
		virtual size_t			length(const char *path);
		virtual FileHandle *	open(const char *path);
		virtual sstr *			path(const char *str);

	protected:
		std::string strprefix;
	};

	class ZipFileLoader : public FileLoader
	{
	public:
		ZipFileLoader(const char *path, const char *prefix);
		virtual ~ZipFileLoader();
		virtual bool			exist(const char *path);
		virtual size_t			length(const char *path);
		virtual FileHandle *	open(const char *path);
		virtual sstr *			path(const char *str);

	protected:
		std::string strpath;
		std::string strprefix;

		struct FileInfo
		{
			size_t size;
			unz_file_pos pos;
		};
		std::map<unsigned int, FileInfo> files;
	};

}

#endif
