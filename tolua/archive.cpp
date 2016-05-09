#define UTIL_CORE
#include <cstdlib>
#include "filesys.h"
#include "archive.h"
#include "loaders.h"

namespace external
{
	static char metakey;

	static int open_tolua(lua_State *L)
	{
		const char *path = luaL_checkstring(L, 1);
		Archive **ptr = (Archive **)lua_newuserdata(L, sizeof(Archive *));
		lua_pushlightuserdata(L, &metakey);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_setmetatable(L, -2);
		*ptr = ext_archive_open(path);
		return 1;
	}

	static int close_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)checkudata(L, 1, &metakey, "archive");
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		ext_archive_close(*ptr);
		*ptr = NULL;
		return 0;
	}

	static int status_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)checkudata(L, 1, &metakey, "archive");
		lua_pushboolean(L, *ptr != NULL ? 1 : 0);
		return 1;
	}

	static int length_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)checkudata(L, 1, &metakey, "archive");
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		const char *path = luaL_checkstring(L, 2);
		lua_pushnumber(L, ext_archive_length(*ptr, path));
		return 1;
	}

	static int exist_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)checkudata(L, 1, &metakey, "archive");
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		const char *path = luaL_checkstring(L, 2);
		lua_pushboolean(L, ext_archive_exist(*ptr, path));
		return 1;
	}

	static int files_iter_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)lua_touserdata(L, lua_upvalueindex(1));
		unsigned int index = (unsigned int)lua_tointeger(L, lua_upvalueindex(2));
		unsigned int total = (unsigned int)lua_tointeger(L, lua_upvalueindex(3));
		if (index >= total)
		{
			return 0;
		}
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		lua_pushnumber(L, index + 1);
		lua_replace(L, lua_upvalueindex(2));
		unsigned int len;
		const char *name = ext_archive_file(*ptr, index, &len);
		lua_pushlstring(L, name, len);
		return 1;
	}

	static int files_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)checkudata(L, 1, &metakey, "archive");
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		lua_pushvalue(L, 1);
		lua_pushnumber(L, 0);
		lua_pushnumber(L, ext_archive_files(*ptr));
		lua_pushcclosure(L, files_iter_tolua, 3);
		return 1;
	}

	static int read_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)checkudata(L, 1, &metakey, "archive");
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		const char *path = luaL_checkstring(L, 2);
		unsigned int len = ext_archive_length(*ptr, path);
		void *output = malloc(len);
		len = ext_archive_read(*ptr, path, output, len);
		lua_pushlstring(L, (const char *)output, len);
		free(output);
		return 1;
	}

	static int write_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)checkudata(L, 1, &metakey, "archive");
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		const char *path = luaL_checkstring(L, 2);
		size_t len;
		const char *input = luaL_checklstring(L, 3, &len);
		ext_archive_write(*ptr, path, input, (unsigned int)len);
		return 0;
	}
	
	static int gc_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)lua_touserdata(L, 1);
		if (*ptr != NULL)
		{
			ext_archive_close(*ptr);
		}
		return 0;
	}

	static int len_tolua(lua_State *L)
	{
		Archive **ptr = (Archive **)lua_touserdata(L, 1);
		if (*ptr == NULL)
		{
			luaL_error(L, "attempt to use a closed archive");
			return 0;
		}
		lua_pushnumber(L, ext_archive_files(*ptr));
		return 1;
	}

	int archive_tolua(lua_State *L)
	{
		static const luaL_Reg libs[] = {
			{"open", open_tolua},
			{"close", close_tolua},
			{"status", status_tolua},
			{"length", length_tolua},
			{"exist", exist_tolua},
			{"files", files_tolua},
			{"read", read_tolua},
			{"write", write_tolua},
			{NULL, NULL}
		};
		static const luaL_Reg metas[] = {
			{"__gc", gc_tolua},
			{"__len", len_tolua},
			{NULL, NULL}
		};
		lua_pushvalue(L, LUA_ENVIRONINDEX);
		luaL_register(L, NULL, libs);
		lua_createtable(L, 0, sizeof(metas));
		luaL_register(L, NULL, metas);
		lua_pushvalue(L, -2);
		lua_setfield(L, -2, "__index");
		lua_pushlightuserdata(L, &metakey);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
		lua_pop(L, 1);
		return 1;
	}
}
