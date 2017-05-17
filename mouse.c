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

void
handlebuttonpress(int x, int y, int button)
{
  struct tab *f = mace->focus->tab;
  
  tabbuttonpress(f, x - f->x, y - f->y, button);
}

void
handlebuttonrelease(int x, int y, int button)
{
  struct tab *f = mace->focus->tab;

  tabbuttonrelease(f, x - f->x, y - f->y, button);
}

void
handlemotion(int x, int y)
{
  struct tab *f = mace->focus->tab;

  tabmotion(f, x - f->x, y - f->y);
}

void
handlescroll(int x, int y, int dy)
{
  struct tab *f = mace->focus->tab;

  tabscroll(f, x - f->x, y - f->y, dy);
}

