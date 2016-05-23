#define UTIL_CORE
#include "luaex.h"

static char err_key;

// type
// class
// class meta
// class meta get
// class meta set
// instance meta
// instance meta get
// instance meta set

static char parent;
static char classtable;
static char classget;
static char classset;
static char instancemeta;
static char instanceget;
static char instanceset;

static int luaEX_cfunction(lua_State *L)
{
	lua_CFunction fn = (lua_CFunction)lua_touserdata(L, lua_upvalueindex(1));
	int result = fn(L);
	lua_pushlightuserdata(L, &err_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (lua_toboolean(L, -1))
	{
		lua_pop(L, 1);
		lua_pushlightuserdata(L, &err_key);
		lua_pushboolean(L, 0);
		lua_settable(L, LUA_REGISTRYINDEX);
		return luaL_error(L, "%s", lua_tostring(L, -1));
	}
	lua_pop(L, 1);
	return result;
}

static int luaEX_closure(lua_State *L)
{
	int top = lua_gettop(L);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_insert(L, 1);
	lua_call(L, top, LUA_MULTRET);
	lua_pushlightuserdata(L, &err_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (lua_toboolean(L, -1))
	{
		lua_pop(L, 1);
		lua_pushlightuserdata(L, &err_key);
		lua_pushboolean(L, 0);
		lua_settable(L, LUA_REGISTRYINDEX);
		return luaL_error(L, "%s", lua_tostring(L, -1));
	}
	lua_pop(L, 1);
	return lua_gettop(L);
}

static void gettable(lua_State *L, void *key)
{
	lua_pushlightuserdata(L, key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		lua_createtable(L, 0, 0);
		lua_pushlightuserdata(L, key);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	}
}

void luaEX_pushcfunction(lua_State *L, lua_CFunction fn)
{
	lua_pushlightuserdata(L, fn);
	lua_pushcclosure(L, luaEX_cfunction, 1);
}

void luaEX_pushcclosure(lua_State *L, lua_CFunction fn, int n)
{
	lua_pushcclosure(L, fn, n);
	lua_pushcclosure(L, luaEX_closure, 1);
}

int luaEX_pushuserdata(lua_State *L, void *ptr)
{
	static char key;
	lua_pushlightuserdata(L, &key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
		lua_createtable(L, 0, 1);
		lua_pushliteral(L, "v");
		lua_setfield(L, -2, "__mode");
		lua_setmetatable(L, -2);
		lua_pushlightuserdata(L, &key);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -2);
	if (!lua_isnil(L, -1))
	{
		lua_replace(L, -2);
		return 0;
	}
	lua_pop(L, 1);
	void **ud = (void **)lua_newuserdata(L, sizeof(void *));
	*ud = ptr;
	lua_pushlightuserdata(L, ptr);
	lua_pushvalue(L, -2);
	lua_settable(L, -4);
	lua_replace(L, -2);
	return 1;
}

int luaEX_getmetatable(lua_State *L, void *ptr)
{
	gettable(L, &instancemeta);
	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -2);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 2);
		return 0;
	}
	lua_replace(L, -2);
	return 1;
}

int luaEX_error(lua_State *L, const char *msg, int len)
{
	lua_pushlightuserdata(L, &err_key);
	lua_pushboolean(L, 1);
	lua_settable(L, LUA_REGISTRYINDEX);
	if (len >= 0)
		lua_pushlstring(L, msg, len);
	return 1;
}

static int class_get(lua_State *L)
{
	lua_pushvalue(L, 2);					// key
	lua_rawget(L, lua_upvalueindex(2));	// getkey
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		gettable(L, &parent);				// parenttable
		gettable(L, &classtable);			// parenttable classtable
		gettable(L, &classget);				// parenttable classtable classgettable
		lua_pushvalue(L, lua_upvalueindex(1));	// parenttable classtable classgettable type
		while (true)
		{
			lua_rawget(L, -4);				// parenttable classtable classgettable basetype
			if (lua_isnil(L, -1))
			{
				return 1;
			}
			lua_pushvalue(L, -1);
			lua_rawget(L, -4);				// parenttable classtable classgettable basetype class
			if (!lua_isnil(L, -1))
			{
				lua_pushvalue(L, 2);
				lua_rawget(L, -2);			// parenttable classtable classgettable basetype class getkey
				if (!lua_isnil(L, -1))
				{
					lua_pushvalue(L, 2);
					lua_pushvalue(L, -2);
					lua_rawset(L, 1);
					return 1;
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			lua_pushvalue(L, -1);
			lua_rawget(L, -3);				// parenttable classtable classgettable basetype classget
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				continue;
			}
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);				// parenttable classtable classgettable basetype classget getkey
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 2);
				continue;
			}
			lua_replace(L, -6);					// getkey classtable classgettable basetype classget
			lua_pop(L, 4);						// getkey
			break;
		}
	}
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	return 1;
}

static int class_set(lua_State *L)
{
	lua_pushvalue(L, 2);					// key
	lua_rawget(L, lua_upvalueindex(2));		// setkey
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		gettable(L, &parent);				// parenttable
		gettable(L, &classset);				// parenttable classsettable
		lua_pushvalue(L, lua_upvalueindex(1));	// parenttable classsettable type
		while (true)
		{
			lua_rawget(L, -3);				// parenttable classsettable basetype
			if (lua_isnil(L, -1))
			{
				return luaL_error(L, "field or property '%s' does not exist", lua_tostring(L, 2));
			}
			lua_pushvalue(L, -1);
			lua_rawget(L, -3);				// parenttable classsettable basetype classset
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				continue;
			}
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);				// parenttable classsettable basetype classset setkey
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 2);
				continue;
			}
			lua_replace(L, -5);					// setkey classsettable basetype classset
			lua_pop(L, 3);						// setkey
			break;
		}
	}
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 3);
	lua_call(L, 2, 0);
	return 0;
}

static int instance_get(lua_State *L)
{
	lua_pushvalue(L, 2);					// key
	lua_rawget(L, lua_upvalueindex(2));		// getkey
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		gettable(L, &parent);				// parenttable
		gettable(L, &instanceget);				// parenttable instancegettable
		lua_pushvalue(L, lua_upvalueindex(1));	// parenttable instancegettable type
		while (true)
		{
			lua_rawget(L, -3);				// parenttable instancegettable basetype
			if (lua_isnil(L, -1))
			{
				lua_pushvalue(L, 2);
				lua_gettable(L, lua_upvalueindex(3));
				return 1;
			}
			lua_pushvalue(L, -1);
			lua_rawget(L, -3);				// parenttable instancegettable basetype instanceget
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				continue;
			}
			lua_pushvalue(L, 2);
			lua_gettable(L, -2);				// parenttable instancegettable basetype instanceget getkey
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 2);
				continue;
			}
			lua_replace(L, -5);					// getkey instancegettable basetype instanceget
			lua_pop(L, 3);						// getkey
			break;
		}
	}
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	return 1;
}

static int instance_set(lua_State *L)
{
	lua_pushvalue(L, 2);					// key
	lua_rawget(L, lua_upvalueindex(2));		// setkey
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		gettable(L, &parent);				// parenttable
		gettable(L, &instanceset);				// parenttable instancesettable
		lua_pushvalue(L, lua_upvalueindex(1));	// parenttable instancesettable type
		while (true)
		{
			lua_rawget(L, -3);				// parenttable instancesettable basetype
			if (lua_isnil(L, -1))
			{
				lua_pushvalue(L, 2);
				lua_pushvalue(L, 3);
				lua_settable(L, lua_upvalueindex(3));
				return 0;
			}
			lua_pushvalue(L, -1);
			lua_rawget(L, -3);				// parenttable instancesettable basetype instanceset
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				continue;
			}
			lua_pushvalue(L, 2);
			lua_gettable(L, -2);				// parenttable instancesettable basetype instanceset setkey
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 2);
				continue;
			}
			lua_replace(L, -5);					// setkey instancesettable basetype instanceset
			lua_pop(L, 3);						// setkey
			break;
		}
	}
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 3);
	lua_call(L, 2, 0);
	return 0;
}

void luaEX_newtype(lua_State *L, const char *name, void *type)
{
	lua_pushlightuserdata(L, type);
	if (name && name[0] != '\0')
	{
		luaL_findtable(L, LUA_GLOBALSINDEX, name, 0);
		lua_settop(L, lua_gettop(L) + 6);
	}
	else
	{
		lua_settop(L, lua_gettop(L) + 7);
	}
}

void luaEX_basetype(lua_State *L, void *parenttype)
{
	int index = lua_gettop(L) - 7;
	gettable(L, &parent);
	lua_pushvalue(L, index);
	lua_pushlightuserdata(L, parenttype);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

void luaEX_nexttype(lua_State *L)
{
	int index = lua_gettop(L) - 7;
	if (lua_istable(L, index + 3) || lua_istable(L, index + 4))
	{
		if (!lua_istable(L, index + 1))
		{
			lua_newtable(L);
			lua_replace(L, index + 1);
		}
		if (!lua_istable(L, index + 2))
		{
			lua_newtable(L);
			lua_replace(L, index + 2);
		}
		lua_pushvalue(L, index + 2);
		lua_setmetatable(L, index + 1);
		if (lua_istable(L, index + 3))
		{
			lua_pushvalue(L, index);
			lua_pushvalue(L, index + 3);
			lua_pushcclosure(L, class_get, 2);
			lua_setfield(L, index + 2, "__index");
		}
		if (lua_istable(L, index + 4))
		{
			lua_pushvalue(L, index);
			lua_pushvalue(L, index + 4);
			lua_pushcclosure(L, class_set, 2);
			lua_setfield(L, index + 2, "__newindex");
		}

		lua_pushvalue(L, index);
		lua_setfield(L, index + 2, "__type");
	}

	if (lua_istable(L, index + 5))
	{
		if (lua_istable(L, index + 6) || lua_istable(L, index + 1))
		{
			lua_pushvalue(L, index);
			lua_pushvalue(L, index + 6);
			lua_pushvalue(L, index + 1);
			lua_pushcclosure(L, instance_get, 3);
			lua_setfield(L, index + 5, "__index");
		}
		if (lua_istable(L, index + 7))
		{
			lua_pushvalue(L, index);
			lua_pushvalue(L, index + 7);
			lua_pushvalue(L, index + 1);
			lua_pushcclosure(L, instance_set, 3);
			lua_setfield(L, index + 5, "__newindex");
		}
	}

	if (lua_istable(L, index + 1))
	{
		gettable(L, &classtable);
		lua_pushvalue(L, index);
		lua_pushvalue(L, index + 1);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	if (lua_istable(L, index + 3))
	{
		gettable(L, &classget);
		lua_pushvalue(L, index);
		lua_pushvalue(L, index + 3);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	if (lua_istable(L, index + 4))
	{
		gettable(L, &classset);
		lua_pushvalue(L, index);
		lua_pushvalue(L, index + 4);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	if (lua_istable(L, index + 5))
	{
		gettable(L, &parent);
		lua_pushvalue(L, index);
		lua_rawget(L, -2);
		if (!lua_isnil(L, -1))
		{
			gettable(L, &instancemeta);
			lua_replace(L, -3);
			lua_rawget(L, -2);
			if (!lua_isnil(L, -1))
			{
				static const char *metas[] = {
					"__add",
					"__sub",
					"__mul",
					"__div",
					"__mod",
					"__eq",
					"__lt",
					"__le",
					"__unm",
				};
				for (int i = 0; i < sizeof(metas) / sizeof(metas[0]); ++i)
				{
					lua_pushstring(L, metas[i]);
					lua_pushvalue(L, -1);
					lua_rawget(L, index + 5);
					if (lua_isnil(L, -1))
					{
						lua_pop(L, 1);
						lua_pushvalue(L, -1);
						lua_rawget(L, -3);
						if (!lua_isnil(L, -1))
						{
							lua_rawset(L, index + 5);
							continue;
						}
					}
					lua_pop(L, 2);
				}
			}
			lua_pop(L, 2);
		}
		lua_pop(L, 2);
		gettable(L, &instancemeta);
		lua_pushvalue(L, index);
		lua_pushvalue(L, index + 5);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	if (lua_istable(L, index + 6))
	{
		gettable(L, &instanceget);
		lua_pushvalue(L, index);
		lua_pushvalue(L, index + 6);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	if (lua_istable(L, index + 7))
	{
		gettable(L, &instanceset);
		lua_pushvalue(L, index);
		lua_pushvalue(L, index + 7);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	lua_settop(L, index - 1);
}

void luaEX_construct(lua_State *L, lua_CFunction fn)
{
	int index = lua_gettop(L) - 7;
	if (!lua_istable(L, index + 2))
	{
		lua_newtable(L);
		lua_replace(L, index + 2);
	}
	lua_pushliteral(L, "__call");
	luaEX_pushcfunction(L, fn);
	lua_rawset(L, index + 2);
}

void luaEX_method(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	int index = lua_gettop(L) - 7;
	if (!lua_istable(L, index + 1))
	{
		lua_newtable(L);
		lua_replace(L, index + 1);
	}
	lua_pushlstring(L, name, len);
	luaEX_pushcfunction(L, fn);
	lua_rawset(L, index + 1);
}

void luaEX_index(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	luaEX_method(L, "get_Item", sizeof("get_Item") - 1, fn);
}

void luaEX_newindex(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	luaEX_method(L, "set_Item", sizeof("set_Item") - 1, fn);
}

void luaEX_setter(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	int index = lua_gettop(L) - 7;
	if (!lua_istable(L, index + 7))
	{
		lua_newtable(L);
		lua_replace(L, index + 7);
	}
	lua_pushlstring(L, name, len);
	luaEX_pushcfunction(L, fn);
	lua_rawset(L, index + 7);
}

void luaEX_getter(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	int index = lua_gettop(L) - 7;
	if (!lua_istable(L, index + 6))
	{
		lua_newtable(L);
		lua_replace(L, index + 6);
	}
	lua_pushlstring(L, name, len);
	luaEX_pushcfunction(L, fn);
	lua_rawset(L, index + 6);
}

void luaEX_value(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	int index = lua_gettop(L) - 7;
	if (!lua_istable(L, index + 3))
	{
		lua_newtable(L);
		lua_replace(L, index + 3);
	}
	lua_pushlstring(L, name, len);
	luaEX_pushcfunction(L, fn);
	lua_rawset(L, index + 3);
}

void luaEX_newvalue(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	int index = lua_gettop(L) - 7;
	if (!lua_istable(L, index + 4))
	{
		lua_newtable(L);
		lua_replace(L, index + 4);
	}
	lua_pushlstring(L, name, len);
	luaEX_pushcfunction(L, fn);
	lua_rawset(L, index + 4);
}

void luaEX_opt(lua_State *L, const char *name, unsigned int len, lua_CFunction fn)
{
	int index = lua_gettop(L) - 7;
	if (!lua_istable(L, index + 5))
	{
		lua_newtable(L);
		lua_replace(L, index + 5);
	}
	lua_pushlstring(L, name, len);
	luaEX_pushcfunction(L, fn);
	lua_rawset(L, index + 5);
}
