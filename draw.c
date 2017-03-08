#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

FT_Library library;
FT_Face face;

struct colour bg = { 1.0f, 1.0f, 1.0f, 1.0f };
struct colour fg = { 0, 0, 0, 1.0f };

unsigned int fontsize;
unsigned int listheight;
unsigned int scrollwidth = 15;
unsigned int tabwidth = 80;

void
initrender(void)
{
  int e;
  
  e = FT_Init_FreeType(&library);
  if (e != 0) {
    err(e, "Failed to initialize freetype2 library!\n");
  }

  e = FT_New_Face(library,
		  "/usr/local/share/fonts/Liberation/LiberationMono-Regular.ttf",
		  0, &face);
  if (e != 0) {
    err(e, "Failed to load freetype2 face!\n");
  }

  fontsize = 15;
  listheight = fontsize + 5;
  
  e = FT_Set_Pixel_Sizes(face, 0, fontsize);
  if (e != 0) {
    err(e, "Failed to set character size!\n");
  }
}

static char
blend(float cf, float cb, float a)
{
  float b;

  b = (cf * a + cb * (1.0f - a));
  return (char) (255.0f * b);
}

static void
drawpixel(char *buf, int bw, int bh,
	  int x, int y, struct colour *c)
{
  unsigned char *bb;
  float r, g, b;

  if (0 <= x && x < bw 
      && 0 <= y && y < bh) {
  
    bb = &buf[(x + y * bw) * 4];

    b = ((float) *(bb+0)) / 255.0f;
    g = ((float) *(bb+1)) / 255.0f;
    r = ((float) *(bb+2)) / 255.0f;

    *(bb+0) = blend(c->b, b, c->a);
    *(bb+1) = blend(c->g, g, c->a);
    *(bb+2) = blend(c->r, r, c->a);
  }
} 

void
drawprerender(char *dest, int dw, int dh,
	      int dx, int dy,
	      char *src, int sw, int sh,
	      int sx, int sy,
	      int w, int h)
{
  int xx, yy;
  char *d, *s;
  
  for (xx = 0; xx < w; xx++) {
    for (yy = 0; yy < h; yy++) {
      if (0 < dx + xx && dx + xx < dw
	  && 0 < dy + yy && dy + yy < dh) {
	if (0 < sx + xx && sx + xx < sw
	    && 0 < sy + yy && sy + yy < sh) {

	  d = &dest[((dx + xx) + (dy + yy) * dw) * 4];
	  s = &src[((sx + xx) + (sy + yy) * sw) * 4];

	  *(d+0) = *(s+0);
	  *(d+1) = *(s+1);
	  *(d+2) = *(s+2);
	  *(d+3) = *(s+3);
	}
      }
    }
  }
}

void
drawglyph(char *buf, int bw, int bh,
	  int x, int y,
	  int xoff, int yoff,
	  int w, int h,
	  struct colour *c)
{
  struct colour rc = { c->r, c->g, c->b, 0.0f };
  FT_Bitmap *map = &face->glyph->bitmap;
  int xx, yy;
  
  for (xx = xoff; xx < w; xx++) {
    for (yy = yoff; yy < h; yy++) {
      if (x + xx < 0 || x + xx >= bw) {
	continue;
      } else if (y + yy < 0 || y + yy >= bh) {
	continue;
      } 

      rc.a = c->a * (float ) map->buffer[xx + yy * map->width]
	/ 255.0f;

      drawpixel(buf, bw, bh,
		x + xx, y + yy, &rc);
    }
  }
}

bool
loadchar(long c)
{
  FT_UInt i;
  int e;
  
  i = FT_Get_Char_Index(face, c);
  if (i == 0) {
    return false;
  }

  e = FT_Load_Glyph(face, i, FT_LOAD_DEFAULT);
  if (e != 0) {
    return false;
  }

  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
    return false;
  }

  return true;
}

void
drawrect(char *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c)
{
  int xx, yy;
  
  for (xx = x1; xx <= x2; xx++) {
    for (yy = y1; yy <= y2; yy++) {
      drawpixel(buf, bw, bh,
		xx, yy, c);
    }
  }
}

/* Only works with vertical or horizontal lines */

void
drawline(char *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c)
{
  int xd, yd;

  xd = x1 == x2 ? 0 : (x1 < x2 ? 1 : -1);
  yd = y1 == y2 ? 0 : (y1 < y2 ? 1 : -1);

  while (x1 != x2 || y1 != y2) {
    drawpixel(buf, bw, bh,
	      x1, y1, c);

    x1 += xd;
    y1 += yd;
  }
}

static void
drawtablist(struct pane *p)
{
  struct tab *t;
  int xo;

  xo = 0;

  for (t = p->norm.tabs; t != NULL; t = t->next) {
    drawprerender(buf, width, height,
		  p->x + xo, p->y,
		  t->buf, tabwidth, listheight,
		  0, 0,
		  tabwidth, listheight);

    if (p->norm.focus == t) {
      drawline(buf, width, height,
	       p->x + xo, p->y + listheight - 1,
	       p->x + xo + tabwidth, p->y + listheight - 1,
	       &bg);
    } else {
      drawline(buf, width, height,
	       p->x + xo, p->y + listheight - 1,
	       p->x + xo + tabwidth, p->y + listheight - 1,
	       &fg);
    }
   
    xo += tabwidth;
  }

  if (xo < p->width) {
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

void
redraw(void)
{
  drawpane(root);
}

