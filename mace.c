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

  m = malloc(sizeof(struct mace));
  if (m == NULL) {
    return NULL;
  }

  m->cr = cr;

  m->fontlibrary = NULL;
  m->fontface = NULL;
  m->baseline = 0;
  m->lineheight = 0;
  
  m->tab = tabnew(name, strlen(name));
  if (m->tab == NULL) {
    free(m);
    return NULL;
  }
  
  m->focus = m->tab->main;
  m->running = true;

  printf("init font\n");
  if (!fontinit(m)) {
    macefree(m);
    return NULL;
  }
  
  printf("init lua\n");
  if (!luainit(m)) {
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
  tabfree(m->tab);

  /* Free the rest of the stuff */

  free(m);
}
