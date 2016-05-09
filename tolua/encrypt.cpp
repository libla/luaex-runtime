#define UTIL_CORE
#include "md5.h"
#include "xxhash.h"
#include "base64.h"
#include "rc4.h"
#include "loaders.h"

namespace
{
	class mempool
	{
	public:
		void *ptr;
		size_t len;

		void * alloc(size_t l)
		{
			if (len < l)
			{
				free(ptr);
				len = l;
				ptr = malloc(len);
			}
			return ptr;
		}

		void reduce()
		{
			if (len > (1 << 20))
			{
				free(ptr);
				len = 0;
				ptr = NULL;
			}
		}
	};

	static int mempool_gc_tolua(lua_State *L)
	{
		mempool *pool = (mempool *)lua_touserdata(L, 1);
		free(pool->ptr);
		pool->ptr = NULL;
		return 0;
	}
}

namespace external
{
	static char rc4metakey;

	static int md5_tolua(lua_State *L)
	{
		size_t len;
		const char *input = luaL_checklstring(L, 1, &len);
		unsigned char output[16];
		ext_md5calc(output, input, (unsigned int)len);
		lua_pushlstring(L, (const char *)output, sizeof(output));
		return 1;
	}

	static int xxhash_tolua(lua_State *L)
	{
		size_t len;
		const char *input = luaL_checklstring(L, 1, &len);
		double seed = luaL_checknumber(L, 2);
		if (seed > 0 && seed < 1)
		{
			seed = seed * 0xffffffff + 0.5;
		}
		else if (seed < 0)
		{
			seed = 0;
		}
		else if (seed > UINT_MAX)
		{
			seed = UINT_MAX;
		}
		else
		{
			seed = seed + 0.5;
		}
		lua_pushnumber(L, XXH32(input, (int)len, (unsigned int)seed));
		return 1;
	}

	static int base64_encode_tolua(lua_State *L)
	{
		size_t len;
		const char *input = luaL_checklstring(L, 1, &len);
		unsigned int outlen = ext_base64_encode(NULL, input, (unsigned int)len);
		mempool *pool = (mempool *)lua_touserdata(L, lua_upvalueindex(1));
		char *output = (char *)pool->alloc(outlen);
		ext_base64_encode(output, input, (unsigned int)len);
		lua_pushlstring(L, output, outlen);
		pool->reduce();
		return 1;
	}

	static int base64_decode_tolua(lua_State *L)
	{
		size_t len;
		const char *input = luaL_checklstring(L, 1, &len);
		unsigned int outlen = ext_base64_decode(NULL, input, (unsigned int)len);
		mempool *pool = (mempool *)lua_touserdata(L, lua_upvalueindex(1));
		char *output = (char *)pool->alloc(outlen);
		ext_base64_decode(output, input, (unsigned int)len);
		lua_pushlstring(L, output, outlen);
		pool->reduce();
		return 1;
	}

	static int rc4_create_tolua(lua_State *L)
	{
		size_t len;
		const char *input = luaL_checklstring(L, 1, &len);
		rc4key **ptr = (rc4key **)lua_newuserdata(L, sizeof(rc4key *));
		lua_pushlightuserdata(L, &rc4metakey);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_setmetatable(L, -2);
		*ptr = ext_rc4_create(input, (unsigned int)len);
		return 1;
	}

	static int rc4_encode_tolua(lua_State *L)
	{
		rc4key **ptr = (rc4key **)checkudata(L, 1, &rc4metakey, "rc4");
		size_t len;
		const char *input = luaL_checklstring(L, 2, &len);
		if (len == 0)
		{
			lua_pushliteral(L, "");
		}
		else
		{
			mempool *pool = (mempool *)lua_touserdata(L, lua_upvalueindex(1));
			char *output = (char *)pool->alloc(len);
			ext_rc4_encode(*ptr, output, input, (unsigned int)len);
			lua_pushlstring(L, output, len);
			pool->reduce();
		}
		return 1;
	}

	static int rc4_decode_tolua(lua_State *L)
	{
		rc4key **ptr = (rc4key **)checkudata(L, 1, &rc4metakey, "rc4");
		size_t len;
		const char *input = luaL_checklstring(L, 2, &len);
		if (len == 0)
		{
			lua_pushliteral(L, "");
		}
		else
		{
			mempool *pool = (mempool *)lua_touserdata(L, lua_upvalueindex(1));
			char *output = (char *)pool->alloc(len);
			ext_rc4_decode(*ptr, output, input, (unsigned int)len);
			lua_pushlstring(L, output, len);
			pool->reduce();
		}
		return 1;
	}

	static int rc4_gc_tolua(lua_State *L)
	{
		rc4key **ptr = (rc4key **)lua_touserdata(L, 1);
		ext_rc4_destroy(*ptr);
		return 0;
	}

	int encrypt_tolua(lua_State *L)
	{
		static const luaL_Reg libs[] = {
			{"md5", md5_tolua},
			{"xxhash", xxhash_tolua},
			{NULL, NULL}
		};
		static const luaL_Reg base64libs[] = {
			{"encode", base64_encode_tolua},
			{"decode", base64_decode_tolua},
			{NULL, NULL}
		};
		static const luaL_Reg rc4libs[] = {
			{"create", rc4_create_tolua},
			{"encode", rc4_encode_tolua},
			{"decode", rc4_decode_tolua},
			{NULL, NULL}
		};
		static const luaL_Reg rc4metas[] = {
			{"__gc", rc4_gc_tolua},
			{NULL, NULL}
		};
		mempool *pool = (mempool *)lua_newuserdata(L, sizeof(mempool));
		pool->ptr = NULL;
		pool->len = 0;
		int ipool = lua_gettop(L);
		lua_createtable(L, 0, 1);
		lua_pushcfunction(L, mempool_gc_tolua);
		lua_setfield(L, -2, "__gc");
		lua_setmetatable(L, -2);

		lua_pushvalue(L, LUA_ENVIRONINDEX);
		luaL_register(L, NULL, libs);
		lua_createtable(L, 0, sizeof(base64libs));
		lua_pushvalue(L, ipool);
		luaI_openlib(L, NULL, base64libs, 1);
		lua_setfield(L, -2, "base64");
		lua_createtable(L, 0, sizeof(rc4libs));
		lua_pushvalue(L, ipool);
		luaI_openlib(L, NULL, rc4libs, 1);
		lua_createtable(L, 0, sizeof(rc4metas));
		luaL_register(L, NULL, rc4metas);
		lua_pushvalue(L, -2);
		lua_setfield(L, -2, "__index");
		lua_pushlightuserdata(L, &rc4metakey);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
		lua_pop(L, 1);
		lua_setfield(L, -2, "rc4");
		return 1;
	}
}
