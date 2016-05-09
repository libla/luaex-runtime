#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include "config.h"

namespace external
{
	class JSONWriter;
}

using namespace external;

EXPORTS int ext_json_parse(const char *content, unsigned int len, void *ud, int (*Null)(void *), int (*Bool)(void *, int), int (*Int)(void *, int), int (*Uint)(void *, unsigned int), int (*Double)(void *, double), int (*String)(void *, const char *, unsigned int), int (*StartObject)(void *), int (*Key)(void *, const char *, unsigned int), int (*EndObject)(void *, unsigned int), int (*StartArray)(void *), int (*EndArray)(void *, unsigned int), void (*Error)(void *, int, int, const char *, unsigned int));
EXPORTS JSONWriter *	ext_json_writer(int pretty, int escape);
EXPORTS void			ext_json_writer_close(JSONWriter *writer);
EXPORTS void			ext_json_writer_reset(JSONWriter *writer);
EXPORTS unsigned int	ext_json_writer_length(JSONWriter *writer);
EXPORTS unsigned int	ext_json_writer_flush(JSONWriter *writer, char *output, unsigned int len);
EXPORTS int				ext_json_write_null(JSONWriter *writer);
EXPORTS int				ext_json_write_bool(JSONWriter *writer, int b);
EXPORTS int				ext_json_write_int(JSONWriter *writer, int i);
EXPORTS int				ext_json_write_uint(JSONWriter *writer, unsigned int u);
EXPORTS int				ext_json_write_double(JSONWriter *writer, double d);
EXPORTS int				ext_json_write_string(JSONWriter *writer, const char *str, unsigned int len);
EXPORTS int				ext_json_write_newobject(JSONWriter *writer);
EXPORTS int				ext_json_write_key(JSONWriter *writer, const char *str, unsigned int len);
EXPORTS int				ext_json_write_closeobject(JSONWriter *writer, unsigned int count);
EXPORTS int				ext_json_write_newarray(JSONWriter *writer);
EXPORTS int				ext_json_write_closearray(JSONWriter *writer, unsigned int count);
