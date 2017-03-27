#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
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
    err(e, "Failed to initialize freetype2 library!\n");
  }

  e = FT_New_Face(library,
		  "/usr/X11R6/lib/X11/fonts/TTF/DejaVuSansMono.ttf",
		  0, &face);
  if (e != 0) {
    err(e, "Failed to load freetype2 face!\n");
  }

  fontsize = 15;
  
  e = FT_Set_Pixel_Sizes(face, 0, fontsize);
  if (e != 0) {
    err(e, "Failed to set character size!\n");
  }
 
  /* This is apparently what it should be */
  baseline = face->size->metrics.height >> 6;
  /* This seems to work */
  lineheight = (face->ascender + face->descender) >> 6;
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

