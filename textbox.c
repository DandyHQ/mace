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

  t->font = tab->mace->font;
  
  t->sequence = seq;
  t->tab = tab;
  
  t->cursor = 0;

  t->newselpos = SIZE_MAX;
  t->csel = NULL;
  t->selections = NULL;

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
  while (t->selections != NULL) {
    selectionremove(t, t->selections);
  }
  
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

  textboxcalcpositions(t, 0);
  textboxpredraw(t);

  return true;
}

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  struct selection *sel;
  size_t pos, start, len;

  pos = textboxfindpos(t, x, y);
  
  switch (button) {
  case 1:
    while (t->tab->mace->keyfocus->selections != NULL) {
      selectionremove(t->tab->mace->keyfocus,
		      t->tab->mace->keyfocus->selections);
    }

    textboxpredraw(t->tab->mace->keyfocus);
    
    t->tab->mace->keyfocus = t;

    t->cursor = pos;
    t->newselpos = pos;

    textboxpredraw(t);

    return true;

  case 3:
    sel = inselections(t, pos);
    if (sel != NULL) {
      sel->type = SELECTION_command;

      textboxpredraw(t);
      return true;

    } else if (sequencefindword(t->sequence, pos, &start, &len)) {
      sel = selectionadd(t, SELECTION_command, start, start + len - 1);

      textboxpredraw(t);
      return true;

    } else {
      return false;
    }
  }

  return false;
}

bool
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button)
{
  struct selection *sel, *next;
  size_t pos, start, len;
  uint8_t *buf;

  pos = textboxfindpos(t, x, y);

  t->csel = NULL;
  t->newselpos = SIZE_MAX;

  switch (button) {
  case 1:
  case 2:
    break;

  case 3:
    sel = inselections(t, pos);
    if (sel != NULL && sel->type == SELECTION_command) {
      start = sel->start;
      len = sel->end - start + 1;
    } else {
      len = 0;
    }

    sel = t->selections;
    while (sel != NULL) {
      next = sel->next;

      if (sel->type == SELECTION_command) {
	selectionremove(t, sel);
      }

      sel = next;
    }

    textboxpredraw(t);

    if (len != 0) {
      buf = malloc(len);
      if (buf == NULL) {
	return false;
      }

      if (sequenceget(t->sequence, start, buf, len) == 0) {
	return false;
      }

      /* TODO: Show an error somehow if this returns false. */
      command(t->tab->mace, buf);

      free(buf);
    }


    return true;
  }

  return false;
}

bool
textboxmotion(struct textbox *t, int x, int y)
{
  size_t pos;

  if (t->newselpos != SIZE_MAX) {
    pos = textboxfindpos(t, x, y);
    if (pos == t->newselpos) {
      return false;
    }

    t->csel = selectionadd(t, SELECTION_normal, t->newselpos, pos);
    if (t->csel == NULL) {
      return false;
    }

    t->newselpos = SIZE_MAX;
        
    textboxpredraw(t);
    return true;

  } else if (t->csel != NULL) {
    pos = textboxfindpos(t, x, y);

    if (selectionupdate(t->csel, pos)) {
      /* TODO: set cursor to pos + next code point length */
      t->cursor = pos;
      textboxpredraw(t);
      return true;
    }
  }
  
  return false;
}

static bool
deleteselections(struct textbox *t)
{
  struct selection *sel;
  size_t start;
  
  start = t->sequence->pieces[SEQ_end].pos;
    
  while (t->selections != NULL) {
    sel = t->selections;
    
    if (sel->start < start) {
      start = sel->start;
    }

    if (!sequencedelete(t->sequence,
			sel->start,
			sel->end - sel->start + 1)) {

      return false;
    }

    if (sel->start <= t->cursor && t->cursor <= sel->end) {
      t->cursor = sel->start;
    }

    selectionremove(t, sel);
  }

  textboxcalcpositions(t, start);
  textboxpredraw(t);

  return true;
}

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  bool redraw = false;
  
  if (t->selections != NULL) {
    redraw = deleteselections(t);
  }

  if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
    return redraw;
  }

  textboxcalcpositions(t, t->cursor);

  t->cursor += l;

  textboxpredraw(t);

  return true;
}

bool
textboxkeypress(struct textbox *t, keycode_t k)
{
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
    return textboxscroll(t, 0, t->yoff
			 - (t->maxheight - t->font->lineheight * 2));
    
  case KEY_pagedown:
    return textboxscroll(t, 0, t->yoff
			 + (t->maxheight - t->font->lineheight * 2));
    
  case KEY_home:
    break;
    
  case KEY_end:
    break;
    
  case KEY_tab:
    if (t->selections != NULL) {
      deleteselections(t);
    }

    l = snprintf((char *) s, sizeof(s), "\t");
    
    if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
      return true;
    }

    textboxcalcpositions(t, t->cursor);
    t->cursor += l;
    textboxpredraw(t);

    return true;

  case KEY_return:
    if (t->selections != NULL) {
      deleteselections(t);
    }

    l = snprintf((char *) s, sizeof(s), "\n");

    if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
      return true;
    }

    textboxcalcpositions(t, t->cursor);
    t->cursor += l;
    textboxpredraw(t);

    return true;
  
  case KEY_delete:
    if (t->selections != NULL) {
      return deleteselections(t);

    } else if (sequencedelete(t->sequence, t->cursor, 1)) {
      textboxcalcpositions(t, t->cursor);
      textboxpredraw(t);
      return true;
    } 

    break;
    
  case KEY_backspace:
    if (t->selections != NULL) {
      return deleteselections(t);

    } else if (t->cursor > 0 && sequencedelete(t->sequence,
					       t->cursor - 1, 1)) {

      t->cursor--;
      textboxcalcpositions(t, t->cursor);
      textboxpredraw(t);
      return true;
    } 

    break;
    
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
textboxscroll(struct textbox *t, int xoff, int yoff)
{
  if (yoff < 0) {
    yoff = 0;
  } else if (yoff > t->height - t->font->lineheight) {
    yoff = t->height - t->font->lineheight;
  }

  if (yoff != t->yoff) {
    t->yoff = yoff;

    textboxpredraw(t);

    return true;
  } else {
    return false;
  }
}
