#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

bool
textboxpredraw(struct textbox *t);

struct textbox *
textboxnew(struct tab *tab,
	   struct colour *bg)
{
  struct piece *b, *e;
  struct textbox *t;

  t = malloc(sizeof(struct textbox));
  if (t == NULL) {
    return NULL;
  }

  b = piecenewtag();
  if (b == NULL) {
    free(t);
    return NULL;
  }

  e = piecenewtag();
  if (e == NULL) {
    piecefree(b);
    free(t);
    return NULL;
  }

  b->prev = NULL;
  b->next = e;
  e->prev = b;
  e->next = NULL;

  t->cpiece = NULL;
  t->pieces = b;
  t->cursor = 0;

  t->tab = tab;

  t->cr = NULL;
  t->sfc = NULL;

  t->yoff = 0;
  t->linewidth = 0;
  t->height = 0;
  
  memmove(&t->bg, bg, sizeof(struct colour));

  return t;
}

void
textboxfree(struct textbox *t)
{
  struct piece *p;

  /* TODO: Free removed pieces. */
  for (p = t->pieces; p != NULL; p = p->next)
    piecefree(p);

  if (t->cr != NULL) {
    cairo_destroy(t->cr);
  }

  if (t->sfc != NULL) {
    cairo_surface_destroy(t->sfc);
  }
}

bool
textboxresize(struct textbox *t, int lw)
{
  t->linewidth = lw;
  
  if (t->cr != NULL) {
    cairo_destroy(t->cr);
    t->cr = NULL;
  }

  if (t->sfc != NULL) {
    cairo_surface_destroy(t->sfc);
    t->sfc = NULL;
  }

  t->sfc = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				       lw, lineheight * 30);
  if (t->sfc == NULL) {
    return false;
  }

  t->cr = cairo_create(t->sfc);
  if (t->cr == NULL) {
    return false;
  }

  return textboxpredraw(t);
}

/* Returns the piece that was clicked on and sets *pos to the position
   in the list of pieces.
*/

static struct piece *
findpos(struct textbox *t,
	int x, int y,
	int32_t *pos, int32_t *i)
{
  int32_t code, a;
  struct piece *p;
  int xx, yy, ww;

  xx = 0;
  yy = 0;

  *pos = 0;

  for (p = t->pieces; p != NULL; p = p->next) {
    for (*i = 0; *i < p->pl; *pos += a, *i += a) {
      a = utf8proc_iterate(p->s + *i, p->pl - *i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code, p->s + *i, p->pl - *i, &a)) {
	if (y < yy + lineheight - 1) {
	  return p;
	} else {
	  xx = 0;
	  yy += lineheight;
	}
      }
     
      if (!loadglyph(code)) {
	continue;
      }

      ww = face->glyph->advance.x >> 6;

      /* Wrap Line. */
      if (xx + ww >= t->linewidth) {
	if (y < yy + lineheight - 1) {
	  return p;
	} else {
	  xx = 0;
	  yy += lineheight;
	}
      }

      xx += ww;
      
      if (y < yy + lineheight - 1 && x < xx) {
	/* Found position. */
	return p;
      }
    }
  
    if (p->next == NULL || p->next->s == NULL) {
      return p;
    }
  }

  return NULL;
}

static uint8_t *
rangetostring(struct textbox *t,
	      int32_t start, int32_t end,
	      size_t *len)
{
  struct piece *p;
  int pos, b, l;
  uint8_t *buf;

  *len = end - start + 1;
  buf = malloc(*len);
  if (buf == NULL) {
    return NULL;
  }
  
  pos = 0;
  for (p = t->pieces; p != NULL; p = p->next) {
    if (pos > end) {
      break;
    } else if (pos + p->pl < start) {
      pos += p->pl;
      continue;
    }

    if (pos < start) {
      b = start - pos;
    } else {
      b = 0;
    }

    if (pos + p->pl > end + 1) {
      l = end + 1 - pos - b;
    } else {
      l = p->pl - b;
    }

    memmove(buf + (pos + b - start), p->s + b, l);
    pos += b + l;
  }

  buf[pos - start] = 0;
  return buf;
}

void
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  int32_t pos, i, start, end;
  struct selection *sel, *seln;
  struct piece *p;
  uint8_t *s;
  size_t len;
  
  p = findpos(t, x, y + t->yoff, &pos, &i);
  if (p == NULL) {
    return;
  }
  
  switch (button) {
  case 1:
    t->cpiece = NULL;
    t->cursor = pos;

    /* In future there will be a way to have multiple selections */
    
    sel = selections;
    while (sel != NULL) {
      seln = sel->next;
      selectionfree(sel);
      sel = seln;
    }

    selections = NULL;
    
    t->csel = selectionnew(t, pos);
    if (t->csel != NULL) {
      t->csel->next = selections;
      selections = t->csel;
    }
    
    textboxpredraw(t);
    tabdraw(t->tab);
    
    break;

  case 3:
    sel = inselections(t, pos);
    if (sel != NULL) {
      start = sel->start;
      end = sel->end;
    } else if (!piecefindword(t->pieces, pos, &start, &end)) {
      return;
    }

    s = rangetostring(t, start, end, &len);
    if (s == NULL) {
      return;
    }
    
    eval(t->tab->main, s, len);

    free(s);

    textboxpredraw(t);
    tabdraw(t->tab);

    break;
  }
}

void
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button)
{
  t->csel = NULL;
}

void
textboxmotion(struct textbox *t, int x, int y)
{
  struct piece *p;
  int pos, i;
  
  if (t->csel == NULL) {
    return;
  }
   
  p = findpos(t, x, y + t->yoff, &pos, &i);
  if (p == NULL) {
    return;
  }

  if (selectionupdate(t->csel, pos)) {
    textboxpredraw(t);
    tabdraw(t->tab);
  }
}

void
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  struct piece *o, *n;
  int i;

  if (t->cpiece != NULL && pieceappend(t->cpiece, s, l)) {
    t->cursor += l;
  } else {
    o = piecefind(t->pieces, t->cursor, &i);
    if (o == NULL) {
      return;
    }

    n = pieceinsert(o, i, s, l);
    if (n == NULL) {
      return;
    }

    t->cursor += l;
    t->cpiece = n;
  }

  textboxpredraw(t);
  tabdraw(t->tab);
}

void
textboxkeypress(struct textbox *t, keycode_t k)
{
  struct piece *o, *n;
  uint8_t s[16];
  size_t l;
  int i;
   
  switch (k) {
  default: 
    return;
    
  case KEY_shift:
    return;
    
  case KEY_alt:
    return;
    
  case KEY_super:
    return;
    
  case KEY_control:
    return;
    

  case KEY_left:
    return;
    
  case KEY_right:
    return;
    
  case KEY_up:
    return;
    
  case KEY_down:
    return;
    
  case KEY_pageup:
    return;
    
  case KEY_pagedown:
    return;
    
  case KEY_home:
    return;
    
  case KEY_end:
    return;
    

  case KEY_return:
    l = snprintf((char *) s, sizeof(s), "\n");

    if (t->cpiece != NULL) {
      o = t->cpiece;
      i = o->pl;
    } else {
      o = piecefind(t->pieces, t->cursor, &i);
      if (o == NULL) {
	return;
      }
    }
    
    n = pieceinsert(o, i, s, l);
    if (n == NULL) {
      return;
    }

    t->cursor += l;
    t->cpiece = n;

    break;
    
  case KEY_tab:
    return;
    
  case KEY_backspace:
    return;
    
  case KEY_delete:
    return;
    

  case KEY_escape:
    return;
  }

  textboxpredraw(t);
  tabdraw(t->tab);
}

void
textboxkeyrelease(struct textbox *t, keycode_t k)
{

}

void
textboxscroll(struct textbox *t, int x, int y, int dy)
{
  t->yoff += dy;
  if (t->yoff < 0) {
    t->yoff = 0;
  } else if (t->yoff > t->height - lineheight) {
    t->yoff = t->height - lineheight;
  }

  tabdraw(t->tab);
}
