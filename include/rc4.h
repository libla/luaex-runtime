#ifndef __RC4__
#define __RC4__

/**
	@file
	@date		2012-08-27 09:04
	@author		libla
	@version	1.0
	@brief		
	
 */

#include <stdlib.h>
#include "config.h"

namespace external
{
	class rc4key;
}

using namespace external;

EXPORTS rc4key *	ext_rc4_create(const char *key, unsigned int len);
EXPORTS void		ext_rc4_destroy(rc4key *key);
EXPORTS void		ext_rc4_encode(rc4key *key, void *output, const void *input, unsigned int length);
EXPORTS void		ext_rc4_decode(rc4key *key, void *output, const void *input, unsigned int length);

#endif // __RC4__
