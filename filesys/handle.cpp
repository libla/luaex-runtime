#define UTIL_CORE
#include "handle.h"
#include "rc4.h"

namespace external
{
	size_t EmptyFileHandle::read(void *output, size_t len)
	{
		return 0;
	}

	RawFileHandle::RawFileHandle(const char *path)
	{
		_file = fopen(path, "rb");
	}

	RawFileHandle::~RawFileHandle()
	{
		if (_file != NULL)
		{
			fclose(_file);
		}
	}

	size_t RawFileHandle::read(void *output, size_t len)
	{
		if (_file == NULL)
			return 0;
		return fread(output, 1, len, _file);
	}

	CompressHandle::CompressHandle(FileHandle *handle)
	{
		_handle = handle;
		_stream.zalloc = Z_NULL;
		_stream.zfree = Z_NULL;
		_stream.opaque = Z_NULL;
		inflateInit(&_stream);
		_stream.avail_out = -1;
	}

	CompressHandle::~CompressHandle()
	{
		inflateEnd(&_stream);
		delete _handle;
	}

	size_t CompressHandle::read(void *output, size_t len)
	{
		size_t offset = 0;
		while (offset != len)
		{
			if (_stream.avail_out != 0)
			{
				_stream.avail_in = _handle->read(_input, sizeof(_input));
				_stream.next_in = _input;
			}
			if (0 == _stream.avail_in)
				break;

			_stream.avail_out = len - offset;
			_stream.next_out = (Bytef *)output + offset;
			int result = inflate(&_stream, Z_NO_FLUSH);
			offset = len - _stream.avail_out;
			if (Z_STREAM_END == result)
			{
				break;
			}
		}
		return offset;
	}

	ArchiveHandle::ArchiveHandle(const char *path, const unz_file_pos &pos)
	{
		_file = unzOpen(path);
		unzGoToFilePos(_file, &pos);
		unzOpenCurrentFile(_file);
	}

	ArchiveHandle::~ArchiveHandle()
	{
		if (_file != NULL)
		{
			unzCloseCurrentFile(_file);
			unzClose(_file);
		}
	}

	size_t ArchiveHandle::read(void *output, size_t len)
	{
		int result = unzReadCurrentFile(_file, output, len);
		return result <= 0 ? 0 : (size_t)result;
	}

	EncryptHandle::EncryptHandle(FileHandle *handle, rc4key *key)
	{
		_handle = handle;
		_key = key;
	}

	EncryptHandle::~EncryptHandle()
	{
		delete _handle;
	}

	size_t EncryptHandle::read(void *output, size_t len)
	{
		len = _handle->read(output, len);
		ext_rc4_encode(_key, output, output, (unsigned int)len);
		return len;
	}
}
