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

#include <fontconfig/fontconfig.h>

bool
fontinit(struct mace *mace)
{
  if (FT_Init_FreeType(&mace->fontlibrary) != 0) {
    fprintf(stderr, "Failed to initialize freetype2 library");
    return false;
  }

  if (!fontset(mace, "-15")) {
    fprintf(stderr, "Failed to load default font!\n");
    return false;
  }

  return true;
}

bool
fontset(struct mace *mace, const uint8_t *spattern)
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

  printf("use fontlib in mace %p\n", mace);
  e = FT_New_Face(mace->fontlibrary, file, 0, &mace->fontface);
  if (e != 0) {
    goto err2;
  }

  printf("use fontface\n");
  e = FT_Set_Pixel_Sizes(mace->fontface, 0, size);
  if (e != 0) {
    goto err2;
  }

  mace->baseline = 1 + ((mace->fontface->size->metrics.height
			 + mace->fontface->size->metrics.descender) >> 6);

  mace->lineheight = 2 + (mace->fontface->size->metrics.height >> 6);

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

  i = FT_Get_Char_Index(mace->fontface, code);
  if (i == 0) {
    return false;
  }

  e = FT_Load_Glyph(mace->fontface, i, FT_LOAD_DEFAULT);
  if (e != 0) {
    return false;
  }

  if (FT_Render_Glyph(mace->fontface->glyph,
		      FT_RENDER_MODE_NORMAL) != 0) {
    return false;
  }

  return true;
}

