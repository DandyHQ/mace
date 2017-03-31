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
linebreak(int32_t code, uint8_t *s, int32_t max, int32_t *l)
{
  size_t a;
  
  switch (code) {
  default:
    return false;

  case 0x000A: /* Line Feed */
    *l = 0;
    return true;

  case 0x000B: /* Vertical Tab */
    *l = 0;
    return true;

  case 0x000C: /* Form Feed */
    *l = 0;
    return true;

  case 0x000D: /* Carriage Return */
    a = utf8proc_iterate(s, max, &code);
    if (a > 0 && code == 0x000A) {
      /* Carriage Return Followed by Line Feed. */
      *l = a;
      return true;
    } else {
      *l = 0;
      return true;
    }

  case 0x0085: /* Next Line */
    *l = 0;
    return true;

  case 0x2028: /* Line Separator */
    *l = 0;
    return true;

  case 0x2029: /* Paragraph Separator */
    *l = 0;
    return true;
  }
}
