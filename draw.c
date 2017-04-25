#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

static uint8_t
blend(unsigned int cf, unsigned int cb, unsigned int a)
{
  return (uint8_t) ((cf * a + cb * (255 - a)) / 255);
}

static void
drawpixel(uint8_t *dest, int dw, int dh,
	  int x, int y, struct colour *c)
{
  uint8_t *bb;
  
  bb = &dest[(x + y * dw) * 4];

  *(bb+0) = blend(c->b, *(bb+0), c->a);
  *(bb+1) = blend(c->g, *(bb+1), c->a);
  *(bb+2) = blend(c->r, *(bb+2), c->a);
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
drawglyph(uint8_t *dest, int dw, int dh,
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
      rc.a = c->a
	* map->buffer[(sx + xx) + (sy + yy) * map->width]
	/ 255;

      drawpixel(dest, dw, dh,
		dx + xx, dy + yy, &rc);
    }
  }
}

void
drawbuffer(uint8_t *dest, int dw, int dh,
	   int dx, int dy,
	   uint8_t *src, int sw, int sh,
	   int sx, int sy,
	   int w, int h)
{
  uint8_t *d, *s;
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

int
drawstring(uint8_t *dest, int dw, int dh,
	   int dx, int dy,
	   int tx, int ty,
	   int tw, int th,
	   uint8_t *s, bool drawpart,
	   struct colour *c)
{
  int i, xx, ww, x, w;
  int32_t code;
  ssize_t a;

  i = 0;

  for (i = 0, xx = 0; s[i] != 0 && xx < tx + tw; xx += ww) {
    a = utf8proc_iterate(s + i, -1, &code);
    if (a <= 0) {
      i += 1;
      continue;
    }
    
    if (!loadglyph(code)) {
      continue;
    } else {
      i += a;
    }

    ww = face->glyph->advance.x >> 6;

    if (xx > tx) {
      x = 0;
    } else if (xx + ww > tx) {
      x = tx - xx;
    } else {
      continue;
    }

    if (xx + ww > tx + tw) {
      if (drawpart) {
	w = xx + ww - tx - tw;
      } else {
	i -= a;
	break;
      }
    } else {
      w = face->glyph->bitmap.width;
    }
    
    drawglyph(dest, dw, dh,
	      dx + xx + face->glyph->bitmap_left,
	      dy + baseline - face->glyph->bitmap_top,
	      x, ty,
	      w, face->glyph->bitmap.rows,
	      c);
  }

  return i;
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
drawrect(uint8_t *dest, int dw, int dh,
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

void
drawline(uint8_t *dest, int dw, int dh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c)
{
  int xd, yd;

  fixboundspp(x1, y1, x2, y2, dw, dh);

  xd = x1 == x2 ? 0 : (x1 < x2 ? 1 : -1);
  yd = y1 == y2 ? 0 : (y1 < y2 ? 1 : -1);

  while (true) {
    drawpixel(dest, dw, dh,
	      x1, y1, c);

    if (x1 == x2 && y1 == y2) {
      break;
    }
    
    x1 += xd;
    y1 += yd;
  }
}
