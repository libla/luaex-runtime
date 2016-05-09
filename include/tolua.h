#ifndef __TOLUA__
#define __TOLUA__

/**
	@file
	@date		2012-08-27 09:04
	@author		libla
	@version	1.0
	@brief		
	
 */

#include <stdlib.h>
#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include "config.h"

EXPORTS void ext_loadlualibs(lua_State *L);

#endif // __TOLUA__
