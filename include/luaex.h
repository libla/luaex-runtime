#ifndef __LUAEX__
#define __LUAEX__

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

EXPORTS void luaEX_pushcfunction(lua_State *L, lua_CFunction fn);
EXPORTS void luaEX_pushcclosure(lua_State *L, lua_CFunction fn, int n);
EXPORTS int luaEX_pushuserdata(lua_State *L, void *ptr);
EXPORTS int luaEX_getmetatable(lua_State *L, void *ptr);
EXPORTS int luaEX_error(lua_State *L, const char *msg, int len);

EXPORTS void luaEX_newtype(lua_State *L, const char *name, void *type);
EXPORTS void luaEX_basetype(lua_State *L, void *parenttype);
EXPORTS void luaEX_nexttype(lua_State *L);
EXPORTS void luaEX_construct(lua_State *L, lua_CFunction fn);
EXPORTS void luaEX_method(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);
EXPORTS void luaEX_index(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);
EXPORTS void luaEX_newindex(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);
EXPORTS void luaEX_setter(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);
EXPORTS void luaEX_getter(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);
EXPORTS void luaEX_value(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);
EXPORTS void luaEX_newvalue(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);
EXPORTS void luaEX_opt(lua_State *L, const char *name, unsigned int len, lua_CFunction fn);

#endif // __LUAEX__
