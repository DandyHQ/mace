#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "mace.h"
#include "config.h"

#include <fontconfig/fontconfig.h>

struct font *
fontnew(void)
{
  struct font *f;
  int e;

  f = malloc(sizeof(struct font));
  if (f == NULL) {
    return NULL;
  }

  e = FT_Init_FreeType(&f->library);
  if (e != 0) {
    fontfree(f);
    return NULL;
  }

  f->face = NULL;

  e = fontset(f, (const uint8_t *) "-15");
  if (e != 0) {
    fontfree(f);
    return NULL;
  }

  return f;
}

void
fontfree(struct font *font)
{
  if (font->face != NULL) {
    FT_Done_Face(font->face);
  }

  if (font->library != NULL) {
    FT_Done_FreeType(font->library);
  }

  free(font);
}

int
fontset(struct font *font, const uint8_t *spattern)
{
  FcPattern *pat, *fontpat;
  FcConfig *config;
  FcResult result;
  FcChar8 *file;
  double size;
  int e;

  config = FcInitLoadConfigAndFonts();
  if (config == NULL) {
    e = -1;
    goto err0;
  }

  pat = FcNameParse(spattern);
  if (pat == NULL) {
    e = -1;
    goto err0;
  }

  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  fontpat = FcFontMatch(config, pat, &result);
  if (fontpat == NULL) {
    e = -1;
    goto err1;
  }

  if (FcPatternGetString(fontpat, FC_FILE, 0, &file)
      != FcResultMatch) {
    e = -1;
    goto err2;
  }

  if (FcPatternGetDouble(fontpat, FC_SIZE, 0, &size)
      != FcResultMatch) {
    e = -1;
    goto err2;
  }

  if (font->face != NULL) {
    FT_Done_Face(font->face);
  }

  e = FT_New_Face(font->library, (const char *) file, 0, &font->face);
  if (e != 0) {
    goto err2;
  }

  e = FT_Set_Pixel_Sizes(font->face, 0, size);
  if (e != 0) {
    goto err2;
  }

  font->baseline = 1 + ((font->face->size->metrics.height
			 + font->face->size->metrics.descender) >> 6);

  font->lineheight = 2 + (font->face->size->metrics.height >> 6);

  return 0;

 err2:
  FcPatternDestroy(fontpat);
 err1:
  FcPatternDestroy(pat);
 err0:
  return e;
}

bool
loadglyph(FT_Face face, int32_t code)
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

  e = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  if (e != 0) {
    return false;
  }

  return true;
}

/* Should probably replace this with pango somehow */

void 
drawglyph(struct font *f, cairo_t *cr, int x, int y,
	  struct colour *fg, struct colour *bg)
{
  uint8_t buf[1024]; /* Hopefully this is big enough */
  FT_Bitmap *map = &f->face->glyph->bitmap;
  cairo_surface_t *s;
  int stride, h;

  /* The buffer needs to be in a format cairo accepts */

  stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, map->width);
  for (h = 0; h < map->rows; h++) {
    memmove(buf + h * stride,
	    map->buffer + h * map->width,
	    map->width);
  }

  s = cairo_image_surface_create_for_data(buf,
					  CAIRO_FORMAT_A8,
					  map->width,
					  map->rows,
					  stride);
  if (s == NULL) {
    return;
  }

  cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);
  cairo_rectangle(cr, x, y, 
		  f->face->glyph->advance.x >> 6,
		  f->lineheight);

  cairo_fill(cr);
  
  cairo_set_source_rgb(cr, fg->r, fg->g, fg->b);

  cairo_mask_surface(cr, s, x + f->face->glyph->bitmap_left,
		     y + f->baseline - f->face->glyph->bitmap_top);

  cairo_surface_destroy(s);
}

