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

static lua_State *L = NULL;

static struct textbox *
lua_checktextbox(lua_State *L)
{
  luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
  return (struct textbox *) lua_touserdata(L, 1);
}

static int
ltextboxtostring(lua_State *L)
{
  struct textbox *t = lua_checktextbox(L);

  lua_pushfstring(L, "textbox[%d %dx%d -> %d]", t->cursor,
		  t->linewidth, t->height, t->yoff);

  return 1;
}

static int
ltextboxcursor(lua_State *L)
{
  struct textbox *t = lua_checktextbox(L);

  lua_pushnumber(L, t->cursor);

  return 1;
}

static const struct luaL_Reg textbox_f[] = {
  { "tostring",   ltextboxtostring },
  { "cursor",     ltextboxcursor },
  { NULL, NULL },
};

void
command(struct textbox *main, uint8_t *s)
{
  struct selection *sel;
  int index;
  
  lua_getglobal(L, s);

  lua_pushlightuserdata(L, main);

  lua_newtable(L);
  index = 0;
  for (sel = selections; sel != NULL; sel = sel->next) {
    lua_pushnumber(L, index++);
    lua_newtable(L);

    lua_pushstring(L, "start");
    lua_pushnumber(L, sel->start);
    lua_settable(L, -3);

    lua_pushstring(L, "len");
    lua_pushnumber(L, sel->end - sel->start + 1);
    lua_settable(L, -3);

    lua_pushstring(L, "textbox");
    lua_pushlightuserdata(L, sel->textbox);
    lua_settable(L, -3);

    lua_settable(L, -3);
  }

  if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
    fprintf(stderr, "error calling %s: %s\n", s, lua_tostring(L, -1));
    return;
  }
}

static int
lsetfont(lua_State *L)
{
  const uint8_t *name;
  size_t size, len;
  int r;

  name = (const uint8_t *) luaL_checklstring(L, -2, &len);
  size = luaL_checknumber(L, -1);

  r = fontset(name, size);
  
  lua_pushnumber(L, r);

  return 1; 
}

static const struct luaL_Reg funcs[] = {
  { "setfont", lsetfont },
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

  luaL_newlib(L, textbox_f);
  lua_setglobal(L, "textbox");

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
