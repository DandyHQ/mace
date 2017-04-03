#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

#define PADDING 5

bool
textboxinit(struct textbox *t, struct colour *bg)
{
  struct piece *b, *e;

  b = piecenewtag();
  if (b == NULL) {
    return false;
  }

  e = piecenewtag();
  if (e == NULL) {
    piecefree(b);
    return false;
  }

  b->prev = NULL;
  b->next = e;
  e->prev = b;
  e->next = NULL;

  t->pieces = b;
  t->cursor = 0;

  t->xscroll = 0;
  t->yscroll = 0;

  t->width = 0;
  t->height = 0;

  memmove(&t->bg, bg, sizeof(struct colour));

  return true;
}

void
textboxfree(struct textbox *t)
{
  struct piece *p;

  /* TODO: Free removed pieces. */
  for (p = t->pieces; p != NULL; p = p->next)
    piecefree(p);
}

static void
drawcursor(int x, int y, int ww, bool focus)
{
  drawline(buf, width, height,
	   x, y + 2,
	   x, y + lineheight - 3,
	   &fg);
}

void
textboxdraw(struct textbox *t, uint8_t *dest, int dw, int dh,
	    int x, int y, int w, int h, bool focus)
{
  int32_t code, a, aa, pos;
  int i, xx, yy, ly, ww;
  struct piece *p;

  yy = 0;
  xx = PADDING;
  pos = 0;

  drawrect(dest, dw, dh,
	   x, y + yy,
	   x + w - 1, y + yy + lineheight - 1,
	   &t->bg);

  for (p = t->pieces; p != NULL; p = p->next) {
    if (p->s == NULL) continue;

    for (i = 0; i < p->pl; pos += a, i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      if (linebreak(code, p->s + i + a, p->pl - i - a, &aa)) {
	a += aa;
	xx = PADDING;
	yy += lineheight;

	drawrect(dest, dw, dh,
		 x, y + yy,
		 x + w - 1, y + yy + lineheight - 1,
		 &t->bg);
      }

      if (!loadglyph(code)) {
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      if (xx + ww >= w - PADDING) {
	xx = PADDING;
	yy += lineheight;

	drawrect(dest, dw, dh,
		 x, y + yy,
		 x + w - 1, y + yy + lineheight - 1,
		 &t->bg);
      }

      if (yy >= h) {
	goto done;
      } else if (yy + lineheight >= h) {
	ly = h - yy;
      } else {
	ly = 0;
      }

      drawglyph(buf, width, height,
		x + xx + face->glyph->bitmap_left,
		y + yy + baseline - face->glyph->bitmap_top - ly,
		0, ly,
		face->glyph->bitmap.width,
		face->glyph->bitmap.rows - ly,
		&fg);

      if (pos == t->cursor) {
	drawcursor(x + xx, y + yy, ww, focus);
      }

      xx += ww;
    }
  }

 done:
  if (pos == t->cursor) {
    drawcursor(x + xx, y + yy, ww, focus);
  }

  t->width = w;
  t->height = yy + lineheight;
}

static void
updatecursor(struct textbox *t, int x, int y)
{
  struct piece *tp;
  int pos;

  tp = findpos(t->pieces, x, y,
	       t->width,
	       &pos);

  if (tp == NULL) {
    pos = 0;
    for (tp = t->pieces; tp->next != NULL; tp = tp->next) {
      pos += tp->pl;
    }
  }

  t->cursor = pos;
}

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  updatecursor(t, x, y);

  return true;
}

bool
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button)
{
  return false;
}

bool
textboxmotion(struct textbox *t, int x, int y)
{
  return false;
}

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  struct piece *o, *n;
  int i;

  o = findpiece(t->pieces, t->cursor, &i);
  if (o == NULL) {
    return false;
  }

  n = pieceinsert(o, i, s, l);
  if (n == NULL) {
    return false;
  }

  t->cursor += l;
  
  return true;
}

bool
textboxkeypress(struct textbox *t, keycode_t k)
{
  struct piece *o, *n;
  uint8_t s[16];
  size_t l;
  int i;

  o = findpiece(t->pieces, t->cursor, &i);
  if (o == NULL) {
    return false;
  }
  
  switch (k) {
  default: 
    return false;
    
  case KEY_shift:
    return false;
    
  case KEY_alt:
    return false;
    
  case KEY_super:
    return false;
    
  case KEY_control:
    return false;
    

  case KEY_left:
    return false;
    
  case KEY_right:
    return false;
    
  case KEY_up:
    return false;
    
  case KEY_down:
    return false;
    
  case KEY_pageup:
    return false;
    
  case KEY_pagedown:
    return false;
    
  case KEY_home:
    return false;
    
  case KEY_end:
    return false;
    

  case KEY_return:
    l = snprintf((char *) s, sizeof(s), "\n");

    n = pieceinsert(o, i, s, l);
    if (n == NULL) {
      return false;
    }

    t->cursor += l;
    return true;
    
  case KEY_tab:
    return false;
    
  case KEY_backspace:
    return false;
    
  case KEY_delete:
    return false;
    

  case KEY_escape:
    return false;
  }
}

bool
textboxkeyrelease(struct textbox *t, keycode_t k)
{
  return false;
}
