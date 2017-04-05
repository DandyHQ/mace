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
textboxdrawglyph(struct textbox *t, uint8_t *dest, int dw, int dh,
		 int x, int y, int ww, int ly, int lh,
		 struct colour *fg, struct colour *bg)
{
  int by, bh;

  if (baseline - face->glyph->bitmap_top < ly) {
    by = ly - (baseline - face->glyph->bitmap_top);
  } else {
    by = 0;
  }

  bh = face->glyph->bitmap.rows - by;
  if (baseline - face->glyph->bitmap_top + by - ly + bh >= lh - 1) {
    bh = lh - 1 - (baseline - face->glyph->bitmap_top + by - ly);
  }

  drawrect(dest, dw, dh,
	   x, y + ly,
	   x + ww, y + ly + lh,
	   bg);
  
  drawglyph(dest, dw, dh,
	    x + face->glyph->bitmap_left,
	    y + baseline - face->glyph->bitmap_top + by,
	    0, by,
	    face->glyph->bitmap.width, bh,
	    fg);

}

static void
drawnormal(struct textbox *t, uint8_t *dest, int dw, int dh,
	   int x, int y, int ww, int ly, int lh)
{
  textboxdrawglyph(t, dest, dw, dh,
		   x, y, ww, ly, lh,
		   &fg, &t->bg);
}

static void
drawselected(struct textbox *t, uint8_t *dest, int dw, int dh,
	     int x, int y, int ww, int ly, int lh)
{
  textboxdrawglyph(t, dest, dw, dh,
		   x, y, ww, ly, lh,
		   &t->bg, &fg);
}

static void
drawcursor(struct textbox *t, uint8_t *dest, int dw, int dh,
	   int x, int y, int ww, int ly, int lh, bool focus)
{
  struct colour *cfg, *cbg;

  if (focus) {
    cfg = &t->bg;
    cbg = &fg;
  } else {
    cfg = &fg;
    cbg = &t->bg;
  }

  textboxdrawglyph(t, dest, dw, dh,
		   x, y, ww, ly, lh,
		   cfg, cbg);

  if (!focus) {
    drawline(dest, dw, dh,
	     x, y + ly + 1,
	     x + ww - 1, y + ly + 1,
	     &fg);

    drawline(dest, dw, dh,
	     x, y + ly + lh - 1,
	     x + ww - 1, y + ly + lh - 1,
	     &fg);

    drawline(dest, dw, dh,
	     x, y + ly + 1,
	     x, y + ly + lh - 1,
	     &fg);

    drawline(dest, dw, dh,
	     x + ww - 1, y + ly + 1,
	     x + ww - 1, y + ly + lh - 1,
	     &fg);
  }
}

static void
fixlinebounds(struct textbox *t, 
	      int *xx, int *yy,
	      int w, int h,
	      int *ly, int *lh)
{
  if (*yy - t->yscroll < 0) {
    *ly = t->yscroll - *yy;
    *lh = lineheight - *ly;
  } else if (*yy - t->yscroll + lineheight >= h) {
    *ly = 0;
    *lh = h - (*yy - t->yscroll);
  } else {
    *ly = 0;
    *lh = lineheight;
  }
}

static void
endofline(struct textbox *t, uint8_t *dest, int dw, int dh,
	  int x, int y, int *xx, int *yy,
	  int w, int h,
	  int *ly, int *lh)
{
  if (*yy - t->yscroll + lineheight + *ly >= 0) {
    drawrect(dest, dw, dh,
	     x + *xx, y + *yy - t->yscroll + *ly,
	     x + w - 1, y + *yy - t->yscroll + *ly + *lh - 1,
	     &t->bg);
  }
  
  *xx = TEXTBOX_PADDING;
  *yy += lineheight;

  fixlinebounds(t, xx, yy, w, h, ly, lh);

  if (*yy - t->yscroll + lineheight + *ly >= 0) {
    drawrect(dest, dw, dh,
	     x, y + *yy - t->yscroll + *ly,
	     x + *xx, y + *yy - t->yscroll + *ly + *lh - 1,
	     &t->bg);
  }
}

static void
textboxlinebreak(struct textbox *t, unsigned int pos,
		 uint8_t *dest, int dw, int dh,
		 int x, int y, int *xx, int *yy,
		 int w, int h,
		 int *ly, int *lh)
{
  int ww;
  
  if (pos == t->cursor) {
    if (loadglyph((int32_t) ' ')) {
      ww = face->glyph->advance.x >> 6;
      if (*xx + ww < w - TEXTBOX_PADDING) {
	drawcursor(t, dest, dw, dh,
		   x + *xx, y + *yy - t->yscroll,
		   ww, *ly, *lh - 1, focus);
      }

      *xx += ww;
    }
  }

  endofline(t, dest, dw, dh,
	    x, y, xx, yy, w, h,
	    ly, lh);
}

static bool
inselection(struct textbox *t, unsigned int pos)
{
  struct selection *s;

  for (s = t->selections; s != NULL; s = s->next) {
    if (s->start <= pos && pos <= s->end) {
      return true;
    }
  }

  return false;
}

void
textboxdraw(struct textbox *t, uint8_t *dest, int dw, int dh,
	    int x, int y, int w, int h, bool focus)
{
  int i, xx, yy, ww, ly, lh;
  int32_t code, a, pos;
  struct piece *p;

  yy = 0;
  xx = TEXTBOX_PADDING;
  pos = 0;
  ly = 0;
  lh = lineheight;

  fixlinebounds(t, &xx, &yy, w, h, &ly, &lh);

  if (yy - t->yscroll + lineheight + ly >= 0) {
    drawrect(dest, dw, dh,
	     x, y + yy - t->yscroll + ly,
	     x + xx, y + yy - t->yscroll + ly + lh - 1,
	     &t->bg);
  }

  for (p = t->pieces; p != NULL; p = p->next) {
    for (i = 0; i < p->pl; pos += a, i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      if (linebreak(code, p->s + i, p->pl - i, &a)) {
	textboxlinebreak(t, pos, dest, dw, dh,
			 x, y, &xx, &yy,
			 w, h, &ly, &lh);
	continue;
      }

      if (!loadglyph(code)) {
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      if (xx + ww >= w - TEXTBOX_PADDING) {/* Wrap line */
	endofline(t, dest, dw, dh,
		  x, y, &xx, &yy, w, h,
		  &ly, &lh);
      }

      if (yy - t->yscroll + lineheight >= 0
	  && yy - t->yscroll < h - 1) {

	if (pos == t->cursor) {
	  drawcursor(t, dest, dw, dh,
		     x + xx, y + yy - t->yscroll,
		     ww, ly, lh - 1, focus);

	} else if (inselection(t, pos)) {
	  drawselected(t, dest, dw, dh,
		       x + xx, y + yy - t->yscroll,
		       ww, ly, lh - 1);
	} else {
	  drawnormal(t, dest, dw, dh,
		     x + xx, y + yy - t->yscroll,
		     ww, ly, lh - 1);
	}
      }
      
      xx += ww;
    }
  }

  textboxlinebreak(t, pos, dest, dw, dh,
		   x, y, &xx, &yy,
		   w, h, &ly, &lh);
	
  t->width = w;
  t->height = yy - t->yscroll;
}

