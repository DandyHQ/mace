#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

void
handlebuttonpress(int x, int y, int button)
{
  tabbuttonpress(tab, x - tab->x, y - tab->y, button);
}

void
handlebuttonrelease(int x, int y, int button)
{
  tabbuttonrelease(tab, x - tab->x, y - tab->y, button);
}

void
handlemotion(int x, int y)
{
  tabmotion(tab, x - tab->x, y - tab->y);
}

void
handlescroll(int x, int y, int dy)
{
  tabscroll(tab, x - tab->x, y - tab->y, dy);
}

