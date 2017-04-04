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

  t->selections = t->cselection = NULL;
  
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
  int32_t code, a, aa, pos;
  int i, xx, yy, ly, ww;
  struct piece *p;

  yy = 0;
  xx = PADDING;
  pos = 0;

  drawrect(dest, dw, dh,
	   x, y + yy,
	   x + xx, y + yy + lineheight - 1,
	   &t->bg);

  for (p = t->pieces; p != NULL && yy < h; p = p->next) {
    if (p->s == NULL) continue;

    for (i = 0; i < p->pl; pos += a, i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      if (linebreak(code, p->s + i + a, p->pl - i - a, &aa)) {
	if (pos == t->cursor) {
	  printf("should draw cursor on linebreak\n");
	}
	
	/* Line Break. */
	a += aa;

	drawrect(dest, dw, dh,
		 x + xx, y + yy,
		 x + w - 1, y + yy + lineheight - 1,
		 &t->bg);

 	xx = PADDING;
	yy += lineheight;

	drawrect(dest, dw, dh,
		 x, y + yy,
		 x + xx, y + yy + lineheight - 1,
		 &t->bg);

	continue;
      }

      if (!loadglyph(code)) {
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      if (xx + ww >= w - PADDING) {
	/* Wrap line */

	drawrect(dest, dw, dh,
		 x + xx, y + yy,
		 x + w - 1, y + yy + lineheight - 1,
		 &t->bg);

 	xx = PADDING;
	yy += lineheight;

	drawrect(dest, dw, dh,
		 x, y + yy,
		 x + xx, y + yy + lineheight - 1,
		 &t->bg);
      }

      if (yy >= h) {
	break;
      } else if (yy + lineheight >= h) {
	ly = h - yy;
      } else {
	ly = 0;
      }

      if (pos == t->cursor) {
	drawcursor(x + xx, y + yy, ww, focus);
      } else if (inselection(t, pos)) {
	drawrect(dest, dw, dh,
		 x + xx, y + yy,
		 x + xx + ww, y + yy + lineheight - 1,
		 &fg);
 
	drawglyph(buf, width, height,
		  x + xx + face->glyph->bitmap_left,
		  y + yy + baseline - face->glyph->bitmap_top - ly,
		  0, ly,
		  face->glyph->bitmap.width,
		  face->glyph->bitmap.rows - ly,
		  &t->bg);
 
      } else {
	drawrect(dest, dw, dh,
		 x + xx, y + yy,
		 x + xx + ww, y + yy + lineheight - 1,
		 &t->bg);
 
	drawglyph(buf, width, height,
		  x + xx + face->glyph->bitmap_left,
		  y + yy + baseline - face->glyph->bitmap_top - ly,
		  0, ly,
		  face->glyph->bitmap.width,
		  face->glyph->bitmap.rows - ly,
		  &fg);
      }
      
      xx += ww;
    }
  }

  if (pos == t->cursor) {
    printf("should draw cursor at eof\n");
  }
  
  if (yy + lineheight - 1 < h) {
    drawrect(dest, dw, dh,
	     x + xx, y + yy,
	     x + w - 1, y + yy + lineheight - 1,
	     &t->bg);
  }

  t->width = w;
  t->height = yy + lineheight;
}

/* Returns the piece that was clicked on and sets *pos to the position
   in the list of pieces.
*/

static struct piece *
findpos(struct textbox *t,
	int x, int y,
	int *pos)
{
  int32_t code, a, aa;
  int i, xx, yy, ww;
  struct piece *p;

  xx = PADDING;
  yy = 0;

  *pos = 0;

  for (p = t->pieces; p != NULL; p = p->next) {
    if (p->s == NULL) {
      continue;
    }
    
    for (i = 0; i < p->pl; i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      if (linebreak(code, p->s + i + a, p->pl - i - a, &aa)) {
	/* Line Break. */
	a += aa;

	if (yy < y && y <= yy + lineheight) {
	  *pos += i;
	  return p;
	} else {
	  yy += lineheight;
	  xx = PADDING;
	}
      }
     
      if (!loadglyph(code)) {
	continue;
      }

      ww = face->glyph->advance.x >> 6;

      if (xx + ww >= t->width - PADDING) {
	/* Wrap Line. */
	if (yy < y && y <= yy + lineheight) {
	  *pos += i;
	  return p;
	} else {
	  yy += lineheight;
	  xx = PADDING;
	}
      }

      xx += ww;
      
      if (yy <= y && y < yy + lineheight && x <= xx) {
	/* Found character. */
	*pos += i;
	return p;
      }
    }
  
    *pos += i;
    if ((p->next == NULL || p->next->s == NULL)
	&& yy < y && y <= yy + lineheight) {
      return p;
    }
  }

  return NULL;
}

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  struct selection *s, *sn;
  struct piece *tp;
  int pos;

  switch (button) {
  case 1:
    tp = findpos(t, x, y, &pos);
    if (tp == NULL) {
      pos = 0;
      for (tp = t->pieces; tp->next != NULL; tp = tp->next) {
	pos += tp->pl;
      }
    }

    t->cursor = pos;

    s = t->selections;
    while (s != NULL) {
      sn = s->next;
      selectionfree(s);
      s = sn;
    }

    s = selectionnew(pos, pos);
    if (s == NULL) {
      return true;
    }

    t->cselection = t->selections = s;

    return true;

  default:
    return false;
  }
}

bool
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button)
{
  switch (button) {
  case 1:
    t->cselection = NULL;
    return true;

  default:
    return false;
  }
}

bool
textboxmotion(struct textbox *t, int x, int y)
{
  struct piece *tp;
  int pos;

  if (t->cselection == NULL) {
    return false;
  }

  tp = findpos(t, x, y, &pos);
  if (tp == NULL) {
    pos = 0;
    for (tp = t->pieces; tp->next != NULL; tp = tp->next) {
      pos += tp->pl;
    }
  }

  selectionupdate(t->cselection, pos);
  t->cursor = pos;
 
  return true;
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
    printf("should backspace\n");
    return false;
    
  case KEY_delete:
    printf("should delete\n");
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
