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

struct textbox *
textboxnew(struct tab *tab,
	   struct colour *bg)
{
  struct textbox *t;

  t = malloc(sizeof(struct textbox));
  if (t == NULL) {
    return NULL;
  }

  t->text = sequencenew();
  if (t->text == NULL) {
    free(t);
    return NULL;
  }
  
  t->cursor = 0;
  t->csel = NULL;
  
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
  sequencefree(t->text);

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

  for (p = SEQ_start; p != SEQ_end; p = t->text->pieces[p].next) {
    for (i = 0; i < t->text->pieces[p].len; pos += a, i += a) {
      a = utf8proc_iterate(t->text->data + t->text->pieces[p].off + i,
			   t->text->pieces[p].len - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code,
		      t->text->data + t->text->pieces[p].off + i,
		      t->text->pieces[p].len - i, &a)) {

	if (y < yy + lineheight - 1) {
	  return pos;
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
	  return pos;
	} else {
	  xx = 0;
	  yy += lineheight;
	}
      }

      xx += ww;
      
      if (y < yy + lineheight - 1 && x < xx) {
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
    t->cselvalid = false;
    
    textboxpredraw(t);
    tabdraw(t->tab);
    
    break;

  case 3:
    sel = inselections(t, pos);
    if (sel != NULL) {
      start = sel->start;
      len = sel->end - sel->start + 1;
    } else if (!sequencefindword(t->text, pos, &start, &len)) {
      return;
    }

    buf = malloc(len + 1);
    if (buf == NULL) {
      return;
    }

    if (!sequenceget(t->text, start, buf, len)) {
      return;
    }

    buf[len] = 0;
    
    command(t->tab->main, buf);

    free(buf);

    textboxpredraw(t);
    tabdraw(t->tab);

    break;
  }
}

void
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button)
{
  if (!t->cselvalid && t->csel != NULL) {
    selectionremove(t->csel);

    textboxpredraw(t);
    tabdraw(t->tab);
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
    t->cselvalid = true;
    textboxpredraw(t);
    tabdraw(t->tab);
  }
}

void
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  if (!sequenceinsert(t->text, t->cursor, s, l)) {
    return;
  }

  t->cursor += l;
  textboxpredraw(t);
  tabdraw(t->tab);
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

    if (!sequenceinsert(t->text, t->cursor, s, l)) {
      return;
    }

    t->cursor += l;
 
    textboxpredraw(t);
    tabdraw(t->tab);

    break;
    
  case KEY_tab:
    break;


    /* TODO: improve significantly */

  case KEY_delete:
    if (selections == NULL) {
      if (sequencedelete(t->text, t->cursor, 1)) {
	textboxpredraw(t);
	tabdraw(t->tab);
      }

      break;
    } 

    /* Fall through to delete selections */
    
  case KEY_backspace:
    if (selections == NULL && t->cursor > 0) {
      if (sequencedelete(t->text, t->cursor - 1, 1)) {
	t->cursor--;
	textboxpredraw(t);
	tabdraw(t->tab);
      }

      break;
    } 

    /* Delete selections */

    for (sel = selections; sel != NULL; sel = nsel) {
      nsel = sel->next;

      if (sel->textbox != t) {
	continue;
      }

      if (sel->start <= t->cursor && t->cursor <= sel->end) {
	t->cursor = sel->start;
      }
      
      sequencedelete(t->text, sel->start, sel->end - sel->start + 1);
      selectionremove(sel);
    }

    textboxpredraw(t);
    tabdraw(t->tab);
 
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
  } else if (t->yoff > t->height - lineheight) {
    t->yoff = t->height - lineheight;
  }

  tabdraw(t->tab);
}
