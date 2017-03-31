#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mace.h"

static lua_State *lua = NULL;

static int
lfontload(lua_State *L)
{
  const uint8_t *name;
  size_t size, len;
  int r;

  name = (const uint8_t *) luaL_checklstring(L, -2, &len);
  size = luaL_checknumber(L, -1);

  r = fontload(name, size);
  
  lua_pushnumber(L, r);
  return 1; 
}

luaL_Reg funcs[] = {
  { "loadfont", lfontload },
};

void
luainit(void)
{
  int r, i;

  lua = luaL_newstate();
  if (lua == NULL) {
    err(1, "Failed to initalize lua!\n");
  }

  luaL_openlibs(lua);

  for (i = 0; i < sizeof(funcs) / sizeof(funcs[0]); i++) {
    lua_pushcfunction(lua, funcs[i].func);
    lua_setglobal(lua, funcs[i].name);
  }
  
  r = luaL_loadfile(lua, "init.lua");
  if (r == LUA_ERRFILE) {
    err(1, "Failed to open init.lua\n");
  } else if (r != LUA_OK) {
    err(1, "Error loading init: %s\n", lua_tostring(lua, -1));
  }

  r = lua_pcall(lua, 0, LUA_MULTRET, 0);
  if (r != LUA_OK) {
    err(1, "Error running init: %s\n", lua_tostring(lua, -1));
  }
}
