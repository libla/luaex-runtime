#define UTIL_CORE
#include "tolua.h"
#include "loaders.h"

namespace external
{
	void * checkudata(lua_State *L, int idx, void *meta, const char *name)
	{
		void *ptr = lua_touserdata(L, idx);
		if (ptr != NULL)
		{
			if (lua_getmetatable(L, idx))
			{
				lua_pushlightuserdata(L, meta);
				lua_gettable(L, LUA_REGISTRYINDEX);
				if (lua_rawequal(L, -1, -2))
				{
					lua_pop(L, 2);
					return ptr;
				}
				lua_pop(L, 2);
			}
		}
		luaL_typerror(L, idx, name);
		return NULL;
	}
}

using namespace external;

void ext_loadlualibs(lua_State *L)
{
	static const luaL_Reg loads[] = {
		{"archive",	archive_tolua},
		{"encrypt",	encrypt_tolua},
		{"sqlite",	sqlite_tolua},
		{"json",	json_tolua},
		{NULL, NULL}
	};
	luaL_findtable(L, LUA_REGISTRYINDEX, "_PRELOAD", 0);
	for (size_t i = 0; i < sizeof(loads) / sizeof(loads[0]) - 1; ++i)
	{
		lua_pushcfunction(L, loads[i].func);
		lua_setfield(L, -2, loads[i].name);
	}
	lua_pop(L, 1);
}
