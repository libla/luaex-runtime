#define UTIL_CORE
#include "sqlite3.h"
#include "loaders.h"
#include "config.h"
#include <set>
#include <vector>
#include <string>
#include <string.h>
#include <float.h>

namespace
{
	class SQLiteLuaStmt;

	class SQLiteLuaHandle
	{
	public:
		sqlite3 *sqlite;
		int refcount;
		std::set<SQLiteLuaStmt *> stmts;

	public:
		SQLiteLuaHandle();
		~SQLiteLuaHandle();

		void retain();
		void release();
		bool close();
	};

	class SQLiteLuaStmt
	{
	public:
		SQLiteLuaHandle *root;
		bool iseof;
		sqlite3_stmt *stmt;
		std::string nextsql;

	public:
		SQLiteLuaStmt(SQLiteLuaHandle *handle);
		~SQLiteLuaStmt();
	};

	SQLiteLuaHandle::SQLiteLuaHandle()
	{
		sqlite = NULL;
		refcount = 1;
	}

	SQLiteLuaHandle::~SQLiteLuaHandle()
	{
		close();
	}

	void SQLiteLuaHandle::retain()
	{
		++refcount;
	}

	void SQLiteLuaHandle::release()
	{
		if (--refcount == 0)
		{
			delete this;
		}
	}

	bool SQLiteLuaHandle::close()
	{
		if (sqlite != NULL)
		{
			for (std::set<SQLiteLuaStmt *>::iterator i = stmts.begin(); i != stmts.end(); ++i)
			{
				sqlite3_finalize((*i)->stmt);
				(*i)->stmt = NULL;
				(*i)->root->release();
				(*i)->root = NULL;
			}
			stmts.clear();
			if (SQLITE_OK != sqlite3_close(sqlite))
			{
				return false;
			}
			sqlite = NULL;
			return true;
		}
		return true;
	}

	SQLiteLuaStmt::SQLiteLuaStmt(SQLiteLuaHandle *handle)
	{
		root = handle;
		root->retain();
		iseof = false;
		stmt = NULL;
		root->stmts.insert(this);
	}

	SQLiteLuaStmt::~SQLiteLuaStmt()
	{
		if (root != NULL)
		{
			std::set<SQLiteLuaStmt *>::iterator i = root->stmts.find(this);
			if (i != root->stmts.end())
			{
				root->stmts.erase(i);
			}
			sqlite3_finalize(stmt);
			root->release();
		}
	}

	struct sqlite_lua_callback_data
	{
		lua_State *L;
		int func;
		int table;
	};
	static int sqlite_lua_callback(void *pArg, int count, char **values, char **columns)
	{
		bool result = true;
		sqlite_lua_callback_data *data = (sqlite_lua_callback_data *)pArg;
		lua_State *L = data->L;
		int top = lua_gettop(L);
		if (data->func == 0)
		{
			if (lua_isnil(L, data->table))
			{
				lua_newtable(L);
				lua_createtable(L, count, 0);
				for (int i = 0; i < count; ++i)
				{
					lua_pushstring(L, columns[i]);
					lua_rawseti(L, -2, i + 1);
				}
				lua_rawseti(L, -2, 1);
				lua_replace(L, data->table);
			}
			lua_createtable(L, count, 0);
			for (int i = 0; i < count; ++i)
			{
				if (values[i] == NULL)
				{
					lua_pushnil(L);
				}
				else
				{
					lua_pushstring(L, values[i]);
				}
				lua_rawseti(L, -2, i + 1);
			}
			lua_rawseti(L, data->table, (int)lua_objlen(L, data->table) + 1);
		}
		else
		{
			lua_pushvalue(L, data->func);
			if (!lua_isnil(L, -1))
			{
				if (lua_isnil(L, data->table))
				{
					lua_createtable(L, 2, 0);
					lua_createtable(L, count, 0);
					for (int i = 0; i < count; ++i)
					{
						lua_pushstring(L, columns[i]);
						lua_rawseti(L, -2, i + 1);
					}
					lua_rawseti(L, -2, 1);
					lua_createtable(L, count, 0);
					lua_rawseti(L, -2, 2);
					lua_replace(L, data->table);
				}
				lua_rawgeti(L, data->table, 1);
				lua_rawgeti(L, data->table, 2);
				for (int i = 0; i < count; ++i)
				{
					if (values[i] == NULL)
					{
						lua_pushnil(L);
					}
					else
					{
						lua_pushstring(L, values[i]);
					}
					lua_rawseti(L, -2, i + 1);
				}
				lua_call(L, 2, 1);
				result = lua_isnil(L, -1) || lua_toboolean(L, -1) != 0;
			}
		}
		lua_settop(L, top);
		return result ? 0 : 1;
	}
}

namespace external
{
	static char sqlitemetakey;
	static char stmtmetakey;

	static int open_tolua(lua_State *L)
	{
		const char *path = luaL_checkstring(L, 1);
		const char *option = luaL_optstring(L, 2, "rw");
		bool readonly = strchr(option, 'w') == NULL;
		sqlite3 *sqlite = NULL;
		if (SQLITE_OK != sqlite3_open_v2(path, &sqlite, readonly ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0))
		{
			lua_pushnil(L);
			if (sqlite != NULL)
			{
				lua_pushstring(L, sqlite3_errmsg(sqlite));
				sqlite3_close(sqlite);
				return 2;
			}
			return 1;
		}
		SQLiteLuaHandle **ptr = (SQLiteLuaHandle **)lua_newuserdata(L, sizeof(SQLiteLuaHandle *));
		lua_pushlightuserdata(L, &sqlitemetakey);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_setmetatable(L, -2);
		SQLiteLuaHandle *handle = new SQLiteLuaHandle();
		*ptr = handle;
		handle->sqlite = sqlite;
		return 1;
	}

	static int close_tolua(lua_State *L)
	{
		SQLiteLuaHandle **ptr = (SQLiteLuaHandle **)checkudata(L, 1, &sqlitemetakey, "sqlite.db");
		SQLiteLuaHandle *handle = *ptr;
		if (!handle->close())
		{
			lua_pushboolean(L, 0);
			lua_pushstring(L, sqlite3_errmsg(handle->sqlite));
			return 2;
		}
		lua_pushboolean(L, 1);
		return 1;
	}

	static int execute_tolua(lua_State *L)
	{
		SQLiteLuaHandle **ptr = (SQLiteLuaHandle **)checkudata(L, 1, &sqlitemetakey, "sqlite.db");
		const char *sql = luaL_checkstring(L, 2);
		SQLiteLuaHandle *handle = *ptr;
		if (handle->sqlite == NULL)
		{
			lua_pushnil(L);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		sqlite_lua_callback_data data;
		data.L = L;
		if (lua_isnone(L, 3))
		{
			data.func = 0;
		}
		else
		{
			if (!lua_isnil(L, 3))
			{
				luaL_checktype(L, 3, LUA_TFUNCTION);
			}
			data.func = 3;
		}
		lua_pushnil(L);
		data.table = lua_gettop(L);
		char *errormsg = NULL;
		if (SQLITE_OK != sqlite3_exec(handle->sqlite, sql, sqlite_lua_callback, &data, &errormsg))
		{
			lua_pushboolean(L, 0);
			if (errormsg != NULL)
			{
				lua_pushstring(L, errormsg);
				return 2;
			}
			return 1;
		}
		if (data.func == 0)
		{
			if (lua_isnil(L, data.table))
			{
				lua_pushboolean(L, 1);
			}
			else
			{
				lua_pushvalue(L, data.table);
			}
		}
		else
		{
			lua_pushboolean(L, 1);
		}
		return 1;
	}

	static int prepare_tolua(lua_State *L)
	{
		SQLiteLuaHandle **ptr = (SQLiteLuaHandle **)checkudata(L, 1, &sqlitemetakey, "sqlite.db");
		const char *sql = luaL_checkstring(L, 2);
		SQLiteLuaHandle *handle = *ptr;
		if (handle->sqlite == NULL)
		{
			lua_pushnil(L);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		sqlite3_stmt *sqlite_stmt = NULL;
		const char *left_over;
		while (*sql)
		{
			while (isspace(*sql)) ++sql;
			int result = sqlite3_prepare(handle->sqlite, sql, -1, &sqlite_stmt, &left_over);
			if (result != SQLITE_OK)
			{
				lua_pushnil(L);
				lua_pushstring(L, sqlite3_errmsg(handle->sqlite));
				return 2;
			}
			if (sqlite_stmt != NULL)
				break;
			sql = left_over;
		}
		if (sqlite_stmt == NULL)
		{
			lua_pushnil(L);
			return 1;
		}
		SQLiteLuaStmt *stmt = (SQLiteLuaStmt *)lua_newuserdata(L, sizeof(SQLiteLuaStmt));
		lua_pushlightuserdata(L, &stmtmetakey);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_setmetatable(L, -2);
		new (stmt)SQLiteLuaStmt(handle);
		stmt->stmt = sqlite_stmt;
		if (left_over != NULL)
		{
			stmt->nextsql = left_over;
		}
		return 1;
	}

	static int status_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		if (ptr->stmt == NULL)
		{
			lua_pushliteral(L, "detached");
		}
		else if (ptr->iseof)
		{
			lua_pushliteral(L, "eof");
		}
		else
		{
			lua_pushliteral(L, "valid");
		}
		return 1;
	}

	static int column_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		if (ptr->stmt == NULL)
		{
			lua_pushnil(L);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		int col = sqlite3_column_count(ptr->stmt);
		lua_createtable(L, col, 0);
		for (int i = 0; i < col; ++i)
		{
			lua_pushstring(L, sqlite3_column_name(ptr->stmt, i));
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int step_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		if (ptr->stmt == NULL)
		{
			lua_pushnil(L);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		if (ptr->iseof)
		{
			lua_pushnil(L);
			lua_pushliteral(L, "attempt to step a halted statement");
			return 2;
		}
		int result = sqlite3_step(ptr->stmt);
		if (result == SQLITE_ROW)
		{
			int col = sqlite3_column_count(ptr->stmt);
			lua_createtable(L, col, 0);
			for (int i = 0; i < col; ++i)
			{
				switch (sqlite3_column_type(ptr->stmt, i))
				{
				case SQLITE_INTEGER:
					{
						i64 l = sqlite3_column_int64(ptr->stmt, i);
						if (l > UINT_MAX || l < INT_MIN)
						{
							char bytes[9];
							bytes[0] = 'L';
							for (int i = 1; i < 9; ++i)
							{
								bytes[9 - i] = (char)(l & 0xff);
								l >>= 8;
							}
							lua_pushlstring(L, bytes, 9);
						}
						else
						{
							lua_pushnumber(L, (double)l);
						}
					}
					break;
				case SQLITE_FLOAT:
					{
						double d = sqlite3_column_double(ptr->stmt, i);
						lua_pushnumber(L, d);
					}
					break;
				case SQLITE_TEXT:
					{
						const char *str = (const char *)sqlite3_column_text(ptr->stmt, i);
						if (str == NULL)
						{
							lua_pushnil(L);
						}
						else
						{
							lua_pushstring(L, str);
						}
					}
					break;
				case SQLITE_BLOB:
					{
						const char *blob = (const char *)sqlite3_column_blob(ptr->stmt, i);
						if (blob == NULL)
						{
							lua_pushnil(L);
						}
						else
						{
							int len = sqlite3_column_bytes(ptr->stmt, i);
							lua_pushlstring(L, blob, (size_t)len);
						}
					}
					break;
				default:
					lua_pushnil(L);
				}
				lua_rawseti(L, -2, i + 1);
			}
			return 1;
		}
		if (result == SQLITE_OK || result == SQLITE_DONE)
		{
			ptr->iseof = true;
			return 0;
		}
		lua_pushnil(L);
		lua_pushstring(L, sqlite3_errmsg(ptr->root->sqlite));
		return 2;
	}

	static int rows_empty(lua_State *L)
	{
		return 0;
	}

	static int rows_iter(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)lua_touserdata(L, lua_upvalueindex(1));
		int result = sqlite3_step(ptr->stmt);
		if (result == SQLITE_ROW)
		{
			int col = sqlite3_column_count(ptr->stmt);
			if (col == 0)
			{
				lua_rawset(L, lua_upvalueindex(3));
				return 1;
			}
			for (int i = 0; i < col; ++i)
			{
				lua_rawgeti(L, lua_upvalueindex(2), i + 1);
				switch (sqlite3_column_type(ptr->stmt, i))
				{
				case SQLITE_INTEGER:
					{
						i64 l = sqlite3_column_int64(ptr->stmt, i);
						if (l > UINT_MAX || l < INT_MIN)
						{
							char bytes[9];
							bytes[0] = 'L';
							for (int i = 1; i < 9; ++i)
							{
								bytes[9 - i] = (char)(l & 0xff);
								l >>= 8;
							}
							lua_pushlstring(L, bytes, sizeof(bytes));
						}
						else
						{
							lua_pushnumber(L, (double)l);
						}
					}
					break;
				case SQLITE_FLOAT:
					{
						double d = sqlite3_column_double(ptr->stmt, i);
						lua_pushnumber(L, d);
					}
					break;
				case SQLITE_TEXT:
					{
						const char *str = (const char *)sqlite3_column_text(ptr->stmt, i);
						if (str == NULL)
						{
							lua_pushnil(L);
						}
						else
						{
							lua_pushstring(L, str);
						}
					}
					break;
				case SQLITE_BLOB:
					{
						const char *blob = (const char *)sqlite3_column_blob(ptr->stmt, i);
						if (blob == NULL)
						{
							lua_pushnil(L);
						}
						else
						{
							int len = sqlite3_column_bytes(ptr->stmt, i);
							lua_pushlstring(L, blob, (size_t)len);
						}
					}
					break;
				default:
					lua_pushnil(L);
				}
				lua_rawset(L, lua_upvalueindex(3));
			}
			lua_pushvalue(L, lua_upvalueindex(3));
			return 1;
		}
		if (result == SQLITE_OK || result == SQLITE_DONE)
		{
			ptr->iseof = true;
			return 0;
		}
		return 0;
	}

	static int rows_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		if (ptr->stmt == NULL)
		{
			lua_pushcfunction(L, rows_empty);
			return 1;
		}
		sqlite3_reset(ptr->stmt);
		ptr->iseof = false;
		lua_pushvalue(L, 1);
		int col = sqlite3_column_count(ptr->stmt);
		if (col > 0)
		{
			lua_createtable(L, col, 0);
			for (int i = 0; i < col; ++i)
			{
				lua_pushstring(L, sqlite3_column_name(ptr->stmt, i));
				lua_rawseti(L, -2, i + 1);
			}
			lua_createtable(L, 0, col);
			lua_pushcclosure(L, rows_iter, 3);
		}
		else
		{
			lua_pushcclosure(L, rows_iter, 1);
		}
		return 1;
	}

	static int reset_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		if (ptr->stmt == NULL)
		{
			lua_pushboolean(L, 0);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		if (SQLITE_OK == sqlite3_reset(ptr->stmt))
		{
			ptr->iseof = false;
			lua_pushboolean(L, 1);
			return 1;
		}
		lua_pushboolean(L, 0);
		lua_pushstring(L, sqlite3_errmsg(ptr->root->sqlite));
		return 2;
	}

	static int clear_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		if (ptr->stmt == NULL)
		{
			lua_pushboolean(L, 0);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		if (SQLITE_OK == sqlite3_clear_bindings(ptr->stmt))
		{
			lua_pushboolean(L, 1);
			return 1;
		}
		lua_pushboolean(L, 0);
		lua_pushstring(L, sqlite3_errmsg(ptr->root->sqlite));
		return 2;
	}

	static int bind_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		int N = luaL_checkint(L, 2);
		if (ptr->stmt == NULL)
		{
			lua_pushboolean(L, 0);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		switch (lua_type(L, 2))
		{
		case LUA_TNONE:
		case LUA_TNIL:
			if (SQLITE_OK == sqlite3_bind_null(ptr->stmt, N))
			{
				lua_pushboolean(L, 1);
				return 1;
			}
			break;
		case LUA_TBOOLEAN:
			if (SQLITE_OK == sqlite3_bind_int(ptr->stmt, N, lua_toboolean(L, 3) == 0 ? 0 : 1))
			{
				lua_pushboolean(L, 1);
				return 1;
			}
			break;
		case LUA_TNUMBER:
			if (SQLITE_OK == sqlite3_bind_double(ptr->stmt, N, lua_tonumber(L, 3)))
			{
				lua_pushboolean(L, 1);
				return 1;
			}
			break;
		case LUA_TSTRING:
			{
				size_t len;
				const char *str = lua_tolstring(L, 3, &len);
				if (len == 9 && str[0] == 'L')
				{
					i64 l = 0;
					for (int i = 1; i < 9; ++i)
					{
						l = (l << 8) + str[i];
					}
					if (SQLITE_OK == sqlite3_bind_int64(ptr->stmt, N, l))
					{
						lua_pushboolean(L, 1);
						return 1;
					}
				}
				else
				{
					bool blob = len >= 65536;
					if (!blob)
					{
						for (size_t i = 0; i < len; ++i)
						{
							if (str[i] == 0 || (unsigned char)str[i] > 0xEF)
							{
								blob = true;
								break;
							}
						}
					}
					if (blob)
					{
						if (SQLITE_OK == sqlite3_bind_blob(ptr->stmt, N, str, (int)len, SQLITE_TRANSIENT))
						{
							lua_pushboolean(L, 1);
							return 1;
						}
					}
					else
					{
						if (SQLITE_OK == sqlite3_bind_text(ptr->stmt, N, str, (int)len, SQLITE_TRANSIENT))
						{
							lua_pushboolean(L, 1);
							return 1;
						}
					}
				}
			}
			break;
		default:
			luaL_argerror(L, 3, "only one of the following types(nil boolean number string)");
		}
		lua_pushboolean(L, 0);
		lua_pushstring(L, sqlite3_errmsg(ptr->root->sqlite));
		return 2;
	}

	static int next_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)checkudata(L, 1, &stmtmetakey, "sqlite.stmt");
		SQLiteLuaHandle *handle = ptr->root;
		const char *sql = ptr->nextsql.c_str();
		if (handle->sqlite == NULL)
		{
			lua_pushnil(L);
			lua_pushliteral(L, "closed database");
			return 2;
		}
		sqlite3_stmt *sqlite_stmt = NULL;
		const char *left_over;
		while (*sql)
		{
			while (isspace(*sql)) ++sql;
			int result = sqlite3_prepare(handle->sqlite, sql, -1, &sqlite_stmt, &left_over);
			if (result != SQLITE_OK)
			{
				lua_pushnil(L);
				lua_pushstring(L, sqlite3_errmsg(handle->sqlite));
				return 2;
			}
			if (sqlite_stmt != NULL)
				break;
			sql = left_over;
		}
		if (sqlite_stmt == NULL)
		{
			lua_pushnil(L);
			return 1;
		}
		SQLiteLuaStmt *stmt = (SQLiteLuaStmt *)lua_newuserdata(L, sizeof(SQLiteLuaStmt));
		lua_pushlightuserdata(L, &stmtmetakey);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_setmetatable(L, -2);
		new (stmt)SQLiteLuaStmt(handle);
		stmt->stmt = sqlite_stmt;
		if (left_over != NULL)
		{
			stmt->nextsql = left_over;
		}
		return 1;
	}

	static int gc_tolua(lua_State *L)
	{
		SQLiteLuaHandle **ptr = (SQLiteLuaHandle **)lua_touserdata(L, 1);
		SQLiteLuaHandle *handle = *ptr;
		handle->release();
		return 0;
	}

	static int stmt_gc_tolua(lua_State *L)
	{
		SQLiteLuaStmt *ptr = (SQLiteLuaStmt *)lua_touserdata(L, 1);
		ptr->~SQLiteLuaStmt();
		return 0;
	}

	int sqlite_tolua(lua_State *L)
	{
		static const luaL_Reg libs[] = {
			{"open", open_tolua},
			{"close", close_tolua},
			{"execute", execute_tolua},
			{"prepare", prepare_tolua},
			{NULL, NULL}
		};
		static const luaL_Reg metas[] = {
			{"__gc", gc_tolua},
			{NULL, NULL}
		};
		static const luaL_Reg stmtlibs[] = {
			{"status", status_tolua},
			{"column", column_tolua},
			{"step", step_tolua},
			{"rows", rows_tolua},
			{"reset", reset_tolua},
			{"clear", clear_tolua},
			{"bind", bind_tolua},
			{"next", next_tolua},
			{NULL, NULL}
		};
		static const luaL_Reg stmtmetas[] = {
			{"__gc", stmt_gc_tolua},
			{NULL, NULL}
		};
		lua_pushvalue(L, LUA_ENVIRONINDEX);
		luaL_register(L, NULL, libs);
		lua_createtable(L, 0, sizeof(metas));
		luaL_register(L, NULL, metas);
		lua_pushvalue(L, -2);
		lua_setfield(L, -2, "__index");
		lua_pushlightuserdata(L, &sqlitemetakey);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
		lua_pop(L, 1);
		lua_pushlightuserdata(L, &stmtmetakey);
		lua_createtable(L, 0, sizeof(stmtmetas));
		luaL_register(L, NULL, stmtmetas);
		lua_createtable(L, 0, sizeof(stmtlibs));
		luaL_register(L, NULL, stmtlibs);
		lua_setfield(L, -2, "__index");
		lua_settable(L, LUA_REGISTRYINDEX);
		return 1;
	}
}
