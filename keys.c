#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

bool
handletyping(uint8_t *s, size_t n)
{
  if (focus != NULL && textboxtyping(focus, s, n)) {
    panedrawfocus(focuspane);
    return true;
  }

  return false;
}

bool
handlekeypress(keycode_t k)
{
  if (focus != NULL && textboxkeypress(focus, k)) {
    panedrawfocus(focuspane);
    return true;
  } else {
    return false;
  }
}

bool
handlekeyrelease(keycode_t k)
{
  if (focus != NULL && textboxkeyrelease(focus, k)) {
    panedrawfocus(focuspane);
    return true;
  } else {
    return false;
  }
}
