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

  textboxpredraw(t);

  return true;
}

static size_t
findpos(struct textbox *t,
	int x, int y)
{
  int32_t code, a, i;
  int xx, yy, ww;
  size_t pos;
  size_t p;

  xx = 0;
  yy = 0;

  pos = 0;

  for (p = SEQ_start; p != SEQ_end; p = t->sequence->pieces[p].next) {
    for (i = 0; i < t->sequence->pieces[p].len; pos += a, i += a) {
      a = utf8proc_iterate(t->sequence->data + t->sequence->pieces[p].off + i,
			   t->sequence->pieces[p].len - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code,
		      t->sequence->data + t->sequence->pieces[p].off + i,
		      t->sequence->pieces[p].len - i, &a)) {

	if (y < yy + mace->lineheight - 1) {
	  return pos;
	} else {
	  xx = 0;
	  yy += mace->lineheight;
	}
      }
     
      if (!loadglyph(code)) {
	continue;
      }

      ww = mace->fontface->glyph->advance.x >> 6;

      /* Wrap Line. */
      if (xx + ww >= t->linewidth) {
	if (y < yy + mace->lineheight - 1) {
	  return pos;
	} else {
	  xx = 0;
	  yy += mace->lineheight;
	}
      }

      xx += ww;
      
      if (y < yy + mace->lineheight - 1 && x < xx) {
	/* Found position. */
	return pos;
      }
    }
  }

  return pos;
}

void
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  size_t pos, start, len;
  struct selection *sel;
  uint8_t *buf;
  
  pos = findpos(t, x, y + t->yoff);
  
  switch (button) {
  case 1:
    t->cursor = pos;

    t->csel = selectionreplace(t, pos);
    t->cselisvalid = false;
    
    textboxpredraw(t);
    break;

  case 3:
    sel = inselections(t, pos);
    if (sel != NULL) {
      start = sel->start;
      len = sel->end - sel->start + 1;
    } else if (!sequencefindword(t->sequence, pos, &start, &len)) {
      return;
    }

    buf = malloc(len + 1);
    if (buf == NULL) {
      return;
    }

    if (sequenceget(t->sequence, start, buf, len) == 0) {
      return;
    }

    command(buf);

    free(buf);

    textboxpredraw(t);

    break;
  }
}

void
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button)
{
  if (!t->cselisvalid && t->csel != NULL) {
    selectionremove(t->csel);
    textboxpredraw(t);
  }
  
  t->csel = NULL;
}

void
textboxmotion(struct textbox *t, int x, int y)
{
  size_t pos;
  
  if (t->csel == NULL) {
    return;
  }
   
  pos = findpos(t, x, y + t->yoff);

  if (selectionupdate(t->csel, pos)) {
    t->cselisvalid = true;
    textboxpredraw(t);
  }
}

void
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
    return;
  }

  t->cursor += l;
  textboxpredraw(t);
}

void
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
      return;
    }

    t->cursor += l;
 
    textboxpredraw(t);

    break;
    
  case KEY_tab:
    break;


    /* TODO: improve significantly */

  case KEY_delete:
    if (mace->selections == NULL) {
      if (sequencedelete(t->sequence, t->cursor, 1)) {
	textboxpredraw(t);
      }

      break;
    } 

    /* Fall through to delete selections */
    
  case KEY_backspace:
    if (mace->selections == NULL && t->cursor > 0) {
      if (sequencedelete(t->sequence, t->cursor - 1, 1)) {
	t->cursor--;
	textboxpredraw(t);
      }

      break;
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

    textboxpredraw(t);
 
    break;

  case KEY_escape:
    break;
  }
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
  } else if (t->yoff > t->height - mace->lineheight) {
    t->yoff = t->height - mace->lineheight;
  }

  textboxpredraw(t);
}
