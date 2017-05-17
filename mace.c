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

struct mace *mace;

struct mace *
macenew(cairo_t *cr)
{
  const uint8_t name[] = "Mace";
  struct mace *m;

  m = calloc(1, sizeof(struct mace));
  if (m == NULL) {
    return NULL;
  }

  m->cr = cr;

  m->fontlibrary = NULL;
  m->fontface = NULL;
  m->baseline = 0;
  m->lineheight = 0;
  
  m->tabs = tabnewempty(m, name, strlen(name));
  if (m->tabs == NULL) {
    free(m);
    return NULL;
  }

  m->focus = m->tabs->main;
  m->running = true;

  if (fontinit(m) != 0) {
    macefree(m);
    return NULL;
  }
  
  if (luainit(m) != 0) {
    macefree(m);
    return NULL;
  }

  return m;
}

void
macequit(struct mace *m)
{
  m->running = false;
}

void
macefree(struct mace *m)
{
  struct tab *t, *tn;

  while (m->selections != NULL) {
    selectionremove(m->selections);
  } 

  t = m->tabs;
  while (t != NULL) {
    tn = t->next;
    tabfree(t);
    t = tn;
  }

  /* Thing that created the mace instance deals with m->cr as it
     wishes. */

  luaend(m);
  fontend(m);

  free(m);
}
