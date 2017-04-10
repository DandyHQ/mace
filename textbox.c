#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

bool
textboxinit(struct textbox *t, struct tab *tab,
	    bool scrollable,
	    struct colour *bg)
{
  struct piece *b, *e;
 
  t->width = 0;
  t->height = 0;
  t->rheight = 0;

  t->buf = malloc(sizeof(uint8_t) * 4);
  if (t->buf == NULL) {
    return false;
  }

  b = piecenewtag();
  if (b == NULL) {
    free(t->buf);
    return false;
  }

  e = piecenewtag();
  if (e == NULL) {
    piecefree(b);
    free(t->buf);
    return false;
  }

  b->prev = NULL;
  b->next = e;
  e->prev = b;
  e->next = NULL;

  t->cpiece = NULL;
  t->pieces = b;
  t->cursor = 0;

  t->scrollable = scrollable;
  t->scroll = 0;
  
  t->tab = tab;

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

  free(t->buf);
}

void
textboxresize(struct textbox *t, int w)
{
  t->width = w;
  t->height = 0;
  t->rheight = lineheight;

  t->buf = realloc(t->buf, t->width * t->rheight * sizeof(uint8_t) * 4);
  if (t->buf == NULL) {
    err(1, "Failed to allocate new buffer for textbox!\n");
  }

  textboxpredraw(t);
}

/* Returns the piece that was clicked on and sets *pos to the position
   in the list of pieces.
*/

static struct piece *
findpos(struct textbox *t,
	int x, int y,
	int *pos)
{
  int i, xx, yy, ww;
  int32_t code, a;
  struct piece *p;

  xx = 0;
  yy = 0;

  *pos = 0;

  for (p = t->pieces; p != NULL; p = p->next) {
    for (i = 0; i < p->pl; *pos += a, i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code, p->s + i, p->pl - i, &a)) {
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
      if (xx + ww >= t->width) {
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

bool
textboxscroll(struct textbox *t, int dx, int dy)
{
  if (!t->scrollable) return false;
  
  t->scroll += dy;
  if (t->scroll < 0) {
    t->scroll = 0;
  } else if (t->scroll > t->height - lineheight) {
    t->scroll = t->height - lineheight;
  }

  return true;
}

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  struct selection *s, *sn;
  int pos, start, end;
  struct piece *tp;
  uint8_t *cmd;
  bool r;

  t->cpiece = NULL;
  tp = findpos(t, x, y, &pos);
  if (tp == NULL) {
    return false;
  }

  switch (button) {
  case 1:
    t->cursor = pos;

    s = t->selections;
    while (s != NULL) {
      sn = s->next;
      selectionfree(s);
      s = sn;
    }

    s = selectionnew(&t->bg, &fg, pos, pos);
    /* Doesn't matter if s == NULL */
    
    t->cselection = t->selections = s;

    textboxpredraw(t);
    return true;

  case 3:
    s = inselections(t->selections, pos);
    if (s == NULL) {
      if (!piecefindword(t->pieces, pos, &start, &end)) {
	return false;
      }

      s = selectionnew(&t->bg, &fg, start, end);
      if (s == NULL) {
	return false;
      }
    }

    cmd = selectiontostring(s, t->pieces);
    if (cmd != NULL) {
      r = docommand(cmd);
      free(cmd);
      textboxpredraw(t);
      return r;
    } else {
      return false;
    }
    
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
    if (t->cselection != NULL) {
      if (t->cselection->start == t->cselection->end) {
	/* TODO: How to differentiate clicks from selections? */
	printf("only selected one glyph, this is probably meant to be a cursor reloaction\n");
      }

      t->cselection = NULL;
    }

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
    return false;
  }
  
  if (selectionupdate(t->cselection, pos)) {
    t->cursor = pos;
    textboxpredraw(t);
    return true;
  } else {
    return false;
  }
}

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  struct piece *o, *n;
  int i;

  if (t->cpiece != NULL && pieceappend(t->cpiece, s, l)) {
    t->cursor += l;
    textboxpredraw(t);
    return true;
  } 
  
  o = piecefind(t->pieces, t->cursor, &i);
  if (o == NULL) {
    return false;
  }

  n = pieceinsert(o, i, s, l);
  if (n == NULL) {
    return false;
  }

  t->cursor += l;
  t->cpiece = n;

  textboxpredraw(t);
  
  return true;
}

bool
textboxkeypress(struct textbox *t, keycode_t k)
{
  struct piece *o, *n;
  uint8_t s[16];
  size_t l;
  int i;
   
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

    if (t->cpiece != NULL && pieceappend(t->cpiece, s, l)) {
      t->cursor += l;
      textboxpredraw(t);
      return true;
    } else {
      o = piecefind(t->pieces, t->cursor, &i);
      if (o == NULL) {
	return false;
      }
    
      n = pieceinsert(o, i, s, l);
      if (n == NULL) {
	return false;
      }

      t->cursor += l;
      t->cpiece = n;

      textboxpredraw(t);
      return true;
    }
    
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
