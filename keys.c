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
handletyping(uint8_t *s, size_t n)
{
  return textboxtyping(mace->focus, s, n);
}

bool
handlekeypress(keycode_t k)
{
  return textboxkeypress(mace->focus, k);
}

bool
handlekeyrelease(keycode_t k)
{
  return textboxkeyrelease(mace->focus, k);
}
