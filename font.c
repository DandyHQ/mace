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

static FT_Library library;

void
fontinit(void)
{
  int e;
  
  e = FT_Init_FreeType(&library);
  if (e != 0) {
    err(e, "Failed to initialize freetype2 library");
  }

  /* TODO: Load a default font. */
  fontload("/usr/X11R6/lib/X11/fonts/TTF/DejaVuSans.ttf", 15);
}

int
fontload(const uint8_t *name, size_t size)
{
  int e;

  e = FT_New_Face(library, (const char *) name, 0, &face);
  if (e != 0) {
    /* TODO: Load default font. */
    return e;
  }

  e = FT_Set_Pixel_Sizes(face, 0, size);
  if (e != 0) {
    /* TODO: Load default font. */
    return e;
  }

  baseline = 1 + ((face->size->metrics.height
		   + face->size->metrics.descender) >> 6);

  lineheight = 2 + (face->size->metrics.height >> 6);

  return 0;
}

bool
loadglyph(int32_t code)
{
  FT_UInt i;
  int e;

  i = FT_Get_Char_Index(face, code);
  if (i == 0) {
    return false;
  }

  e = FT_Load_Glyph(face, i, FT_LOAD_DEFAULT);
  if (e != 0) {
    return false;
  }

  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
    return false;
  }

  return true;
}

