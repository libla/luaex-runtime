#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include "config.h"

namespace external
{
	class Archive;
}

using namespace external;

EXPORTS Archive *		ext_archive_open(const char *path);
EXPORTS void			ext_archive_close(Archive *file);
EXPORTS unsigned int	ext_archive_files(Archive *file);
EXPORTS const char *	ext_archive_file(Archive *file, unsigned int index, unsigned int *len);
EXPORTS unsigned int	ext_archive_length(Archive *file, const char *path);
EXPORTS int				ext_archive_exist(Archive *file, const char *path);
EXPORTS unsigned int	ext_archive_read(Archive *file, const char *path, void *output, unsigned int len);
EXPORTS unsigned int	ext_archive_write(Archive *file, const char *path, const void *input, unsigned int len);
EXPORTS unsigned int	ext_archive_hash(const char *path);
EXPORTS unsigned int	ext_archive_length_hash(Archive *file, unsigned int id);
EXPORTS int				ext_archive_exist_hash(Archive *file, unsigned int id);
EXPORTS unsigned int	ext_archive_read_hash(Archive *file, unsigned int id, void *output, unsigned int len);
