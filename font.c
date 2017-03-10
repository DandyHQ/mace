#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

FT_Library library;
FT_Face face;

int fontsize;

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
  listheight = fontsize + 5;
  
  e = FT_Set_Pixel_Sizes(face, 0, fontsize);
  if (e != 0) {
    err(e, "Failed to set character size!\n");
  }
}

bool
loadchar(long c)
{
  FT_UInt i;
  int e;
  
  i = FT_Get_Char_Index(face, c);
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

