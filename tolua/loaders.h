#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"

namespace external
{
	int archive_tolua(lua_State *L);
	int encrypt_tolua(lua_State *L);
	int json_tolua(lua_State *L);
	int sqlite_tolua(lua_State *L);

	void * checkudata(lua_State *L, int idx, void *meta, const char *name);
}