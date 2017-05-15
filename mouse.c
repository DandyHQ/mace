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
  tabbuttonpress(mace->tab, x - mace->tab->x, y - mace->tab->y, button);
}

void
handlebuttonrelease(int x, int y, int button)
{
  tabbuttonrelease(mace->tab, x - mace->tab->x, y - mace->tab->y, button);
}

void
handlemotion(int x, int y)
{
  tabmotion(mace->tab, x - mace->tab->x, y - mace->tab->y);
}

void
handlescroll(int x, int y, int dy)
{
  tabscroll(mace->tab, x - mace->tab->x, y - mace->tab->y, dy);
}

