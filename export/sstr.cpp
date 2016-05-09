#define UTIL_CORE
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include "sstr.h"

namespace external
{
	class sstr
	{
	public:
		int length;
		char buffer[1];
	};
}

sstr * ext_sstr_new(const char *str, int len)
{
	if (len < 0)
	{
		len = (int)strlen(str);
	}
	sstr *result = (sstr *)malloc(offsetof(sstr, buffer) + len + 1);
	result->length = len;
	memcpy(result->buffer, str, len);
	result->buffer[len] = 0;
	return result;
}

void ext_sstr_delete(sstr *str)
{
	delete str;
}

const char * ext_sstr_read(sstr *str, int *len)
{
	if (len != NULL)
		*len = str->length;
	return str->buffer;
}
