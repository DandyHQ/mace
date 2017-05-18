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
drawglyph(struct textbox *t, int x, int y,
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
textboxpredraw(struct textbox *t,
	       bool startchanged, bool heightchanged)
{
  int32_t code, a, pos;
  struct selection *sel;
  struct sequence *s;
  struct colour *bg;
  bool startfound;
  int x, y, ww;
  size_t p, i;

  s = t->sequence;
  
  cairo_set_source_rgb(t->cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(t->cr);
 
  x = 0;

  if (startchanged) {
    t->startpos = 0;
    t->starty = -t->yoff;

    if (t->yoff < mace->lineheight) {
      startfound = true;
    } else {
      startfound = false;
    }
  } 

  p = sequencefindpiece(s, t->startpos, &i);
  y = t->starty;
  pos = s->pieces[p].pos;

  while (p != SEQ_end) {
    while (i < s->pieces[p].len) {
      a = utf8proc_iterate(s->data + s->pieces[p].off + i,
			   s->pieces[p].len - i, &code);
      if (a <= 0) {
	i++;
	continue;
      }

      sel = inselections(t, pos + i);
      
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
	    
	if (pos + i == t->cursor) {
	  drawcursor(t, x, y);
	}

	x = 0;
	y += mace->lineheight;

	if (!heightchanged && y >= t->maxheight) {
	  return;
	} else if (startchanged
		   && y + mace->lineheight >= 0
		   && !startfound) {

	  startfound = true;
	  t->startpos = pos + i + a;
	  t->starty = y;
	}
      }
      
      if (!loadglyph(code)) {
	i += a;
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
	/* TODO: add in code from line break */
      }

      if (y + mace->lineheight >= 0
	  && y < t->maxheight) {

	if (sel != NULL) {
	  bg = &sbg;
	} else {
	  bg = &t->bg;
	}
      
	drawglyph(t, x, y, &nfg, bg);

	if (pos + i == t->cursor) {
	  drawcursor(t, x, y);
	}
      }

      i += a;
      x += ww;
    }

    pos += i;
    p = s->pieces[p].next;
    i = 0;
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
