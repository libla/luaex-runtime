#ifndef __HANDLE__
#define __HANDLE__

#include <stdio.h>
#include "zlib.h"
#include "unzip.h"

namespace external
{
	class rc4key;

	class FileHandle
	{
	public:
		virtual ~FileHandle() {}
		virtual size_t	read(void *output, size_t len) = 0;
	};

	class EmptyFileHandle : public FileHandle
	{
	public:
		virtual size_t	read(void *output, size_t len);
	};

	class RawFileHandle : public FileHandle
	{
	public:
		RawFileHandle(const char *path);
		virtual ~RawFileHandle();
		virtual size_t	read(void *output, size_t len);

	private:
		FILE *_file;
	};

	class CompressHandle : public FileHandle
	{
	public:
		CompressHandle(FileHandle *handle);
		virtual ~CompressHandle();
		virtual size_t	read(void *output, size_t len);

	private:
		FileHandle *	_handle;
		unsigned char	_input[65536];
		z_stream		_stream;
	};

	class ArchiveHandle : public FileHandle
	{
	public:
		ArchiveHandle(const char *path, const unz_file_pos &pos);
		virtual ~ArchiveHandle();
		virtual size_t	read(void *output, size_t len);

	private:
		unzFile _file;
	};

	class EncryptHandle : public FileHandle
	{
	public:
		EncryptHandle(FileHandle *handle, rc4key *key);
		virtual ~EncryptHandle();
		virtual size_t	read(void *output, size_t len);

	private:
		FileHandle *	_handle;
		rc4key *		_key;
	};
}

#endif
