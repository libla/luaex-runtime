#ifndef __BASE64__
#define __BASE64__

/**
	@file
	@date		2012-08-27 09:04
	@author		libla
	@version	1.0
	@brief		
	
 */

#include "config.h"

EXPORTS unsigned int ext_base64_encode(char *output, const void *input, unsigned int length);
EXPORTS unsigned int ext_base64_decode(void *output, const char *input, unsigned int length);

#endif // __BASE64__
