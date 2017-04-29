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

static bool
nextline(struct textbox *t, int *x, int *y)
{
  cairo_surface_t *sfc, *osfc;
  cairo_t *cr, *ocr;
  int w, h;
  
  *x = 0;
  *y += lineheight;

  h = cairo_image_surface_get_height(t->sfc);
  
  if (*y + lineheight < h) {
    t->height = *y + lineheight;
    return true;
  }

  printf("grow textbox surface\n");

  osfc = t->sfc;
  ocr = t->cr;
  
  w = cairo_image_surface_get_width(osfc);

  sfc = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				   w,
				   *y + lineheight * 30);
  if (sfc == NULL) {
    return false;
  }

  cr = cairo_create(sfc);
  if (cr == NULL) {
    cairo_surface_destroy(sfc);
    return false;
  }

  cairo_set_source_surface(cr, osfc, 0, 0);
  cairo_rectangle(cr, 0, 0, w, h);
  cairo_fill(cr);

  t->sfc = sfc;
  t->cr = cr;

  cairo_destroy(ocr);
  cairo_surface_destroy(osfc);

  t->height = *y + lineheight;

  return true;
}

static void 
drawglyph(struct textbox *t, int x, int y, int32_t pos)
{
  uint8_t buf[1024]; /* Hopefully this is big enough */
  FT_Bitmap *map = &face->glyph->bitmap;
  cairo_surface_t *s;
  int stride, h;

  struct colour *fg, *bg;
  struct colour test1 = { 0.1, 0.5, 1.0 };
  struct colour test2 = { 1.0, 0.5, 1.0 };

  if (t->cursor == pos) {
    fg = &test1;
    bg = &test2;
  } else {
    fg = &test2;
    bg = &test1;
  }

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

  cairo_set_source_rgb(t->cr, bg->r, bg->g, bg->b);
  cairo_rectangle(t->cr, x, y, 
		  face->glyph->advance.x >> 6, lineheight);

  cairo_fill(t->cr);
  
  cairo_set_source_rgb(t->cr, fg->r, fg->g, fg->b);

  cairo_mask_surface(t->cr, s, x + face->glyph->bitmap_left,
		     y + baseline - face->glyph->bitmap_top);

  cairo_surface_destroy(s);
}

bool
textboxpredraw(struct textbox *t)
{
  int32_t code, i, a, pos;
  int x, y, ww;
  struct piece *p;

  cairo_set_source_rgb(t->cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(t->cr);

  pos = 0;
  x = 0;

  /* Hacky */
  y = -lineheight;
  if (!nextline(t, &x, &y)) {
    return false;
  }
  
  for (p = t->pieces; p != NULL; p = p->next) {
    for (a = 0, i = 0; i < p->pl; i += a, pos += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }
      
      if (islinebreak(code, p->s + i, p->pl - i, &a)) {
	if (!nextline(t, &x, &y)) {
	  return false;
	}
      }
      
      if (!loadglyph(code)) {
	continue;
      }

      ww = face->glyph->advance.x >> 6;

      if (x + ww >= cairo_image_surface_get_width(t->sfc)) {
	if (!nextline(t, &x, &y)) {
	  return false;
	}
      }

      drawglyph(t, x, y, pos);

      x += ww;
    }
  }

  return true;
} 
