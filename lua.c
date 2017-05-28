#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pwd.h>

#include "mace.h"
#include "config.h"

/* Mace's, Pane's, Tab's, and Textbox's are represented as user data 
   objects that contain pointers to the c data. The user data 
   structures are stored in lua's registry and are managed by lua's 
   garbage collector. Before data is free'd on the C side it must call
   luaremove(L, addr) to remove the object from the lua registry. 
   Should a lua script try to use a object that has been free'd the 
   lua wrapper functions will check if the address is in the registry,
   if it is not then they fail. Thereby making the objects safe to 
   use. There can be no dangling pointers and we don't have to 
   constantly talk to lua to get data when working only in the C side 
   of life.

   For the sake of safety do not let lua change linked lists by
   itself. It should call bindings to do that to make sure items don't
   get lost. This doesn't really solve the problem of loosing them but
   it helps keep everything a little more consistant.
*/

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

  if (addr == NULL) {
    lua_pushnil(L);
    return NULL;
  }
  
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

    lua_newtable(L);
    lua_setuservalue(L, -2);
	
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

  if (obj_ref_get(L, *handle, type) == NULL) {
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

  if (lua_isnil(L, -1)) {
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
  }
	
  return 1;
}

static int
newindexcommon(lua_State *L)
{
  lua_getuservalue(L, 1);
  lua_pushvalue(L, 2);
  lua_pushvalue(L, 3);
  lua_settable(L, -3);

  return 0;
}

static int
pushtextboxselections(lua_State *L, int i, struct textbox *t)
{
  struct selection *sel;
  
  for (sel = t->selections; sel != NULL; sel = sel->next) {
    lua_pushnumber(L, i++);
    lua_newtable(L);

    lua_pushstring(L, "start");
    lua_pushnumber(L, sel->start);
    lua_settable(L, -3);

    lua_pushstring(L, "len");
    lua_pushnumber(L, sel->end - sel->start + 1);
    lua_settable(L, -3);

    lua_pushstring(L, "textbox");
    obj_ref_new(L, t, "mace.textbox");
    lua_settable(L, -3);

    lua_settable(L, -3);
  }

  return i;
}

static int
lmacetostring(lua_State *L)
{
  /*struct mace *mace = */obj_ref_check(L, 1, "mace");

  lua_pushfstring(L, "[mace]");

  return 1;
}

static int
lmaceindex(lua_State *L)
{
  struct mace *mace = obj_ref_check(L, 1, "mace");
  const char *key;

  if (lua_isstring(L, 2)) {
    key = lua_tostring(L, 2);

    if (strcmp(key, "version") == 0) {
      lua_pushstring(L, VERSION_STR);
      return 1;
    }
    
    if (strcmp(key, "focus") == 0) {
      obj_ref_new(L, mace->focus, "mace.textbox");
      return 1;
    }

    if (strcmp(key, "pane") == 0) {
      obj_ref_new(L, mace->pane, "mace.pane");
      return 1;
    } 
  }

  return indexcommon(L);
}

static int
lmacenewindex(lua_State *L)
{
  struct mace *mace = obj_ref_check(L, 1, "mace");
  struct textbox *tb;
  const char *key;
  
  if (lua_isstring(L, 2)) {
    key = lua_tostring(L, 2);

    if (strcmp(key, "focus") == 0) {
      tb = obj_ref_check(L, 3, "mace.textbox");
      mace->focus = tb;
      return 0;
    }
  }
    
  return newindexcommon(L);
}

static int
lmacesetfont(lua_State *L)
{
  struct mace *mace = obj_ref_check(L, 1, "mace");
  const char *pattern;
  size_t len;
  int r;

  pattern = luaL_checklstring(L, 2, &len);

  r = fontset(mace->font, pattern);

  lua_pushnumber(L, r);

  return 1;
}

static int
lmacequit(lua_State *L)
{
  struct mace *mace = obj_ref_check(L, 1, "mace");

  macequit(mace);

  return 0;
}

static int
lmaceemptytab(lua_State *L)
{
  struct mace *mace = obj_ref_check(L, 1, "mace");
  const uint8_t *name;
  struct tab *t;
  size_t nlen;

  name = (const uint8_t *) luaL_checklstring(L, 2, &nlen);

  t = tabnewempty(mace, name, nlen);
  if (t == NULL) {
    lua_pushnil(L);
    return 1;
  }

  obj_ref_new(L, t, "mace.tab");

  return 1;
}

static int
lmacefiletab(lua_State *L)
{
  struct mace *mace = obj_ref_check(L, 1, "mace");
  const uint8_t *name, *filename;
  size_t nlen, flen;
  struct tab *t;

  name = (const uint8_t *) luaL_checklstring(L, 2, &nlen);
  filename = (const uint8_t *) luaL_checklstring(L, 3, &flen);

  t = tabnewfromfile(mace, name, nlen, filename, flen);
  if (t == NULL) {
    lua_pushnil(L);
    return 1;
  }

  obj_ref_new(L, t, "mace.tab");

  return 1;
}

static int
lmaceselections(lua_State *L)
{
  struct mace *mace = obj_ref_check(L, 1, "mace");
  int i;

  lua_newtable(L);

  i = 1; /* Apparently lua arrays start at 1 */
  if (mace->focus != NULL) {
    i = pushtextboxselections(L, i, mace->focus->tab->action);
    i = pushtextboxselections(L, i, mace->focus->tab->main);
  }

  return 1;
}

static const struct luaL_Reg mace_funcs[] = {
  { "__tostring",      lmacetostring },
  { "__index",         lmaceindex },
  { "__newindex",      lmacenewindex },
  { "setfont",         lmacesetfont },
  { "quit",            lmacequit },
  { "newemptytab",     lmaceemptytab },
  { "newfiletab",      lmacefiletab },
  { "selections",      lmaceselections },
  { NULL, NULL }
};


static int
lpanetostring(lua_State *L)
{
  struct pane *p = obj_ref_check(L, 1, "mace.pane");

  lua_pushfstring(L, "[pane %d,%d %dx%d]",
		  p->x, p->y, p->width, p->height);

  return 1;
}

static int
lpaneindex(lua_State *L)
{
  struct pane *p = obj_ref_check(L, 1, "mace.pane");
  const char *key;

  if (lua_isstring(L, 2)) {
    key = lua_tostring(L, 2);

    if (strcmp(key, "focus") == 0) {
      obj_ref_new(L, p->focus, "mace.tab");
      return 1;
    }

    if (strcmp(key, "tabs") == 0) {
      obj_ref_new(L, p->tabs, "mace.tab");
      return 1;
    } 
  }

  return indexcommon(L);
}

static int
lpanenewindex(lua_State *L)
{
  struct pane *p = obj_ref_check(L, 1, "mace.pane");
  struct tab *tab;
  const char *key;
  
  if (lua_isstring(L, 2)) {
    key = lua_tostring(L, 2);

    if (strcmp(key, "focus") == 0) {
      tab = obj_ref_check(L, 3, "mace.tab");
      p->focus = tab;
      return 0;
    }

    if (strcmp(key, "tabs") == 0) {
      tab = obj_ref_check(L, 3, "mace.tab");
      p->tabs = tab;
      return 0;
    }
  }
    
  return newindexcommon(L);
}

static int
lpaneaddtab(lua_State *L)
{
  struct pane *p = obj_ref_check(L, 1, "mace.pane");
  struct tab *tab = obj_ref_check(L, 2, "mace.tab");
  int pos = luaL_checkinteger(L, 3);
  
  paneaddtab(p, tab, pos);
   
  return 0;
}

static int
lpaneremovetab(lua_State *L)
{
  struct pane *p = obj_ref_check(L, 1, "mace.pane");
  struct tab *tab = obj_ref_check(L, 2, "mace.tab");
  
  paneremovetab(p, tab);
   
  return 0;
}

static const struct luaL_Reg pane_funcs[] = {
  { "__tostring",      lpanetostring },
  { "__index",         lpaneindex },
  { "__newindex",      lpanenewindex },
  { "addtab",          lpaneaddtab },
  { "removetab",       lpaneremovetab },
  { NULL, NULL }
};

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

    if (strcmp(key, "pane") == 0) {
      obj_ref_new(L, t->pane, "mace.pane");
      return 1;
    }

    if (strcmp(key, "next") == 0) {
      obj_ref_new(L, t->next, "mace.tab");
      return 1;
    }
  }

  return indexcommon(L);
}

static int
ltabsetname(lua_State *L)
{
  struct tab *t = obj_ref_check(L, 1, "mace.tab");
  const char *name;
  size_t len;
  bool r;

  name = luaL_checklstring(L, 2, &len);
  r = tabsetname(t, (const uint8_t *) name, len);

  lua_pushboolean(L, r);
  return 1;
}

static int
ltabclose(lua_State *L)
{
  struct tab *t = obj_ref_check(L, 1, "mace.tab");

  if (t->pane != NULL) {
    paneremovetab(t->pane, t);
  }
  
  tabfree(t);

  return 0;
}

static int
ltabselections(lua_State *L)
{
  struct tab *t = obj_ref_check(L, 1, "mace.tab");
  int i;
  
  lua_newtable(L);

  i = 1; /* Apparently lua arrays start at 1 */
  i = pushtextboxselections(L, i, t->action);
  i = pushtextboxselections(L, i, t->main);

  return 1;
}

static const struct luaL_Reg tab_funcs[] = {
  { "__tostring",    ltabtostring },
  { "__index",       ltabindex },
  { "__newindex",    newindexcommon },
  { "setname",       ltabsetname },
  { "close",         ltabclose },
  { "selections",    ltabselections },
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

    if (strcmp(key, "cursor") == 0) {
      lua_pushnumber(L, t->cursor);
      return 1;
    }

    if (strcmp(key, "yoff") == 0) {
      lua_pushnumber(L, t->yoff);
      return 1;
    }

    if (strcmp(key, "tabstring") == 0) {
        lua_pushfstring(L, "%s", t->tabstring);
        return 1;
    }

    if (strcmp(key, "tab") == 0) {
      obj_ref_new(L, t->tab, "mace.tab");
      return 1;
    }
  }

  return indexcommon(L);
}

static int
ltextboxnewindex(lua_State *L)
{
  struct textbox *t = obj_ref_check(L, 1, "mace.textbox");
  const char *key, *str;
  int i;

  if (lua_isstring(L, 2)) {
    key = lua_tostring(L, 2);

    if (strcmp(key, "cursor") == 0) {
      i = luaL_checkinteger(L, 3);
      t->cursor = i;
      textboxpredraw(t);
      return 0;
    }

    if (strcmp(key, "yoff") == 0) {
      i = luaL_checkinteger(L, 3);
      t->yoff = i;
      textboxfindstart(t);
      textboxpredraw(t);
      return 0;
    }

    if (strcmp(key, "tabstring") == 0) {
        str = luaL_checkstring(L, 3);
        snprintf((char *) t->tabstring, sizeof(t->tabstring),
		 "%s", str);
        return 0;
    }
  }
  
  return newindexcommon(L);
}

static int
ltextboxsetfont(lua_State *L)
{
  struct textbox *t = obj_ref_check(L, 1, "mace.textbox");
  const char *pattern;
  size_t len;
  int r;

  pattern = luaL_checklstring(L, 2, &len);

  r = fontset(t->font, pattern);

  if (r == 0) {
    textboxcalcpositions(t, SEQ_start);
    textboxfindstart(t);
    textboxpredraw(t);
  }
  
  lua_pushnumber(L, r);

  return 1;
}

static int
ltextboxselections(lua_State *L)
{
  struct textbox *t = obj_ref_check(L, 1, "mace.textbox");
  
  lua_newtable(L);

  pushtextboxselections(L, 1, t);

  return 1;
}

/* This is a horrible way to do this. Selections should be their own
   type. They don't need to be handled the same way as other mace
   types though. Should make them plain user data. */

static int
ltextboxremoveselection(lua_State *L)
{
  struct selection *sel, *psel;
  struct textbox *t, *st;
  size_t start, len;

  t = obj_ref_check(L, 1, "mace.textbox");

  lua_getfield(L, 2, "start");
  start = luaL_checkinteger(L, -1);

  lua_getfield(L, 2, "len");
  len = luaL_checkinteger(L, -1);

  lua_getfield(L, 2, "textbox");
  st = obj_ref_check(L, -1, "mace.textbox");

  if (st != t) {
    printf("textbox remove selection for wrong textbox.\n");
    return 0;
  }

  psel = NULL;
  for (sel = t->selections; sel != NULL; sel = sel->next) {
    if (sel->start != start) continue;
    if (sel->end != start + len - 1) continue;
    if (sel->textbox != st) continue;

    if (psel == NULL) {
      t->selections = sel->next;
    } else {
      psel->next = sel->next;
    }

    selectionfree(sel);
    break;
  }

  textboxcalcpositions(t, start);
  textboxfindstart(t);
  textboxpredraw(t);

  return 0;
}

static int
ltextboxinsert(lua_State *L)
{
  const uint8_t *data;
  struct textbox *t;
  size_t pos, len;
  bool r;
  
  t = obj_ref_check(L, 1, "mace.textbox");
  pos = luaL_checkinteger(L, 2);
  data = (const uint8_t *) luaL_checklstring(L, 3, &len);

  r = sequenceinsert(t->sequence, pos, data, len);

  textboxcalcpositions(t, pos);
  textboxfindstart(t);
  textboxpredraw(t);
  
  lua_pushboolean(L, r);

  return 1;
}

static int
ltextboxdelete(lua_State *L)
{
  struct textbox *t;
  size_t start, len;
  bool r;
  
  t = obj_ref_check(L, 1, "mace.textbox");
  start = luaL_checkinteger(L, 2);
  len = luaL_checkinteger(L, 3);

  r = sequencedelete(t->sequence, start, len);

  textboxcalcpositions(t, start);
  textboxfindstart(t);
  textboxpredraw(t);
  
  lua_pushboolean(L, r);
  
  return 1;
}

static int
ltextboxget(lua_State *L)
{
  size_t start, len;
  struct textbox *t;
  uint8_t *buf;

  t = obj_ref_check(L, 1, "mace.textbox");
  start = luaL_checkinteger(L, 2);
  len = luaL_checkinteger(L, 3);

  if (len == 0) {
    lua_pushnil(L);
    return 1;
  }
  
  buf = malloc(len);
  if (buf == NULL) {
    lua_pushnil(L);
    return 1;
  }

  len = sequenceget(t->sequence, start, buf, len);
  lua_pushlstring(L, (const char *) buf, len);

  free(buf);
  
  return 1;
}

static int
ltextboxlen(lua_State *L)
{
  struct textbox *t;

  t = obj_ref_check(L, 1, "mace.textbox");

  lua_pushnumber(L, sequencegetlen(t->sequence));

  return 1;
}

static const struct luaL_Reg textbox_funcs[] = {
  { "__tostring",    ltextboxtostring },
  { "__index",       ltextboxindex },
  { "__newindex",    ltextboxnewindex },

  { "setfont",       ltextboxsetfont },

  { "selections",    ltextboxselections },
  { "removeselection",ltextboxremoveselection },

  { "insert",        ltextboxinsert },
  { "delete",        ltextboxdelete },
  { "get",           ltextboxget },
  { "len",           ltextboxlen },

  { NULL,         NULL },
};

void
luaremove(lua_State *L, void *addr)
{
  lua_pushnil(L);
  obj_ref_set(L, addr);
}

void
command(lua_State *L, const uint8_t *s)
{
  lua_getglobal(L, (const char *) s);

  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    fprintf(stderr, "error calling %s: %s\n", s, lua_tostring(L, -1));
    return;
  }
}

static void
addpath(lua_State *L, const char *path)
{
  lua_getglobal(L, "package");
  lua_pushstring(L, path);
  lua_pushstring(L, "/?.lua;");
  lua_getfield(L, -3, "path");
  lua_concat(L, 3);
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);
}

lua_State *
luanew(struct mace *mace)
{
  const char *home, *xdg, *epath;
  char path[PATH_MAX];
  struct passwd *pw;
  lua_State *L;

  L = luaL_newstate();
  if (L == NULL) {
    fprintf(stderr, "Failed to initalize lua!");
    return NULL;
  }

  luaL_openlibs(L);

  /* Give lua some directories to look in for mace's init file and
     other modules. */
  
  addpath(L, MACE_PATH);

  home = getenv("HOME");
  if (home == NULL || home[0] == 0) {
    pw = getpwuid(getuid());
    if (pw != NULL) {
      home = pw->pw_dir;
    }
  }

  xdg = getenv("XDG_CONFIG_HOME");
  if (xdg != NULL && xdg[0] != 0) {
    snprintf(path, sizeof(path), "%s/mace", xdg);
    addpath(L, path);
  } else if (home != NULL && home[0] != 0) {
    snprintf(path, sizeof(path), "%s/.config/mace", home);
    addpath(L, path);
  }

  epath = getenv("MACE_PATH");
  if (epath != NULL && epath[0] != 0) {
    addpath(L, getenv("MACE_PATH"));
  }

  /* Create tables in lua's registry to store binding data. */
  
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "mace.types");

  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "mace.objects");

  obj_type_new(L, "mace");
  luaL_setfuncs(L, mace_funcs, 0);

  obj_type_new(L, "mace.pane");
  luaL_setfuncs(L, pane_funcs, 0);

  obj_type_new(L, "mace.tab");
  luaL_setfuncs(L, tab_funcs, 0);
  
  obj_type_new(L, "mace.textbox");
  luaL_setfuncs(L, textbox_funcs, 0);

  obj_ref_new(L, mace, "mace");
  lua_setglobal(L, "mace");

  return L;
}

void
lualoadrc(lua_State *L)
{
  lua_getglobal(L, "require");
  lua_pushstring(L, "macerc");

  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    fprintf(stderr, "Error loading macerc: %s\n",
	    lua_tostring(L, -1));
  }
}

void
luafree(lua_State *L)
{
  lua_close(L);
}
