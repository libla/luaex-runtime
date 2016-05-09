#ifndef __MD5__
#define __MD5__

/**
	@file
	@date		2012-08-27 09:04
	@author		libla
	@version	1.0
	@brief		
	
 */

#include <stdlib.h>
#include "config.h"

EXPORTS void ext_md5calc(unsigned char output[16], const void *input, unsigned int length);

#endif // __MD5__
