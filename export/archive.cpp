#define UTIL_CORE
#include <string>
#include <map>
#include <vector>
#include <ctime>
#include <cstring>
#include "zlib.h"
#include "zip.h"
#include "unzip.h"
#include "filesys.h"
#include "archive.h"

namespace external
{
	class Archive
	{
	protected:
		std::string strpath;
		bool dirty;

		struct FileInfo
		{
			size_t size;
			unz_file_pos pos;
		};
		std::map<unsigned int, FileInfo> filepos;
		std::vector<std::string> files;
		zipFile writer;

	protected:
		void initIndexs()
		{
			if (dirty)
			{
				dirty = false;
				files.clear();
				filepos.clear();

				unzFile file = unzOpen(strpath.c_str());
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
						files.push_back(name);
						FileInfo &fileinfo = filepos[ext_fnv(name)];
						fileinfo.size = info.uncompressed_size;
						unzGetFilePos(file, &(fileinfo.pos));
					} while (UNZ_OK == unzGoToNextFile(file));
					free(name);
				}
				unzClose(file);
			}
		}

	public:
		Archive(const char *path) : strpath(path), dirty(true), writer(NULL)
		{
		}

		~Archive()
		{
			if (writer != NULL)
				zipClose(writer, NULL);
		}

		size_t GetFileCount()
		{
			initIndexs();
			return files.size();
		}

		const std::string & GetFile(size_t index)
		{
			initIndexs();
			return files[index];
		}

		bool HasFile(const char *str)
		{
			return HasFile(ext_fnv(str));
		}
		
		bool HasFile(unsigned int id)
		{
			initIndexs();
			return filepos.find(id) != filepos.end();
		}

		size_t FileLength(const char *str)
		{
			return FileLength(ext_fnv(str));
		}
		
		size_t FileLength(unsigned int id)
		{
			initIndexs();
			std::map<unsigned int, FileInfo>::iterator i = filepos.find(id);
			if (i != filepos.end())
			{
				return i->second.size;
			}
			return 0;
		}

		size_t Write(const char *path, const void *input, size_t len)
		{
			if (writer == NULL)
			{
				writer = zipOpen(strpath.c_str(), APPEND_STATUS_ADDINZIP);
				if (writer == NULL)
				{
					writer = zipOpen(strpath.c_str(), APPEND_STATUS_CREATE);
				}
				if (writer == NULL)
				{
					return 0;
				}
			}
			zip_fileinfo fileinfo;
			memset(&fileinfo, 0, sizeof(fileinfo));
			time_t now = time(NULL);
			tm *nowtime = localtime(&now);
			fileinfo.tmz_date.tm_year = nowtime->tm_year + 1900;
			fileinfo.tmz_date.tm_mon = nowtime->tm_mon;
			fileinfo.tmz_date.tm_mday = nowtime->tm_mday;
			fileinfo.tmz_date.tm_hour = nowtime->tm_hour;
			fileinfo.tmz_date.tm_min = nowtime->tm_min;
			fileinfo.tmz_date.tm_sec = nowtime->tm_sec;
			if (ZIP_OK !=
				zipOpenNewFileInZip(writer, path, &fileinfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION))
			{
				return 0;
			}
			dirty = true;
			if (ZIP_OK != zipWriteInFileInZip(writer, input, (unsigned int)len))
			{
				zipCloseFileInZip(writer);
				return 0;
			}
			zipCloseFileInZip(writer);
			return len;
		}

		size_t Read(const char *path, void *output, size_t len)
		{
			return Read(ext_fnv(path), output, len);
		}
		
		size_t Read(unsigned int id, void *output, size_t len)
		{
			initIndexs();
			std::map<unsigned int, FileInfo>::iterator i = filepos.find(id);
			if (i == filepos.end())
			{
				return 0;
			}
			unzFile file = unzOpen(strpath.c_str());
			unzGoToFilePos(file, &(i->second.pos));
			unzOpenCurrentFile(file);
			int result = unzReadCurrentFile(file, output, len);
			unzCloseCurrentFile(file);
			unzClose(file);
			return result <= 0 ? 0 : (size_t)result;
		}
	};
}

Archive * ext_archive_open(const char *path)
{
	return new Archive(path);
}

void ext_archive_close(Archive *file)
{
	delete file;
}

unsigned int ext_archive_files(Archive *file)
{
	return (unsigned int)(file->GetFileCount());
}

const char * ext_archive_file(Archive *file, unsigned int index, unsigned int *len)
{
	const std::string &str = file->GetFile(index);
	if (len != NULL)
	{
		*len = str.length();
	}
	return str.c_str();
}

unsigned int ext_archive_length(Archive *file, const char *path)
{
	return (unsigned int)(file->FileLength(path));
}

int ext_archive_exist(Archive *file, const char *path)
{
	return file->HasFile(path) ? 1 : 0;
}

unsigned int ext_archive_read(Archive *file, const char *path, void *output, unsigned int len)
{
	return (unsigned int)(file->Read(path, output, len));
}

unsigned int ext_archive_write(Archive *file, const char *path, const void *input, unsigned int len)
{
	return (unsigned int)(file->Write(path, input, len));
}

unsigned int ext_archive_hash(const char *path)
{
	return ext_fnv(path);
}

unsigned int ext_archive_length_hash(Archive *file, unsigned int id)
{
	return (unsigned int)(file->FileLength(id));
}

int ext_archive_exist_hash(Archive *file, unsigned int id)
{
	return file->HasFile(id) ? 1 : 0;
}

unsigned int ext_archive_read_hash(Archive *file, unsigned int id, void *output, unsigned int len)
{
	return (unsigned int)(file->Read(id, output, len));
}

