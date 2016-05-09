#ifndef __SSTR__
#define __SSTR__

#include "config.h"

namespace external
{
	class sstr;
}

using namespace external;

EXPORTS sstr * ext_sstr_new(const char *str, int len);
EXPORTS void ext_sstr_delete(sstr *str);
EXPORTS const char * ext_sstr_read(sstr *str, int *len);

#endif // __SSTR__
