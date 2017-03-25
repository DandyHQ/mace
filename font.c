#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

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

int
loadglyph(char *s)
{
  FT_UInt i;
  long n;
  int e;

  /* TODO: get utf8 code and number of bytes from string */
  n = *s;
  
  i = FT_Get_Char_Index(face, n);
  if (i == 0) {
    return -1;
  }

  e = FT_Load_Glyph(face, i, FT_LOAD_DEFAULT);
  if (e != 0) {
    return -1;
  }

  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
    return -1;
  }

  /* Return number of bytes */
  return 1;
}

