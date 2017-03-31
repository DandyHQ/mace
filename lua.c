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

  name = lua_tolstring(L, -2, &len);
  
  size = lua_tonumber(L, -1);

  printf("should load font %s strlen %i, size %i\n", name, len, size);

  r = fontload(name, size);
  
  lua_pushnumber(L, r);
  return 1; 
}

void
luainit(void)
{
  int r;

  lua = luaL_newstate();
  if (lua == NULL) {
    err(1, "Failed to initalize lua!\n");
  }

  luaL_openlibs(lua);

  lua_pushcfunction(lua, lfontload);
  lua_setglobal(lua, "fontload");
  
  r = luaL_loadfile(lua, "init.lua");
  if (r == LUA_ERRFILE) {
    err(1, "Failed to open init.lua\n");
  } else if (r != LUA_OK) {
    err(1, "Failed to load init.lua %i\n", r);
  }

  r = lua_pcall(lua, 0, LUA_MULTRET, 0);
  if (r != LUA_OK) {
    err(1, "Error running init.lua %i\n", r);
  }
}
