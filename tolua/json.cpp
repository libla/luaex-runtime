#define UTIL_CORE
#include <cmath>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/memorystream.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"
#include "loaders.h"

using namespace rapidjson;

namespace
{
	class TableHandler
	{
	protected:
		lua_State *L;
		std::vector<bool> keymodes;

	protected:
		void Value()
		{
			size_t size = keymodes.size();
			if (size != 0)
			{
				if (keymodes[size - 1])
				{
					keymodes[size - 1] = false;
					lua_rawset(L, -3);
				}
				else
				{
					lua_getfield(L, -2, "n");
					int len = (int)lua_tonumber(L, -1) + 1;
					lua_pop(L, 1);
					lua_rawseti(L, -2, len);
					lua_pushnumber(L, len);
					lua_setfield(L, -2, "n");
				}
			}
		}

	public:
		TableHandler(lua_State *L)
		{
			this->L = L;
		}
		bool Null()
		{
			lua_pushnil(L);
			Value();
			return true;
		}
		bool Bool(bool b)
		{
			lua_pushboolean(L, b ? 1 : 0);
			Value();
			return true;
		}
		bool Int(int i)
		{
			lua_pushnumber(L, i);
			Value();
			return true;
		}
		bool Uint(unsigned u)
		{
			lua_pushnumber(L, u);
			Value();
			return true;
		}
		bool Double(double d)
		{
			lua_pushnumber(L, d);
			Value();
			return true;
		}
		bool String(const char* str, SizeType length, bool copy)
		{
			lua_pushlstring(L, str, length);
			Value();
			return true;
		}
		bool StartObject()
		{
			lua_newtable(L);
			keymodes.push_back(false);
			return true;
		}
		bool Key(const char* str, SizeType length, bool copy)
		{
			lua_pushlstring(L, str, length);
			size_t size = keymodes.size();
			if (size != 0)
			{
				keymodes[size - 1] = true;
			}
			return true;
		}
		bool EndObject(SizeType memberCount)
		{
			keymodes.pop_back();
			Value();
			return true;
		}
		bool StartArray()
		{
			lua_newtable(L);
			lua_pushnumber(L, 0);
			lua_setfield(L, -2, "n");
			keymodes.push_back(false);
			return true;
		}
		bool EndArray(SizeType elementCount)
		{
			keymodes.pop_back();
			lua_pushnil(L);
			lua_setfield(L, -2, "n");
			Value();
			return true;
		}
	};
}

namespace
{
	struct EscapeUTF8 : public UTF8<>
	{
		enum { supportUnicode = 0 };
	};

	template <class Writer>
	bool WriteValue(lua_State *L, int idx, int filled, Writer &writer)
	{
		switch (lua_type(L, idx))
		{
		case LUA_TNIL:
			if (!writer.Null())
			{
				lua_pushstring(L, GetParseError_En(kParseErrorTermination));
				return false;
			}
			break;
		case LUA_TBOOLEAN:
			if (!writer.Bool(lua_toboolean(L, idx) != 0))
			{
				lua_pushstring(L, GetParseError_En(kParseErrorTermination));
				return false;
			}
			break;
		case LUA_TNUMBER:
			{
				bool result = false;
				double num = lua_tonumber(L, idx);
				double intnum = floor(num + 0.5);
				if (abs(intnum - num) < DBL_EPSILON && intnum <= UINT_MAX && intnum >= INT_MIN)
				{
					if (num >= 0)
					{
						result = writer.Uint((unsigned int)intnum);
					}
					else
					{
						result = writer.Int((int)intnum);
					}
				}
				else
				{
					result = writer.Double(num);
				}
				if (!result)
				{
					lua_pushstring(L, GetParseError_En(kParseErrorTermination));
					return false;
				}
			}
			break;
		case LUA_TSTRING:
			{
				size_t len;
				const char *str = lua_tolstring(L, idx, &len);
				if (!writer.String(str, len, true))
				{
					lua_pushstring(L, GetParseError_En(kParseErrorTermination));
					return false;
				}
			}
			break;
		case LUA_TTABLE:
			{
				lua_pushvalue(L, idx);
				lua_gettable(L, filled);
				if (!lua_isnil(L, -1))
				{
					lua_pushstring(L, "Cycle table.");
					return false;
				}
				lua_pop(L, 1);
				lua_pushvalue(L, idx);
				lua_pushboolean(L, 1);
				lua_settable(L, filled);
				if (lua_objlen(L, idx) == 0)
				{
					if (!writer.StartObject())
					{
						lua_pushstring(L, GetParseError_En(kParseErrorTermination));
						return false;
					}
					int count = 0;
					lua_pushnil(L);
					while (lua_next(L, idx) != 0)
					{
						if (lua_type(L, -2) == LUA_TSTRING)
						{
							++count;
							size_t len;
							const char *str = lua_tolstring(L, -2, &len);
							writer.Key(str, len, true);
							if (!WriteValue(L, lua_gettop(L), filled, writer))
							{
								return false;
							}
						}
						lua_pop(L, 1);
					}
					if (!writer.EndObject((SizeType)count))
					{
						lua_pushstring(L, GetParseError_En(kParseErrorTermination));
						return false;
					}
				}
				else
				{
					if (!writer.StartArray())
					{
						lua_pushstring(L, GetParseError_En(kParseErrorTermination));
						return false;
					}
					size_t count = lua_objlen(L, idx);
					for (size_t i = 1; i <= count; ++i)
					{
						lua_pushnumber(L, i);
						lua_gettable(L, idx);
						if (!WriteValue(L, lua_gettop(L), filled, writer))
						{
							return false;
						}
						lua_pop(L, 1);
					}
					if (!writer.EndArray((SizeType)count))
					{
						lua_pushstring(L, GetParseError_En(kParseErrorTermination));
						return false;
					}
				}
			}
			break;
		default:
			lua_pushfstring(L, "Unexpected '%s'", luaL_typename(L, idx));
			return false;
		}
		return true;
	}
}

namespace external
{
	static int parse_tolua(lua_State *L)
	{
		size_t len;
		const char *content = luaL_checklstring(L, 1, &len);
		MemoryStream mstream(content, len);
		Reader reader;
		int top = lua_gettop(L);
        TableHandler handle = TableHandler(L);
		reader.Parse(mstream, handle);
		if (reader.HasParseError())
		{
			lua_settop(L, top);
			lua_pushnil(L);
			int row = 1, column = 1;
			for (size_t i = 0, j = reader.GetErrorOffset(); i <= j; ++i)
			{
				if (content[i] == '\n')
				{
					++row;
					column= 1;
				}
				else
				{
					++column;
				}
			}
			lua_pushfstring(L, "(%d, %d):%s", row, column, GetParseError_En(reader.GetParseErrorCode()));
			return 2;
		}
		return 1;
	}

	static int print_tolua(lua_State *L)
	{
		luaL_checkany(L, 1);
		bool pretty = lua_toboolean(L, 2) != 0;
		bool escape = lua_toboolean(L, 3) != 0;
		lua_newtable(L);
		StringBuffer buffer;
		if (pretty)
		{
			if (escape)
			{
				PrettyWriter<StringBuffer, UTF8<>, EscapeUTF8> writer(buffer);
				if (!WriteValue(L, 1, lua_gettop(L), writer))
				{
					lua_pushnil(L);
					lua_insert(L, -2);
					return 2;
				}
			}
			else
			{
				PrettyWriter<StringBuffer> writer(buffer);
				if (!WriteValue(L, 1, lua_gettop(L), writer))
				{
					lua_pushnil(L);
					lua_insert(L, -2);
					return 2;
				}
			}
		}
		else
		{
			if (escape)
			{
				Writer<StringBuffer, UTF8<>, EscapeUTF8> writer(buffer);
				if (!WriteValue(L, 1, lua_gettop(L), writer))
				{
					lua_pushnil(L);
					lua_insert(L, -2);
					return 2;
				}
			}
			else
			{
				Writer<StringBuffer> writer(buffer);
				if (!WriteValue(L, 1, lua_gettop(L), writer))
				{
					lua_pushnil(L);
					lua_insert(L, -2);
					return 2;
				}
			}
		}
		lua_pushlstring(L, buffer.GetString(), buffer.GetSize());
		return 1;
	}

	int json_tolua(lua_State *L)
	{
		static const luaL_Reg libs[] = {
			{"parse", parse_tolua},
			{"print", print_tolua},
			{NULL, NULL}
		};
		lua_pushvalue(L, LUA_ENVIRONINDEX);
		luaL_register(L, NULL, libs);
		return 1;
	}
}
