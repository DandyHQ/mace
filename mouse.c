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

bool
handlebuttonpress(int x, int y, int button)
{
  struct tab *f = mace->focus->tab;
  
  return tabbuttonpress(f, x - f->x, y - f->y, button);
}

bool
handlebuttonrelease(int x, int y, int button)
{
  struct tab *f = mace->focus->tab;

  return tabbuttonrelease(f, x - f->x, y - f->y, button);
}

bool
handlemotion(int x, int y)
{
  struct tab *f = mace->focus->tab;

  return tabmotion(f, x - f->x, y - f->y);
}

bool
handlescroll(int x, int y, int dy)
{
  struct tab *f = mace->focus->tab;

  return tabscroll(f, x - f->x, y - f->y, dy);
}

