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
handletyping(uint8_t *s, size_t n)
{
  textboxtyping(focus, s, n);
}

void
handlekeypress(keycode_t k)
{
  textboxkeypress(focus, k);
}

void
handlekeyrelease(keycode_t k)
{
  textboxkeyrelease(focus, k);
}
