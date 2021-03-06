#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "mace.h"
#include "config.h"

#include <fontconfig/fontconfig.h>

static bool
fontloadfile(struct font *font, const char *path,
             double size);

struct font *
fontnew(void)
{
  struct font *f;
  int e;
  f = calloc(1, sizeof(struct font));

  if (f == NULL) {
    return NULL;
  }

  e = FT_Init_FreeType(&f->library);

  if (e != 0) {
    fontfree(f);
    return NULL;
  }

  f->tabwidth = DEFAULT_TAB_WIDTH;

  if (!fontset(f, DEFAULT_FONT_PATTERN)) {
    fontfree(f);
    return NULL;
  }

  return f;
}

void
fontfree(struct font *font)
{
  if (font->cface != NULL) {
    cairo_font_face_destroy(font->cface);
  }

  if (font->face != NULL) {
    FT_Done_Face(font->face);
  }

  if (font->library != NULL) {
    FT_Done_FreeType(font->library);
  }

  free(font);
}

bool
fontloadfile(struct font *font, const char *path,
             double size)
{
  FT_Face new;

  if (FT_New_Face(font->library, path, 0, &new) != 0) {
    return false;
  }

  if (FT_Set_Pixel_Sizes(new, size, size) != 0) {
    FT_Done_Face(new);
    return false;
  }

  if (font->cface != NULL) {
    cairo_font_face_destroy(font->cface);
    font->cface = NULL;
  }

  if (font->face != NULL) {
    FT_Done_Face(font->face);
    font->face = NULL;
  }

  snprintf(font->path, sizeof(font->path), "%s", path);
  font->size = size;
  font->face = new;
  font->cface =
    cairo_ft_font_face_create_for_ft_face(font->face,
                                          FT_LOAD_DEFAULT);
  font->baseline = 1 + ((font->face->size->metrics.height
                         + font->face->size->metrics.descender) >> 6);
  font->lineheight = (font->face->size->metrics.height >> 6);
  fontsettabwidth(font, font->tabwidth);
  return true;
}

bool
fontset(struct font *font, const char *spattern)
{
  FcPattern *pat, *fontpat;
  FcConfig *config;
  FcResult result;
  bool r = false;
  double size;
  char *path;
  config = FcInitLoadConfigAndFonts();

  if (config == NULL) {
    goto ERR;
  }

  pat = FcNameParse((const FcChar8 *) spattern);

  if (pat == NULL) {
    goto ERRCONFIG;
  }

  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);
  fontpat = FcFontMatch(config, pat, &result);

  if (fontpat == NULL) {
    goto ERRPAT;
  }

  if (FcPatternGetString(fontpat, FC_FILE, 0,
                         (FcChar8 **) &path)
      != FcResultMatch) {
    goto ERRFONTPAT;
  }

  if (FcPatternGetDouble(fontpat, FC_SIZE, 0, &size)
      != FcResultMatch) {
    goto ERRFONTPAT;
  }

  if (!fontloadfile(font, path, size)) {
    goto ERRFONTPAT;
  }

  r = true;
ERRFONTPAT:
  FcPatternDestroy(fontpat);
ERRPAT:
  FcPatternDestroy(pat);
ERRCONFIG:
  FcConfigDestroy(config);
ERR:
  return r;
}

bool
fontsettabwidth(struct font *font, size_t spaces)
{
  FT_UInt i;
  i = FT_Get_Char_Index(font->face, ' ');

  if (i == 0) {
    return false;
  }

  if (FT_Load_Glyph(font->face, i, FT_LOAD_DEFAULT) != 0) {
    return false;
  }

  font->tabwidth = spaces;
  font->tabwidthpixels = (font->face->glyph->advance.x >> 6) *
                         spaces;
  return true;
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
  stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8,
                                         map->width);

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

