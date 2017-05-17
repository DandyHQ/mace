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

static struct colour nfg = { 0, 0, 0 };
static struct colour sbg = { 0.5, 0.8, 0.7 };

static void 
drawglyph(struct textbox *t, int x, int y, int32_t pos,
	  struct colour *fg, struct colour *bg)
{
  uint8_t buf[1024]; /* Hopefully this is big enough */
  FT_Bitmap *map = &mace->fontface->glyph->bitmap;
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

  cairo_set_source_rgb(t->cr, bg->r, bg->g, bg->b);
  cairo_rectangle(t->cr, x, y, 
		  mace->fontface->glyph->advance.x >> 6,
		  mace->lineheight);

  cairo_fill(t->cr);
  
  cairo_set_source_rgb(t->cr, fg->r, fg->g, fg->b);

  cairo_mask_surface(t->cr, s, x + mace->fontface->glyph->bitmap_left,
		     y + mace->baseline
		     - mace->fontface->glyph->bitmap_top);

  cairo_surface_destroy(s);
}

static void
drawcursor(struct textbox *t, int x, int y)
{
  cairo_set_source_rgb(t->cr, 0, 0, 0);
  cairo_set_line_width (t->cr, 1.0);

  cairo_move_to(t->cr, x, y);
  cairo_line_to(t->cr, x, y + mace->lineheight - 1);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y);
  cairo_line_to(t->cr, x + 2, y);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y + mace->lineheight - 1);
  cairo_line_to(t->cr, x + 2, y + mace->lineheight - 1);
  cairo_stroke(t->cr);
}

void
textboxpredraw(struct textbox *t)
{
  int32_t code, i, a, pos;
  struct selection *sel;
  struct sequence *s;
  struct colour *bg;
  int x, y, ww;
  size_t p;

  s = t->sequence;
  
  cairo_set_source_rgb(t->cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(t->cr);
 
  pos = 0;
  x = 0;
  y = -t->yoff;

  for (p = SEQ_start; p != SEQ_end; p = s->pieces[p].next) {
    for (a = 0, i = 0; i < s->pieces[p].len; i += a, pos += a) {
      a = utf8proc_iterate(s->data + s->pieces[p].off + i,
			   s->pieces[p].len - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      sel = inselections(t, pos);
      
      if (islinebreak(code,
		      s->data + s->pieces[p].off + i,
		      s->pieces[p].len - i, &a)) {

	if (sel != NULL
	    && y + mace->lineheight >= 0
	    && y < t->maxheight) {

	  cairo_set_source_rgb(t->cr, sbg.r, sbg.g, sbg.b);
	  cairo_rectangle(t->cr, x, y, t->linewidth - x,
			  mace->lineheight);
	  cairo_fill(t->cr);
	}
	    
	if (pos == t->cursor) {
	  drawcursor(t, x, y);
	}

	x = 0;
	y += mace->lineheight;
      }
      
      if (!loadglyph(code)) {
	continue;
      }

      ww = mace->fontface->glyph->advance.x >> 6;

      if (x + ww >= t->linewidth) {
	if (sel != NULL
	    && y + mace->lineheight >= 0
	    && y < t->maxheight) {

	  cairo_set_source_rgb(t->cr, sbg.r, sbg.g, sbg.b);

	  cairo_rectangle(t->cr, x, y,
			  t->linewidth - x,
			  mace->lineheight);

	  cairo_fill(t->cr);
	}

	x = 0;
	y += mace->lineheight;
      }

      if (y + mace->lineheight >= 0 && y < t->maxheight) {
	if (sel != NULL) {
	  bg = &sbg;
	} else {
	  bg = &t->bg;
	}
      
	drawglyph(t, x, y, pos, &nfg, bg);

	if (pos == t->cursor) {
	  drawcursor(t, x, y);
	}
      }

      x += ww;
    }
  }

  sel = inselections(t, pos);
  if (sel != NULL
      && y + mace->lineheight >= 0
      && y < t->maxheight) {

    cairo_set_source_rgb(t->cr, sbg.r, sbg.g, sbg.b);
    cairo_rectangle(t->cr, x, y, t->linewidth - x,
		    mace->lineheight);
    cairo_fill(t->cr);
  }
  
  if (pos == t->cursor) {
    drawcursor(t, x, y);
  }

  t->height = t->yoff + y + mace->lineheight;
} 
