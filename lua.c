#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mace.h"

static lua_State *L= NULL;

void
eval(struct textbox *main, uint8_t *s, size_t len)
{
  printf("getting lua to eval '%s'\n", s);
  
  lua_getglobal(L, "eval");
  lua_pushlightuserdata(L, main);
  lua_pushlstring(L, s, len);

  if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
    fprintf(stderr, "error calling eval: %s\n", lua_tostring(L, -1));
    return;
  }
}

static int
lloadfont(lua_State *L)
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

static const struct luaL_Reg funcs[] = {
  { "loadfont", lloadfont },
  { NULL, NULL }
};

void
luainit(void)
{
  int r;

  L = luaL_newstate();
  if (L == NULL) {
    errx(1, "Failed to initalize lua!");
  }

  luaL_openlibs(L);

  luaL_newlib(L, funcs);
  lua_setglobal(L, "mace");

  r = luaL_loadfile(L, "init.lua");
  if (r == LUA_ERRFILE) {
    errx(1, "Failed to open init.lua");
  } else if (r != LUA_OK) {
    errx(1, "Error loading init: %s", lua_tostring(L, -1));
  }

  r = lua_pcall(L, 0, LUA_MULTRET, 0);
  if (r != LUA_OK) {
    errx(1, "Error running init: %s", lua_tostring(L, -1));
  }
}
