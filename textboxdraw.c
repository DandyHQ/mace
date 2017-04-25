#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

static void
textboxdrawglyph(struct textbox *t, int x, int y, int ww,
		 struct colour *fg, struct colour *bg)
{
  drawrect(t->buf, t->width, t->rheight,
	   x, y,
	   x + ww, y + lineheight - 1,
	   bg);
  
  drawglyph(t->buf, t->width, t->rheight,
	    x + face->glyph->bitmap_left,
	    y + baseline - face->glyph->bitmap_top,
	    0, 0,
	    face->glyph->bitmap.width, face->glyph->bitmap.rows,
	    fg);
}

static void
drawnormal(struct textbox *t, int x, int y, int ww)
{
  textboxdrawglyph(t, x, y, ww, &fg, &t->bg);
}

static void
drawselected(struct textbox *t, int x, int y, int ww,
	     struct selection *s)
{
  textboxdrawglyph(t, x, y, ww, &t->sfg, &t->sbg);
}

static void
drawcursor(struct textbox *t, int x, int y, int ww, bool focus)
{
  struct colour *cfg, *cbg;

  if (focus) {
    cfg = &t->bg;
    cbg = &fg;
  } else {
    cfg = &fg;
    cbg = &t->bg;
  }

  textboxdrawglyph(t, x, y, ww, cfg, cbg);

  if (!focus) {
    drawline(t->buf, t->width, t->rheight,
	     x, y + 1,
	     x + ww - 1, y + 1,
	     cfg);

    drawline(t->buf, t->width, t->rheight,
	     x, y + lineheight - 2,
	     x + ww - 1, y + lineheight - 2,
	     cfg);

    drawline(t->buf, t->width, t->rheight,
	     x, y + 1,
	     x, y + lineheight - 2,
	     cfg);

    drawline(t->buf, t->width, t->rheight,
	     x + ww - 1, y + 1,
	     x + ww - 1, y + lineheight - 2,
	     cfg);
  }
}

static bool
endofline(struct textbox *t, int *x, int *y,
	  struct colour *bg)
{
  int h;
  
  drawrect(t->buf, t->width, t->rheight,
	   *x, *y,
	   t->width - 1,
	   *y + lineheight - 1,
	   bg);
 
  if (*y + lineheight * 2 >= t->rheight) {
    h = t->rheight + lineheight * 10;

    t->buf = realloc(t->buf, t->width * h * sizeof(uint8_t) * 4);
    if (t->buf == NULL) {
      return false;
    }

    t->rheight = h;
  }
  
  *x = 0;
  *y += lineheight;

  t->height = *y;

  return true;
}

static bool
textboxlinebreak(struct textbox *t, unsigned int pos,
		 int *x, int *y)
{
  struct colour *cbg;
  struct selection *s;
  int ww;

  if (pos == t->cursor) {
    if (loadglyph((int32_t) ' ')) {
      ww = face->glyph->advance.x >> 6;
      if (*x + ww < t->width) {
	drawcursor(t, *x, *y, ww, t == focus);
      }

      *x += ww;
    }
  }

  s = inselections(t, pos);
  if (s != NULL) {
    cbg = &t->sbg;
  } else {
    cbg = &t->bg;
  }

  return endofline(t, x, y, cbg);
}

void
textboxpredraw(struct textbox *t)
{
  int32_t code, a, pos;
  struct selection *s;
  int i, x, y, ww;
  struct piece *p;

  x = 0;
  y = 0;
  pos = 0;

  for (p = t->pieces; p != NULL; p = p->next) {
    for (i = 0; i < p->pl; pos += a, i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      if (islinebreak(code, p->s + i, p->pl - i, &a)) {
	if (!textboxlinebreak(t, pos, &x, &y)) {
	  return;
	} else {
	  continue;
	}
      }

      if (!loadglyph(code)) {
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      /* Wrap line */
      if (x + ww >= t->width) {
	if (!endofline(t, &x, &y, &t->bg)) {
	  return;
	}
      }

      if (pos == t->cursor) {
	drawcursor(t, x, y, ww, t == focus);
      } else if ((s = inselections(t, pos)) != NULL) {
	drawselected(t, x, y, ww, s);
      } else {
	drawnormal(t, x, y, ww);
      }
      
      x += ww;
    }
  }

  textboxlinebreak(t, pos, &x, &y);
} 
