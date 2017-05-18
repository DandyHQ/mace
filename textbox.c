#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mace.h"

struct textbox *
textboxnew(struct tab *tab, struct colour *bg,
	   struct sequence *seq)
{
  struct textbox *t;

  t = calloc(1, sizeof(struct textbox));
  if (t == NULL) {
    return NULL;
  }

  t->sequence = seq;
  
  t->cursor = 0;
  t->csel = NULL;
  
  t->tab = tab;

  t->cr = NULL;
  t->sfc = NULL;

  t->yoff = 0;
  t->linewidth = 0;
  t->height = 0;

  t->maxheight = 0;
  t->startpos = 0;
  t->starty = 0;
  
  memmove(&t->bg, bg, sizeof(struct colour));

  return t;
}

void
textboxfree(struct textbox *t)
{
  luaremove(t);
  
  sequencefree(t->sequence);

  if (t->cr != NULL) {
    cairo_destroy(t->cr);
  }

  if (t->sfc != NULL) {
    cairo_surface_destroy(t->sfc);
  }

  free(t);
}

bool
textboxresize(struct textbox *t, int lw, int maxheight)
{
  cairo_surface_t *sfc;
  cairo_t *cr;

  sfc = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				   lw, maxheight);

  if (sfc == NULL) {
    return false;
  }

  cr = cairo_create(sfc);
  if (cr == NULL) {
    cairo_surface_destroy(sfc);
    return false;
  }
  
  if (t->cr != NULL) {
    cairo_destroy(t->cr);
  }

  if (t->sfc != NULL) {
    cairo_surface_destroy(t->sfc);
  }

  t->cr = cr;
  t->sfc = sfc;

  t->linewidth = lw;
  t->maxheight = maxheight;

  textboxpredraw(t, true, true);

  return true;
}

static size_t
findpos(struct textbox *t,
	int x, int y)
{
  struct sequence *s;
  size_t pos, p, i;
  int32_t code, a;
  int xx, yy, ww;

  s = t->sequence;

  xx = 0;

  p = sequencefindpiece(s, t->startpos, &i);
  yy = t->starty;

  pos = s->pieces[p].pos;

  while (p != SEQ_end) {
    while (i < s->pieces[p].len) {
      a = utf8proc_iterate(s->data + s->pieces[p].off + i,
			   s->pieces[p].len - i, &code);
      if (a <= 0) {
	i++;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code,
		      s->data + s->pieces[p].off + i,
		      s->pieces[p].len - i, &a)) {

	if (y < yy + mace->lineheight - 1) {
	  return pos + i;
	} else {
	  xx = 0;
	  yy += mace->lineheight;
	}
      }
     
      if (!loadglyph(code)) {
	i += a;
	continue;
      }

      ww = mace->fontface->glyph->advance.x >> 6;

      /* Wrap Line. */
      if (xx + ww >= t->linewidth) {
	if (y < yy + mace->lineheight - 1) {
	  return pos + i;
	} else {
	  xx = 0;
	  yy += mace->lineheight;
	}
      }

      xx += ww;
      
      if (y < yy + mace->lineheight - 1 && x < xx) {
	/* Found position. */
	return pos + i;
      }

      i += a;
    }

    pos += i;
    i = 0;
    p = s->pieces[p].next;
  }

  return pos + i;
}

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  size_t pos, start, len;
  struct selection *sel;
  uint8_t *buf;
  
  pos = findpos(t, x, y);
  
  switch (button) {
  case 1:
    t->cursor = pos;

    t->csel = selectionreplace(t, pos);
    t->cselisvalid = false;
    
    textboxpredraw(t, false, false);
    return true;

  case 3:
    sel = inselections(t, pos);
    if (sel != NULL) {
      start = sel->start;
      len = sel->end - sel->start + 1;
    } else if (!sequencefindword(t->sequence, pos, &start, &len)) {
      return false;
    }

    buf = malloc(len + 1);
    if (buf == NULL) {
      return false;
    }

    if (sequenceget(t->sequence, start, buf, len) == 0) {
      return false;
    }

    command(buf);

    free(buf);

    return true;
  }

  return false;
}

bool
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button)
{
  if (!t->cselisvalid && t->csel != NULL) {
    selectionremove(t->csel);
    t->csel = NULL;
    textboxpredraw(t, false, false);
    return true;
  } else {
    t->csel = NULL;
    return false;
  }
}

bool
textboxmotion(struct textbox *t, int x, int y)
{
  size_t pos;
  
  if (t->csel == NULL) {
    return false;
  }
   
  pos = findpos(t, x, y);

  if (selectionupdate(t->csel, pos)) {
    t->cselisvalid = true;
    textboxpredraw(t, false, false);
    return true;
  } else {
    return false;
  }
}

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
    return false;
  }

  t->cursor += l;
  textboxpredraw(t, false, true);
  return true;
}

bool
textboxkeypress(struct textbox *t, keycode_t k)
{
  struct selection *sel, *nsel;
  uint8_t s[16];
  size_t l;
   
  switch (k) {
  default:
    break;
    
  case KEY_shift:
    break;
    
  case KEY_alt:
    break;
    
  case KEY_super:
    break;
    
  case KEY_control:
    break;
    

  case KEY_left:
    break;
    
  case KEY_right:
    break;
    
  case KEY_up:
    break;
    
  case KEY_down:
    break;
    
  case KEY_pageup:
    break;
    
  case KEY_pagedown:
    break;
    
  case KEY_home:
    break;
    
  case KEY_end:
    break;
    

  case KEY_return:
    l = snprintf((char *) s, sizeof(s), "\n");

    if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
      return false;
    }

    t->cursor += l;
 
    textboxpredraw(t, false, true);

    return true;
    
  case KEY_tab:
    break;


    /* TODO: improve significantly */

  case KEY_delete:
    if (mace->selections == NULL) {
      if (sequencedelete(t->sequence, t->cursor, 1)) {
	textboxpredraw(t, true, true);
	return true;
      } else {
	return false;
      }
    } 

    /* Fall through to delete selections */
    
  case KEY_backspace:
    if (mace->selections == NULL && t->cursor > 0) {
      if (sequencedelete(t->sequence, t->cursor - 1, 1)) {
	t->cursor--;
	textboxpredraw(t, true, true);
	return true;
      } else {
	return false;
      }
    } 

    /* Delete selections */

    for (sel = mace->selections; sel != NULL; sel = nsel) {
      nsel = sel->next;

      if (sel->textbox != t) {
	continue;
      }

      if (sel->start <= t->cursor && t->cursor <= sel->end) {
	t->cursor = sel->start;
      }
      
      sequencedelete(t->sequence,
		     sel->start,
		     sel->end - sel->start + 1);

      selectionremove(sel);
    }

    textboxpredraw(t, true, true);
 
    return true;

  case KEY_escape:
    break;
  }

  return false;
}

bool
textboxkeyrelease(struct textbox *t, keycode_t k)
{
  return false;
}

bool
textboxscroll(struct textbox *t, int x, int y, int dy)
{
  t->yoff += dy;
  if (t->yoff < 0) {
    t->yoff = 0;
  } else if (t->yoff > t->height - mace->lineheight) {
    t->yoff = t->height - mace->lineheight;
  }

  textboxpredraw(t, true, false);

  return true;
}
