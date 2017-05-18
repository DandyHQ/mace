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
macenew(void)
{
  const uint8_t name[] = "Mace";
  struct mace *m;

  m = calloc(1, sizeof(struct mace));
  if (m == NULL) {
    return NULL;
  }

  m->font = fontnew();
  if (m->font == NULL) {
    macefree(m);
    return NULL;
  }

  m->lua = luanew(m);
  if (m->lua == NULL) {
    macefree(m);
    return NULL;
  }

  m->tabs = tabnewempty(m, name, strlen(name));
  if (m->tabs == NULL) {
    free(m);
    return NULL;
  }

  m->focus = m->tabs->main;
  m->running = true;

  luaruninit(m->lua);
  
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

  t = m->tabs;
  while (t != NULL) {
    tn = t->next;
    tabfree(t);
    t = tn;
  }

  if (m->lua != NULL) {
    luafree(m->lua);
  }

  if (m->font != NULL) {
    fontfree(m->font);
  }

  free(m);
}

bool
handlebuttonpress(struct mace *mace, int x, int y, int button)
{
  struct tab *f = mace->focus->tab;
  
  return tabbuttonpress(f, x - f->x, y - f->y, button);
}

bool
handlebuttonrelease(struct mace *mace, int x, int y, int button)
{
  struct tab *f = mace->focus->tab;

  return tabbuttonrelease(f, x - f->x, y - f->y, button);
}

bool
handlemotion(struct mace *mace, int x, int y)
{
  struct tab *f = mace->focus->tab;

  return tabmotion(f, x - f->x, y - f->y);
}

bool
handlescroll(struct mace *mace, int x, int y, int dy)
{
  struct tab *f = mace->focus->tab;

  return tabscroll(f, x - f->x, y - f->y, dy);
}

bool
handletyping(struct mace *mace, uint8_t *s, size_t n)
{
  return textboxtyping(mace->focus, s, n);
}

bool
handlekeypress(struct mace *mace, keycode_t k)
{
  return textboxkeypress(mace->focus, k);
}

bool
handlekeyrelease(struct mace *mace, keycode_t k)
{
  return textboxkeyrelease(mace->focus, k);
}
