/*
** $Id: linit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"


static const luaL_Reg loadlibs[] = {
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_BITLIBNAME,	luaopen_bit},
  {LUA_PEGLIBNAME,	luaopen_peg},
  {NULL, NULL}
};

static const luaL_Reg preloadlibs[] = {
  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};


LUALIB_API void luaL_initlibs (lua_State *L) {
  int i;
  lua_pushcfunction(L, luaopen_base);
  if (lua_pcall(L, 0, 0, 0) != 0) {
    lua_pop(L, 1);
  }
  luaL_findtable(L, LUA_REGISTRYINDEX, "_PRELOAD",
    sizeof(loadlibs) / sizeof(loadlibs[0]) + sizeof(preloadlibs) / sizeof(preloadlibs[0]) - 2);
  for (i = 0; i < sizeof(preloadlibs) / sizeof(preloadlibs[0]) - 1; ++i) {
    lua_pushcfunction(L, preloadlibs[i].func);
    lua_setfield(L, -2, preloadlibs[i].name);
  }
  for (i = 0; i < sizeof(loadlibs) / sizeof(loadlibs[0]) - 1; ++i) {
    lua_pushcfunction(L, loadlibs[i].func);
    lua_setfield(L, -2, loadlibs[i].name);
  }
  lua_pop(L, 1);
}


LUALIB_API void luaL_openlibs (lua_State *L) {
  int i;
  lua_getfield(L, LUA_GLOBALSINDEX, "require");
  for (i = 0; i < sizeof(loadlibs) / sizeof(loadlibs[0]) - 1; ++i) {
    lua_pushvalue(L, -1);
    lua_pushstring(L, loadlibs[i].name);
    if (lua_pcall(L, 1, 0, 0) != 0)
      lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

