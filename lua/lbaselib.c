/*
** $Id: lbaselib.c,v 1.191.1.6 2008/02/14 16:46:22 roberto Exp $
** Basic library
** See Copyright Notice in lua.h
*/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lbaselib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

static int luaB_tonumber (lua_State *L) {
  int base = luaL_optint(L, 2, 10);
  if (base == 10) {  /* standard conversion */
    luaL_checkany(L, 1);
    if (lua_isnumber(L, 1)) {
      lua_pushnumber(L, lua_tonumber(L, 1));
      return 1;
    }
  }
  else {
    const char *s1 = luaL_checkstring(L, 1);
    char *s2;
    unsigned long n;
    luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    n = strtoul(s1, &s2, base);
    if (s1 != s2) {  /* at least one valid digit? */
      while (isspace((unsigned char)(*s2))) s2++;  /* skip trailing spaces */
      if (*s2 == '\0') {  /* no invalid trailing characters? */
        lua_pushnumber(L, (lua_Number)n);
        return 1;
      }
    }
  }
  lua_pushnil(L);  /* else not a number */
  return 1;
}


static int luaB_error (lua_State *L) {
  int level = luaL_optint(L, 2, 1);
  lua_settop(L, 1);
  if (lua_isstring(L, 1) && level > 0) {  /* add extra information? */
    luaL_where(L, level);
    lua_pushvalue(L, 1);
    lua_concat(L, 2);
  }
  return lua_error(L);
}


static int luaB_getmetatable (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_getmetatable(L, 1)) {
    lua_pushnil(L);
    return 1;  /* no metatable */
  }
  luaL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int luaB_setmetatable (lua_State *L) {
  int t = lua_type(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE, 2,
                    "nil or table expected");
  if (luaL_getmetafield(L, 1, "__metatable"))
    luaL_error(L, "cannot change a protected metatable");
  lua_settop(L, 2);
  lua_setmetatable(L, 1);
  return 1;
}


static int luaB_rawequal (lua_State *L) {
  luaL_checkany(L, 1);
  luaL_checkany(L, 2);
  lua_pushboolean(L, lua_rawequal(L, 1, 2));
  return 1;
}


static int luaB_rawget (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  lua_settop(L, 2);
  lua_rawget(L, 1);
  return 1;
}

static int luaB_rawset (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  luaL_checkany(L, 3);
  lua_settop(L, 3);
  lua_rawset(L, 1);
  return 1;
}


static int luaB_collectgarbage (lua_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul", NULL};
  static const int optsnum[] = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
    LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPAUSE, LUA_GCSETSTEPMUL};
  int o = luaL_checkoption(L, 1, "collect", opts);
  int ex = luaL_optint(L, 2, 0);
  int res = lua_gc(L, optsnum[o], ex);
  switch (optsnum[o]) {
    case LUA_GCCOUNT: {
      int b = lua_gc(L, LUA_GCCOUNTB, 0);
      lua_pushnumber(L, res + ((lua_Number)b/1024));
      return 1;
    }
    case LUA_GCSTEP: {
      lua_pushboolean(L, res);
      return 1;
    }
    default: {
      lua_pushnumber(L, res);
      return 1;
    }
  }
}


static int luaB_typeof (lua_State *L) {
  luaL_checkany(L, 1);
  if (luaL_callmeta(L, 1, "__type"))  /* is there a metafield? */
    return 1;  /* use its value */
  lua_pushstring(L, luaL_typename(L, 1));
  return 1;
}


static int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int luaB_pairs (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushvalue(L, lua_upvalueindex(1));  /* return generator, */
  lua_pushvalue(L, 1);  /* state, */
  lua_pushnil(L);  /* and initial value */
  return 3;
}


static int ipairsaux (lua_State *L) {
  int i = luaL_checkint(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  i++;  /* next value */
  lua_pushinteger(L, i);
  lua_rawgeti(L, 1, i);
  return (lua_isnil(L, -1)) ? 0 : 2;
}


static int luaB_ipairs (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushvalue(L, lua_upvalueindex(1));  /* return generator, */
  lua_pushvalue(L, 1);  /* state, */
  lua_pushinteger(L, 0);  /* and initial value */
  return 3;
}


static int load_aux (lua_State *L, int status) {
  if (status == 0)  /* OK? */
    return 1;
  else {
    lua_pushnil(L);
    lua_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


/*
** Reader for generic `load' function: `lua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (lua_State *L, void *ud, size_t *size) {
  (void)ud;  /* to avoid warnings */
  luaL_checkstack(L, 2, "too many nested functions");
  lua_pushvalue(L, 1);  /* get function */
  lua_call(L, 0, 1);  /* call it */
  if (lua_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (lua_isstring(L, -1)) {
    lua_replace(L, 4);  /* save string in a reserved stack slot */
    return lua_tolstring(L, 4, size);
  }
  else luaL_error(L, "reader function must return a string");
  return NULL;  /* to avoid warnings */
}


static int luaB_load (lua_State *L) {
  size_t l;
  const char *s = lua_tolstring(L, 1, &l);
  const char *defaultname = "=(load)";
  const char *chunkname;
  int status;
  int env = 0;
  if (s != NULL && s[l] == '\0') {
    defaultname = s;
  }
  else {
	if (lua_type(L, 1) != LUA_TFUNCTION) {
	  const char *msg = lua_pushfstring(L, "%s or %s expected, got %s",
		lua_typename(L, LUA_TFUNCTION), lua_typename(L, LUA_TSTRING), luaL_typename(L, 1));
	  luaL_argerror(L, 1, msg);
	}
  }
  if (lua_gettop(L) == 2) {
    if (lua_isstring(L, 2)) {
      chunkname = lua_tostring(L, 2);
    }
    else {
      chunkname = defaultname;
      luaL_checktype(L, 2, LUA_TTABLE);
      env = 2;
    }
  }
  else {
    chunkname = luaL_optstring(L, 2, defaultname);
    if (!lua_isnone(L, 3)) {
      luaL_checktype(L, 3, LUA_TTABLE);
      env = 3;
    }
  }
  if (env == 0) {
    lua_Debug ar;
	lua_getstack(L, 1, &ar);
    lua_getinfo(L, "f", &ar);
    lua_getfenv(L, -1);
    lua_replace(L, -2);
    env = lua_gettop(L);
  }
  if (s != NULL) {
    status = luaL_loadbuffer(L, s, l, chunkname);
  }
  else {
    lua_settop(L, 4);  /* function, eventual name, plus one reserved slot */
    status = lua_load(L, generic_reader, NULL, chunkname);
  }
  if (status == 0) {
    lua_pushvalue(L, env);
    lua_setfenv(L, -2);
  }
  return load_aux(L, status);
}


static int luaB_assert (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_toboolean(L, 1))
    return luaL_error(L, "%s", luaL_optstring(L, 2, "assertion failed!"));
  return lua_gettop(L);
}


static int luaB_unpack (lua_State *L) {
  int i, e, n;
  luaL_checktype(L, 1, LUA_TTABLE);
  i = luaL_optint(L, 2, 1);
  e = luaL_opt(L, luaL_checkint, 3, (int)lua_objlen(L, 1));
  if (i > e) return 0;  /* empty range */
  n = e - i + 1;  /* number of elements */
  if (n <= 0 || !lua_checkstack(L, n))  /* n <= 0 means arith. overflow */
    return luaL_error(L, "too many results to unpack");
  lua_rawgeti(L, 1, i);  /* push arg[i] (avoiding overflow problems) */
  while (i++ < e)  /* push arg[i + 1...e] */
    lua_rawgeti(L, 1, i);
  return n;
}


static int luaB_select (lua_State *L) {
  int n = lua_gettop(L);
  if (lua_type(L, 1) == LUA_TSTRING && *lua_tostring(L, 1) == '#') {
    lua_pushinteger(L, n-1);
    return 1;
  }
  else {
    int i = luaL_checkint(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    luaL_argcheck(L, 1 <= i, 1, "index out of range");
    return n - i;
  }
}


static int luaB_pcall (lua_State *L) {
  int status;
  luaL_checkany(L, 1);
  status = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);
  lua_pushboolean(L, (status == 0));
  lua_insert(L, 1);
  return lua_gettop(L);  /* return status + all results */
}


static int luaB_xpcall (lua_State *L) {
  int status;
  luaL_checkany(L, 2);
  lua_settop(L, 2);
  lua_insert(L, 1);  /* put error function under function to be called */
  status = lua_pcall(L, 0, LUA_MULTRET, 1);
  lua_pushboolean(L, (status == 0));
  lua_replace(L, 1);
  return lua_gettop(L);  /* return status + all results */
}


static int luaB_global (lua_State *L) {
  int top = lua_gettop(L);
  const char *s = NULL;
  const char * name;
  if (top == 0) {
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    return 1;
  }
  name = luaL_checkstring(L, 1);
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  lua_pushvalue(L, -1);
  do {
    name = (s == NULL ? name : s + 1);
    s = strchr(name, '.');
    if (s == NULL)
      s = name + strlen(name);
    if (s == name)
      luaL_argerror(L, 1, "field format error");
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_replace(L, -2);
    lua_pushlstring(L, name, s - name);
    lua_gettable(L, -2);
  } while (*s != '\0');
  if (top > 1) {
    lua_pushstring(L, name);
    lua_pushvalue(L, 2);
    lua_settable(L, -4);
  }
  return 1;
}

static int luaB_tostring (lua_State *L) {
  luaL_checkany(L, 1);
  if (luaL_callmeta(L, 1, "__tostring"))  /* is there a metafield? */
    return 1;  /* use its value */
  switch (lua_type(L, 1)) {
    case LUA_TNUMBER:
      lua_pushstring(L, lua_tostring(L, 1));
      break;
    case LUA_TSTRING:
      lua_pushvalue(L, 1);
      break;
    case LUA_TBOOLEAN:
      lua_pushstring(L, (lua_toboolean(L, 1) ? "true" : "false"));
      break;
    case LUA_TNIL:
      lua_pushliteral(L, "nil");
      break;
    default:
      lua_pushfstring(L, "%s: %p", luaL_typename(L, 1), lua_topointer(L, 1));
      break;
  }
  return 1;
}


static int luaB_newproxy (lua_State *L) {
  lua_settop(L, 1);
  lua_newuserdata(L, 0);  /* create proxy */
  if (lua_isnil(L, 1))
    return 1;  /* no metatable */
  else if (lua_istable(L, 1)) {
    lua_pushvalue(L, 1);  /* push self metatable `m' ... */
    lua_pushvalue(L, -1);  /* ... and mark `m' as a valid metatable */
    lua_pushboolean(L, 1);
    lua_rawset(L, lua_upvalueindex(1));  /* weaktable[m] = true */
  }
  else {
    int validproxy = 0;  /* to check if weaktable[metatable(u)] == true */
    if (lua_getmetatable(L, 1)) {
      lua_rawget(L, lua_upvalueindex(1));
      validproxy = lua_toboolean(L, -1);
      lua_pop(L, 1);  /* remove value */
    }
    luaL_argcheck(L, validproxy, 1, "table or proxy expected");
    lua_getmetatable(L, 1);  /* metatable is valid; get it */
  }
  lua_setmetatable(L, 2);
  return 1;
}


static int luaB_stacktrace (lua_State *L) {
  int arg = 0;
  lua_State *L1 = L;
  const char *msg = NULL;
  if (lua_isthread(L, 1))
  {
    arg = 1;
    L1 = lua_tothread(L, 1);
  }
  if (lua_gettop(L) > arg)
    msg = luaL_checkstring(L, arg + 1);
  luaL_traceback(L, L1, msg, L == L1 ? 1 : 0);
  return 1;
}


static const luaL_Reg base_funcs[] = {
  {"assert", luaB_assert},
  {"collectgarbage", luaB_collectgarbage},
  {"error", luaB_error},
  {"getmetatable", luaB_getmetatable},
  {"load", luaB_load},
  {"next", luaB_next},
  {"pcall", luaB_pcall},
  {"rawequal", luaB_rawequal},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"select", luaB_select},
  {"setmetatable", luaB_setmetatable},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"typeof", luaB_typeof},
  {"unpack", luaB_unpack},
  {"xpcall", luaB_xpcall},
  {"global", luaB_global},
  {"stacktrace", luaB_stacktrace},
  {NULL, NULL}
};


/*
** {======================================================
** Coroutine library
** =======================================================
*/

#define CO_RUN	0	/* running */
#define CO_SUS	1	/* suspended */
#define CO_NOR	2	/* 'normal' (it resumed another coroutine) */
#define CO_DEAD	3

static const char *const statnames[] =
    {"running", "suspended", "normal", "dead"};

static int costatus (lua_State *L, lua_State *co) {
  if (L == co) return CO_RUN;
  switch (lua_status(co)) {
    case LUA_YIELD:
      return CO_SUS;
    case 0: {
      lua_Debug ar;
      if (lua_getstack(co, 0, &ar) > 0)  /* does it have frames? */
        return CO_NOR;  /* it is running */
      else if (lua_gettop(co) == 0)
          return CO_DEAD;
      else
        return CO_SUS;  /* initial state */
    }
    default:  /* some error occured */
      return CO_DEAD;
  }
}


static int luaB_costatus (lua_State *L) {
  lua_State *co = lua_tothread(L, 1);
  luaL_argcheck(L, co, 1, "coroutine expected");
  lua_pushstring(L, statnames[costatus(L, co)]);
  return 1;
}


static int auxresume (lua_State *L, lua_State *co, int narg) {
  int status = costatus(L, co);
  if (!lua_checkstack(co, narg))
    luaL_error(L, "too many arguments to resume");
  if (status != CO_SUS) {
    lua_pushfstring(L, "cannot resume %s coroutine", statnames[status]);
    return -1;  /* error flag */
  }
  lua_xmove(L, co, narg);
  lua_setlevel(L, co);
  status = lua_resume(co, narg);
  if (status == 0 || status == LUA_YIELD) {
    int nres = lua_gettop(co);
    if (!lua_checkstack(L, nres + 1))
      luaL_error(L, "too many results to resume");
    lua_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    lua_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static int luaB_coresume (lua_State *L) {
  lua_State *co = lua_tothread(L, 1);
  int r;
  luaL_argcheck(L, co, 1, "coroutine expected");
  r = auxresume(L, co, lua_gettop(L) - 1);
  if (r < 0) {
    lua_pushboolean(L, 0);
    lua_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    lua_pushboolean(L, 1);
    lua_insert(L, -(r + 1));
    return r + 1;  /* return true + `resume' returns */
  }
}


static int luaB_auxwrap (lua_State *L) {
  lua_State *co = lua_tothread(L, lua_upvalueindex(1));
  int r = auxresume(L, co, lua_gettop(L));
  if (r < 0) {
    if (lua_isstring(L, -1)) {  /* error object is a string? */
      luaL_where(L, 1);  /* add extra info */
      lua_insert(L, -2);
      lua_concat(L, 2);
    }
    lua_error(L);  /* propagate error */
  }
  return r;
}


static int luaB_cocreate (lua_State *L) {
  lua_State *NL = lua_newthread(L);
  luaL_argcheck(L, lua_isfunction(L, 1) && !lua_iscfunction(L, 1), 1,
    "Lua function expected");
  lua_pushvalue(L, 1);  /* move function to top */
  lua_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static int luaB_cowrap (lua_State *L) {
  luaB_cocreate(L);
  lua_pushcclosure(L, luaB_auxwrap, 1);
  return 1;
}


static int luaB_yield (lua_State *L) {
  return lua_yield(L, lua_gettop(L));
}


static int luaB_corunning (lua_State *L) {
  if (lua_pushthread(L))
    lua_pushnil(L);  /* main thread is not a coroutine */
  return 1;
}


static const luaL_Reg co_funcs[] = {
  {"create", luaB_cocreate},
  {"resume", luaB_coresume},
  {"running", luaB_corunning},
  {"status", luaB_costatus},
  {"wrap", luaB_cowrap},
  {"yield", luaB_yield},
  {NULL, NULL}
};

/* }====================================================== */

static const char sentinel_ = 0;
#define sentinel	((void *)&sentinel_)

/*
** {======================================================
** 'require' function
** =======================================================
*/


static int readable (const char *filename) {
  FILE *f = fopen(filename, "r");  /* try to open file */
  if (f == NULL) return 0;  /* open failed */
  fclose(f);
  return 1;
}


static const char *pushnexttemplate (lua_State *L, const char *path) {
  const char *l;
  while (*path == *LUA_PATHSEP) path++;  /* skip separators */
  if (*path == '\0') return NULL;  /* no more templates */
  l = strchr(path, *LUA_PATHSEP);  /* find next separator */
  if (l == NULL) l = path + strlen(path);
  lua_pushlstring(L, path, l - path);  /* template */
  return l;
}

static int loader_Fail (lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  luaL_error(L, "error loading module " LUA_QS ":\n\tno loader available.", name);
  return 0;  /* library loaded successfully */
}


static int aux_require (lua_State *L) {
  lua_pushvalue(L, 2);
  lua_gettable(L, lua_upvalueindex(1));
  if (!lua_isnil(L, -1)) {
    lua_pushvalue(L, 2);
    lua_pushvalue(L, -2);
    lua_settable(L, 1);
  }
  return 1;
}


static int ll_require (lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  int i;
  lua_settop(L, 1);  /* _LOADED table will be at index 2 */
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, 2, name);
  if (!lua_isnil(L, -1)) {  /* is it there? */
    if (lua_touserdata(L, -1) == sentinel)  /* check loops */
      luaL_error(L, "loop or previous error loading module " LUA_QS, name);
    return 1;  /* module is already loaded */
  }
  /* else must load it; iterate over available loaders */
  do {
    lua_getfield(L, LUA_ENVIRONINDEX, "preload");
    if (lua_istable(L, -1)) {
      lua_getfield(L, -1, name);
      if (lua_isfunction(L, -1))
        break;
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_getfield(L, LUA_ENVIRONINDEX, "loaders");
    if (!lua_istable(L, -1))
      luaL_error(L, LUA_QL("require.loaders") " must be a table");
    lua_pushstring(L, name);
    for (i = 1; ; ++i) {
      lua_rawgeti(L, -3, i);  /* get a loader */
      if (lua_isnil(L, -1))
        luaL_error(L, "module " LUA_QS " not found", name);
      lua_pushvalue(L, -2);
      lua_call(L, 1, 1);  /* call it */
      if (lua_isfunction(L, -1))  /* did it find module? */
        break;  /* module loaded successfully */
      lua_pop(L, 1);
    }
  } while (0);
  lua_pushlightuserdata(L, sentinel);
  lua_setfield(L, 2, name);  /* _LOADED[name] = sentinel */
  if (luaL_findtable(L, LUA_GLOBALSINDEX, name, 0) != NULL)
    luaL_error(L, "name conflict for module " LUA_QS, name);
  {
    const char *pdot = strchr(name, '.');
	lua_createtable(L, 0, 1);
    if (pdot == NULL)
    {
      lua_pushvalue(L, LUA_GLOBALSINDEX);
    }
    else
    {
      lua_pushlstring(L, name, pdot - name);
      if (luaL_findtable(L, LUA_GLOBALSINDEX, lua_tostring(L, -1), 0) != NULL)
        luaL_error(L, "name conflict for module " LUA_QS, name);
      lua_remove(L, -2);
    }
    lua_pushcclosure(L, aux_require, 1);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
  }
  lua_insert(L, -2);
  lua_pushvalue(L, -2);
  lua_setfenv(L, -2);
  lua_pushstring(L, name);
  lua_call(L, 1, 1);  /* run loaded module */
  if (lua_isnil(L, -1))
    lua_pop(L, 1);
  lua_pushvalue(L, -1);
  lua_setfield(L, 2, name);  /* _LOADED[name] = returned value */
  return 1;
}

static int aux_require_call (lua_State *L) {
  lua_pushvalue(L, lua_upvalueindex(1));
  lua_replace(L, 1);
  lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
  return lua_gettop(L);
}


static const lua_CFunction loaders[] = {
  {loader_Fail},
  {NULL}
};


/* }====================================================== */


static void auxopen (lua_State *L, const char *name,
                     lua_CFunction f, lua_CFunction u) {
  lua_pushcfunction(L, u);
  lua_pushcclosure(L, f, 1);
  lua_setfield(L, LUA_GLOBALSINDEX, name);
}


static void base_open (lua_State *L) {
  /* set global */
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  /* open lib into global table */
  luaL_register(L, NULL, base_funcs);
  lua_pop(L, 1);
  /* `ipairs' and `pairs' need auxiliary functions as upvalues */
  auxopen(L, "ipairs", luaB_ipairs, ipairsaux);
  auxopen(L, "pairs", luaB_pairs, luaB_next);
  /* `newproxy' needs a weaktable as upvalue */
  lua_createtable(L, 0, 1);  /* new table `w' */
  lua_pushvalue(L, -1);  /* `w' will be its own metatable */
  lua_setmetatable(L, -2);
  lua_pushliteral(L, "kv");
  lua_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  lua_pushcclosure(L, luaB_newproxy, 1);
  lua_setfield(L, LUA_GLOBALSINDEX, "newproxy");  /* set global `newproxy' */
}


static void require_open (lua_State *L) {
  int i;
  /* create `require' table */
  lua_createtable(L, 0, 0);
  /* create `loaders' table */
  lua_createtable(L, sizeof(loaders)/sizeof(loaders[0]) - 1, 0);
  /* fill it with pre-defined loaders */
  for (i = 0; loaders[i] != NULL; ++i) {
    lua_pushcfunction(L, loaders[i]);
    lua_rawseti(L, -2, i + 1);
  }
  lua_setfield(L, -2, "loaders");  /* put it in field `loaders' */
  /* set field `loaded' */
  luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 16);
  lua_setfield(L, -2, "loaded");
  /* set field `preload' */
  luaL_findtable(L, LUA_REGISTRYINDEX, "_PRELOAD", 16);
  lua_setfield(L, -2, "preload");
  lua_createtable(L, 0, 1);
  lua_pushcfunction(L, ll_require);
  lua_pushvalue(L, -3);
  lua_setfenv(L, -2);
  lua_pushcclosure(L, aux_require_call, 1);
  lua_setfield(L, -2, "__call");
  lua_setmetatable(L, -2);
  lua_setfield(L, LUA_GLOBALSINDEX, "require");
}

LUALIB_API int luaopen_coroutine (lua_State *L) {
  lua_pushvalue(L, LUA_ENVIRONINDEX);
  luaL_register(L, NULL, co_funcs);
  return 1;
}


LUALIB_API int luaopen_base (lua_State *L) {
  base_open(L);
  require_open(L);
  return 0;
}

