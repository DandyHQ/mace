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

#include <fontconfig/fontconfig.h>

static FT_Library library;

void
fontinit(void)
{
  int e;
  
  e = FT_Init_FreeType(&library);
  if (e != 0) {
    err(e, "Failed to initialize freetype2 library");
  }

  fontset("-15");
}

bool
fontset(const uint8_t *spattern)
{
  FcPattern *pat, *font;
  FcConfig *config;
  FcResult result;
  FcChar8 *file;
  double size;
  int e;

  config = FcInitLoadConfigAndFonts();
  if (config == NULL) {
    goto err0;
  }
  
  pat = FcNameParse(spattern);
  if (pat == NULL) {
    goto err0;
  }
  
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  font = FcFontMatch(config, pat, &result);
  if (font == NULL) {
    goto err1;
  }
  
  if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch) {
    goto err2;
  }

  if (FcPatternGetDouble(font, FC_SIZE, 0, &size) != FcResultMatch) {
    goto err2;
  }

  e = FT_New_Face(library, file, 0, &face);
  if (e != 0) {
    goto err2;
  }

  e = FT_Set_Pixel_Sizes(face, 0, size);
  if (e != 0) {
    goto err2;
  }

  baseline = 1 + ((face->size->metrics.height
		   + face->size->metrics.descender) >> 6);

  lineheight = 2 + (face->size->metrics.height >> 6);

  return true;

 err2:
  FcPatternDestroy(font);
 err1:
  FcPatternDestroy(pat);
 err0:
  return false; 
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

