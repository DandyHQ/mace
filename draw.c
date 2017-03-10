#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

struct colour bg = { 1.0f, 1.0f, 1.0f, 1.0f };
struct colour fg = { 0, 0, 0, 1.0f };

int listheight;
int scrollwidth = 15;
int tabwidth = 80;

static char
blend(float cf, float cb, float a)
{
  float b;

  b = (cf * a + cb * (1.0f - a));
  return (char) (255.0f * b);
}

static void
drawpixel(char *dest, int dw, int dh,
	  int x, int y, struct colour *c)
{
  unsigned char *bb;
  float r, g, b;

  bb = &dest[(x + y * dw) * 4];

  b = ((float) *(bb+0)) / 255.0f;
  g = ((float) *(bb+1)) / 255.0f;
  r = ((float) *(bb+2)) / 255.0f;

  *(bb+0) = blend(c->b, b, c->a);
  *(bb+1) = blend(c->g, g, c->a);
  *(bb+2) = blend(c->r, r, c->a);
} 

#define fixboundswh(dx, dy, sx, sy, w, h, dw, dh) \
{ \
  if (dx < 0) { \
    sx -= dx; \
    w += dx; \
    dx = 0; \
  } if (dy < 0) { \
    sy -= dy; \
    h += dy; \
    dy = 0; \
  } if (dx + w >= dw) { \
    w = dw - 1 - dx; \
  } if (dy + h >= dh) { \
    h = dh - 1 - dy; \
  } \
}

void
drawprerender(char *dest, int dw, int dh,
	      int dx, int dy,
	      char *src, int sw, int sh,
	      int sx, int sy,
	      int w, int h)
{
  char *d, *s;
  int xx, yy;

  fixboundswh(dx, dy, sx, sy, w, h, dw, dh);
 
  for (xx = 0; xx < w; xx++) {
    for (yy = 0; yy < h; yy++) {
      d = &dest[((dx + xx) + (dy + yy) * dw) * 4];
      s = &src[((sx + xx) + (sy + yy) * sw) * 4];

      *(d+0) = *(s+0);
      *(d+1) = *(s+1);
      *(d+2) = *(s+2);
      *(d+3) = *(s+3);
    }
  }
}

void
drawglyph(char *dest, int dw, int dh,
	  int dx, int dy,
	  int sx, int sy,
	  int w, int h,
	  struct colour *c)
{
  struct colour rc = { c->r, c->g, c->b, 0.0f };
  FT_Bitmap *map = &face->glyph->bitmap;
  int xx, yy;
  
  fixboundswh(dx, dy, sx, sy, w, h, dw, dh);

  for (xx = 0; xx < w; xx++) {
    for (yy = 0; yy < h; yy++) {
      rc.a = c->a *
	(float ) map->buffer[(sx + xx) + (sy + yy) * map->width]
	/ 255.0f;

      drawpixel(dest, dw, dh,
		dx + xx, dy + yy, &rc);
    }
  }
}

#define fixboundspp(x1, y1, x2, y2, dw, dh)	\
{ \
  if (x1 < 0) { \
    x1 = 0; \
  } else if (x1 >= dw) { \
    x1 = dw - 1; \
  } if (y1 < 0) { \
    y1 = 0; \
  } else if (y1 >= dh) { \
    y1 = dh - 1; \
  } if (x2 < 0) { \
    x2 = 0; \
  } else if (x2 >= dw) { \
    x2 = dw - 1; \
  } if (y2 < 0) { \
    y2 = 0; \
  } else if (y2 >= dh) { \
    y2 = dh - 1; \
  } \
}

void
drawrect(char *dest, int dw, int dh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c)
{
  int xx, yy;

  fixboundspp(x1, y1, x2, y2, dw, dh);
 
  for (xx = x1; xx <= x2; xx++) {
    for (yy = y1; yy <= y2; yy++) {
      drawpixel(dest, dw, dh,
		xx, yy, c);
    }
  }
}

/* Only works with vertical or horizontal lines */

void
drawline(char *dest, int dw, int dh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c)
{
  int xd, yd;

  fixboundspp(x1, y1, x2, y2, dw, dh);

  xd = x1 == x2 ? 0 : (x1 < x2 ? 1 : -1);
  yd = y1 == y2 ? 0 : (y1 < y2 ? 1 : -1);

  while (x1 != x2 || y1 != y2) {
    drawpixel(dest, dw, dh,
	      x1, y1, c);

    x1 += xd;
    y1 += yd;
  }
}

void
drawtablist(struct pane *p)
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
		    t->buf, tabwidth, listheight,
		    x, 0,
		    w, listheight);

      if (p->norm.focus == t) {
	drawline(buf, width, height,
		 p->x + xo + x, p->y + listheight - 1,
		 p->x + xo + w, p->y + listheight - 1,
		 &bg);
      } else {
	drawline(buf, width, height,
		 p->x + xo + x, p->y + listheight - 1,
		 p->x + xo + w, p->y + listheight - 1,
		 &fg);
      }
    }

    xo += tabwidth;
  }

  if (xo > 0 && xo < p->width) {
    drawrect(buf, width, height,
	     p->x + xo, p->y,
	     p->x + p->width, p->y + listheight - 1,
	     &bg);
    
    drawline(buf, width, height,
	     p->x + xo - 1, p->y,
	     p->x + p->width, p->y,
	     &fg);

    drawline(buf, width, height,
	     p->x + xo - 1, p->y + listheight - 1,
	     p->x + p->width, p->y + listheight - 1,
	     &fg);
  }
}

static void
drawfocus(struct pane *p)
{
  drawrect(buf, width, height,
	   p->x, p->y + listheight,
	   p->x + p->width - 1, p->y + p->height - 1,
	   &bg);

  drawline(buf, width, height,
	   p->x + p->width - scrollwidth, p->y + listheight + 1,
	   p->x + p->width - scrollwidth, p->y + p->height,
	   &fg);

  drawline(buf, width, height,
	   p->x + p->width - 1, p->y,
	   p->x + p->width - 1, p->y + p->height - 1,
	   &fg);
}

void
drawpane(struct pane *p)
{
  switch (p->type) {
  case PANE_norm:
    drawtablist(p);
    drawfocus(p);

    break;

  case PANE_hsplit:
  case PANE_vsplit:
    drawpane(p->split.a);
    drawpane(p->split.b);
    break;
  }
}
