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

/* Using
   [vis/vis-lua.c](https://github.com/martanne/vis/blob/945db2ed898d4feb55ca9e690dd859503f49c231/vis-lua.c)
   as a reference */

static lua_State *L = NULL;

static void
obj_ref_set(lua_State *L, void *addr)
{
  lua_getfield(L, LUA_REGISTRYINDEX, "mace.objects");
  lua_pushlightuserdata(L, addr);
  lua_pushvalue(L, -3);
  lua_settable(L, -3);
  lua_pop(L, 1);
}

static void
obj_type_new(lua_State *L, const char *type)
{
  luaL_newmetatable(L, type);
  lua_getfield(L, LUA_REGISTRYINDEX, "mace.types");
  lua_pushvalue(L, -2);
  lua_pushstring(L, type);
  lua_settable(L, -3);
  lua_pop(L, 1);
}

static void *
obj_ref_new(lua_State *L, void *addr, const char *type)
{
  void **handle;
  
  lua_getfield(L, LUA_REGISTRYINDEX, "mace.objects");
  lua_pushlightuserdata(L, addr);
  lua_gettable(L, -2);
  lua_remove(L, -2);

  if (lua_type(L, -1) == LUA_TUSERDATA) {
    handle = luaL_checkudata(L, -1, type);

    if (*handle != addr) {
      fprintf(stderr, "lua handle for %p points to %p!\n",
	      addr, *handle);

      return NULL;
    } else {
      return addr;
    }
    
  } else {
    /* Pop the invalid value */
    lua_pop(L, 1); 

    handle = (void **) lua_newuserdata(L, sizeof(addr));

    luaL_getmetatable(L, type);
    lua_setmetatable(L, -2);

    obj_ref_set(L, addr);

    *handle = addr;

    return addr;
  }
}

static void *
obj_ref_get(lua_State *L, void *addr, const char *type)
{
  lua_getfield(L, LUA_REGISTRYINDEX, "mace.objects");
  lua_pushlightuserdata(L, addr);
  lua_gettable(L, -2);
  lua_remove(L, -2);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    return NULL;
  } else {
    return luaL_checkudata(L, -1, type);
  }
}

static void *
obj_ref_check(lua_State *L, int index, const char *type)
{
  void **handle = luaL_checkudata(L, index, type);
  /* Need to check the object is still in the registry before using
     it. */
  if (!obj_ref_get(L, *handle, type)) {
    return NULL;
  } else {
    return *handle;
  }
}

static int
indexcommon(lua_State *L)
{
  lua_getmetatable(L, 1);
  lua_pushvalue(L, 2);
  lua_gettable(L, -2);

  return 1;
}

static int
ltabtostring(lua_State *L)
{
  struct tab *t;

  t = obj_ref_check(L, 1, "mace.tab");

  lua_pushfstring(L, "[tab %s]", t->name);

  return 1;
}

static int
ltabindex(lua_State *L)
{
  struct tab *t = obj_ref_check(L, 1, "mace.tab");
  const char *key;

  if (lua_isstring(L, 2)) {
    key = lua_tostring(L, 2);

    if (strcmp(key, "main") == 0) {
      obj_ref_new(L, t->main, "mace.textbox");
      return 1;
    }

    if (strcmp(key, "action") == 0) {
      obj_ref_new(L, t->action, "mace.textbox");
      return 1;
    }
  }

  return indexcommon(L);
}

static const struct luaL_Reg tab_funcs[] = {
  { "__tostring", ltabtostring },
  { "__index",    ltabindex },
  { NULL,         NULL },
};

static int
ltextboxtostring(lua_State *L)
{
  struct textbox *t;

  t = obj_ref_check(L, 1, "mace.textbox");
  
  lua_pushfstring(L, "[textbox %d]", t->cursor);

  return 1;
}

static int
ltextboxindex(lua_State *L)
{
  struct textbox *t = obj_ref_check(L, 1, "mace.textbox");
  const char *key;

  if (lua_isstring(L, 2)) {
    key = lua_tostring(L, 2);

    if (strcmp(key, "sequence") == 0) {
      obj_ref_new(L, t->sequence, "mace.sequence");
      return 1;
    }

    if (strcmp(key, "cursor") == 0) {
      lua_pushnumber(L, t->cursor);
      return 1;
    }
  }

  return indexcommon(L);
}

static const struct luaL_Reg textbox_funcs[] = {
  { "__tostring", ltextboxtostring },
  { "__index",    ltextboxindex },
  { NULL,         NULL },
};

static int
lsequencetostring(lua_State *L)
{
  struct sequence *s;
  
  s = obj_ref_check(L, 1, "mace.sequence");

  lua_pushfstring(L, "[sequence plen %d, dlen %d]", s->plen, s->dlen);

  return 1;
}

static int
lsequenceinsert(lua_State *L)
{
  const uint8_t *data;
  struct sequence *s;
  size_t pos, len;
  bool r;
  
  s = obj_ref_check(L, 1, "mace.sequence");
  pos = luaL_checknumber(L, 2);
  data = luaL_checklstring(L, 3, &len);

  r = sequenceinsert(s, pos, data, len);

  lua_pushboolean(L, r);

  return 1;
}

static int
lsequencedelete(lua_State *L)
{
  struct sequence *s;
  size_t start, len;
  bool r;
  
  s = obj_ref_check(L, 1, "mace.sequence");
  start = luaL_checknumber(L, 2);
  len = luaL_checknumber(L, 3);

  r = sequencedelete(s, start, len);

  lua_pushboolean(L, r);
  
  return 1;
}

static int
lsequenceget(lua_State *L)
{
  size_t start, len, i, l;
  struct sequence *s;
  uint8_t buf[512];
  luaL_Buffer b;

  s = obj_ref_check(L, 1, "mace.sequence");
  start = luaL_checknumber(L, 2);
  len = luaL_checknumber(L, 3);

  luaL_buffinit(L, &b);

  i = 0;
  while (i < len) {
    l = sequenceget(s, start + i, buf,
		    sizeof(buf) > len - i ? len - i : sizeof(buf));
    if (l == 0) {
      break;
    }

    luaL_addlstring(&b, buf, l);
    i += l;
  }

  luaL_pushresult(&b);
  
  return 1;
}

static const struct luaL_Reg sequence_funcs[] = {
  { "__tostring", lsequencetostring },
  { "__index",    indexcommon },
  { "insert",     lsequenceinsert },
  { "delete",     lsequencedelete },
  { "get",        lsequenceget },
  { NULL,         NULL },
};

void
luafree(void *addr)
{
  lua_pushnil(L);
  obj_ref_set(L, addr);
}

void
command(struct tab *tab, uint8_t *s)
{
  struct selection *sel;
  int index;
  
  lua_getglobal(L, s);

  obj_ref_new(L, tab, "mace.tab");

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
    obj_ref_new(L, sel->textbox, "mace.textbox");
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

static const struct luaL_Reg global_funcs[] = {
  { "setfont", lsetfont },
  { NULL, NULL }
};

void
luainit(void)
{
  const struct luaL_Reg *f;
  int r;

  L = luaL_newstate();
  if (L == NULL) {
    errx(1, "Failed to initalize lua!");
  }

  luaL_openlibs(L);

  /* Set global funcs */
  
  for (f = global_funcs; f->func != NULL; f++) {
    lua_pushcfunction(L, f->func);
    lua_setglobal(L, f->name);
  }
 
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "mace.types");

  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "mace.objects");

  obj_type_new(L, "mace.tab");
  luaL_setfuncs(L, tab_funcs, 0);

  obj_type_new(L, "mace.textbox");
  luaL_setfuncs(L, textbox_funcs, 0);

  obj_type_new(L, "mace.sequence");
  luaL_setfuncs(L, sequence_funcs, 0);

  /* Load init file */
  
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
