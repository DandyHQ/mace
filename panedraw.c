#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

int
panedrawtablist(struct pane *p)
{
  struct tab *t;
  int xo, x, w;

  xo = p->norm.loff;

  for (t = p->norm.tabs; t != NULL && xo < p->width; t = t->next) {
    if (xo + tabwidth > 0) {
      if (xo < 0) {
	x = -xo;
      } else {
	x = 0;
      }

      if (xo + tabwidth < p->width) {
	w = tabwidth;
      } else {
	w = p->width - xo;
      }

      drawprerender(buf, width, height,
		    p->x + xo + x, p->y,
		    t->buf, tabwidth, lineheight,
		    x, 0,
		    w, lineheight);

      if (p->norm.focus == t) {
	drawline(buf, width, height,
		 p->x + xo + x, p->y + lineheight - 1,
		 p->x + xo + w, p->y + lineheight - 1,
		 &bg);
      } else {
	drawline(buf, width, height,
		 p->x + xo + x, p->y + lineheight - 1,
		 p->x + xo + w, p->y + lineheight - 1,
		 &fg);
      }
    }

    xo += tabwidth;
  }

  if (xo > 0 && xo < p->width) {
    drawrect(buf, width, height,
	     p->x + xo, p->y,
	     p->x + p->width, p->y + lineheight - 1,
	     &bg);
    
    drawline(buf, width, height,
	     p->x + xo - 1, p->y,
	     p->x + p->width, p->y,
	     &fg);

    drawline(buf, width, height,
	     p->x + xo - 1, p->y + lineheight - 1,
	     p->x + p->width, p->y + lineheight - 1,
	     &fg);
  }

  drawline(buf, width, height,
	   p->x + p->width, p->y,
	   p->x + p->width, p->y + lineheight,
	   &fg);
 
  drawline(buf, width, height,
	   p->x, p->y,
	   p->x, p->y + lineheight,
	   &fg);

  return lineheight;
}

static void
drawactionoutline(struct pane *p, int y, int yy)
{
  drawline(buf, width, height,
	   p->x + 1, p->y + yy - 1,
	   p->x + p->width - 1, p->y + yy - 1,
	   &fg);

  drawline(buf, width, height,
	   p->x + p->width, p->y + y,
	   p->x + p->width, p->y + yy,
	   &fg);
 
  drawline(buf, width, height,
	   p->x, p->y + y,
	   p->x, p->y + yy,
	   &fg);
}

static void
drawmainoutline(struct pane *p, int y)
{
  drawrect(buf, width, height,
	   p->x, p->y + y,
	   p->x + p->width - 1, p->y + p->height - 1,
	   &bg);

  drawline(buf, width, height,
	   p->x + p->width, p->y + y,
	   p->x + p->width, p->y + p->height - 1,
	   &fg);
 
  drawline(buf, width, height,
	   p->x, p->y + y,
	   p->x, p->y + p->height - 1,
	   &fg);
}


int
panedrawaction(struct pane *p, int y)
{
  int i, xx, yy, ww;
  struct piece *tp;
  int32_t code, a;
  struct tab *t;

  t = p->norm.focus;
  yy = y;
  xx = 0;

  drawrect(buf, width, height,
	   p->x, p->y + yy,
	   p->x + p->width - 1, p->y + yy + lineheight,
	   &abg);

  for (tp = t->action; tp != NULL; tp = tp->next) {
    for (i = 0; i < tp->pl && tp->s[i] != 0; i += a) {
      a = utf8proc_iterate(tp->s + i, tp->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }
      
      if (code == '\n') {
	a = 1;
 	xx = 0;
	yy += lineheight;

	drawrect(buf, width, height,
		 p->x, p->y + yy,
		 p->x + p->width - 1, p->y + yy + lineheight,
		 &abg);

	continue;
      }
      
      if (!loadglyph(code)) {
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      if (PADDING + xx + ww >= p->width - PADDING) {
	xx = 0;
	yy += lineheight;

	drawrect(buf, width, height,
		 p->x, p->y + yy,
		 p->x + p->width - 1, p->y + yy + lineheight,
		 &abg);
      }

      drawglyph(buf, width, height,
		p->x + PADDING + xx + face->glyph->bitmap_left,
		p->y + yy + baseline - face->glyph->bitmap_top,
		0, 0,
		face->glyph->bitmap.width, face->glyph->bitmap.rows,
		&fg);

      xx += ww;
    }
  }

  yy += lineheight;

  drawactionoutline(p, y, yy);
  
  return yy;
}

void
panedrawmain(struct pane *p, int y)
{
  int i, xx, yy, ww, ty;
  struct piece *tp;
  int32_t code, a;
  struct tab *t;

  drawmainoutline(p, y);

  t = p->norm.focus;
  yy = 0;
  xx = 0;

  for (tp = t->main; tp != NULL; tp = tp->next) {
    for (i = 0; i < tp->pl && tp->s[i] != 0; i += a) {
      a = utf8proc_iterate(tp->s + i, tp->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      if (code == '\n') {
 	xx = 0;
	yy += lineheight;
	continue;
      }

      if (!loadglyph(code)) {
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      if (PADDING + xx + ww >= p->width - PADDING) {
	xx = 0;
	yy += lineheight;
      }

      if (y + yy - t->voff >= p->height) {
	return;
      }
      
      if (yy + lineheight < t->voff) {
	xx += ww;
	continue;
      } else if (yy < t->voff) {
	ty = t->voff - yy;
	yy = t->voff;
      } else {
	ty = 0;
      }

      drawglyph(buf, width, height,
		p->x + PADDING + xx + face->glyph->bitmap_left,
		p->y + y + yy - t->voff
		+ baseline - face->glyph->bitmap_top,
		0, ty,
		face->glyph->bitmap.width,
		face->glyph->bitmap.rows - ty,
		&fg);

      xx += ww;
    }

    yy += lineheight;
  }
}

void
panedraw(struct pane *p)
{
  int y;
  
  switch (p->type) {
  case PANE_norm:
    y = panedrawtablist(p);
    y = panedrawaction(p, y);
    panedrawmain(p, y);
    break;

  case PANE_hsplit:
  case PANE_vsplit:
    panedraw(p->split.a);
    panedraw(p->split.b);
    break;
  }
}
