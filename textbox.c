
#include "mace.h"

struct textbox *
textboxnew(struct tab *tab, struct colour *bg,
	   struct sequence *seq)
{
  struct textbox *t;
  struct piece *p;
  
  t = calloc(1, sizeof(struct textbox));
  if (t == NULL) {
    return NULL;
  }

  t->font = tab->mace->font;
  t->sequence = seq;
  t->tab = tab;
  
  t->cursor = 0;

  t->csel = NULL;
  t->selections = NULL;

  t->cr = NULL;
  t->sfc = NULL;

  t->yoff = 0;
  t->linewidth = 0;
  t->height = 0;

  t->maxheight = 0;

  t->startpiece = SEQ_start;

  p = &t->sequence->pieces[t->startpiece];
  p->x = 0;
  p->y = 0;
  
  t->startindex = 0;
  t->startx = 0;
  t->starty = 0;
  
  memmove(&t->bg, bg, sizeof(struct colour));

  return t;
}

void
textboxfree(struct textbox *t)
{
  struct selection *sel, *nsel;

  luaremove(t->tab->mace->lua, t);

  sel = t->selections;
  while (sel != NULL) {
    nsel = sel->next;
    selectionfree(sel);
    sel = nsel;
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

  t->startpiece = SEQ_start;
  t->startindex = 0;
  t->startx = 0;
  t->starty = 0;
  
  textboxcalcpositions(t, 0);
  textboxfindstart(t);
  textboxpredraw(t);

  return true;
}

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button)
{
  struct selection *sel, *nsel;
  size_t pos, start, len;
  uint8_t *buf;

  pos = textboxfindpos(t, x, y);
  
  switch (button) {
  case 1:
    t->cursor = pos;

    sel = t->selections;
    t->selections = NULL;

    while (sel != NULL) {
      nsel = sel->next;
      selectionfree(sel);
      sel = nsel;
    }
    
    t->csel = t->selections = selectionnew(t, pos);
    t->cselisvalid = false;

    textboxpredraw(t);
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

    command(t->tab->mace->lua, buf);

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

    t->selections = t->selections->next;
    selectionfree(t->csel);
    t->csel = NULL;

    textboxpredraw(t);

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

  pos = textboxfindpos(t, x, y);

  if (selectionupdate(t->csel, pos)) {
    t->cselisvalid = true;
    textboxpredraw(t);
    return true;
  } else {
    return false;
  }
}

static bool
removeselections(struct textbox *t)
{
  struct selection *sel, *nsel;
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

    nsel = sel->next;
    t->selections = nsel;
    selectionfree(sel);
    sel = nsel;
  }

  textboxcalcpositions(t, start);

  t->startpiece = SEQ_start;  
  textboxfindstart(t);
  textboxpredraw(t);

  return true;
}

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
  bool redraw = false;
  
  if (t->selections != NULL) {
    redraw = removeselections(t);
  }

  if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
    return redraw;
  }

  textboxcalcpositions(t, t->cursor);

  t->cursor += l;

  textboxfindstart(t);

  textboxpredraw(t);

  return true;
}

bool
textboxkeypress(struct textbox *t, keycode_t k)
{
  bool removed = false, redraw = false;
  uint8_t s[16];
  size_t l;

  if (t->selections != NULL) {
    redraw = removeselections(t);
    removed = true;
  }

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
    
  case KEY_tab:
    break;

  case KEY_return:
    l = snprintf((char *) s, sizeof(s), "\n");

    if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
      return redraw;
    }

    textboxcalcpositions(t, t->cursor);
    t->cursor += l;
    textboxpredraw(t);

    return true;
  
  case KEY_delete:
    if (!removed) {
      if (sequencedelete(t->sequence, t->cursor, 1)) {
	textboxcalcpositions(t, t->cursor);

	t->startpiece = SEQ_start;  
	textboxfindstart(t);
	textboxpredraw(t);
	return true;
      } else {
	return redraw;
      }
    } 

    break;
    
  case KEY_backspace:
    if (!removed && t->cursor > 0) {
      if (sequencedelete(t->sequence, t->cursor - 1, 1)) {
	t->cursor--;
	textboxcalcpositions(t, t->cursor);

	t->startpiece = SEQ_start;  
	textboxfindstart(t);
	textboxpredraw(t);
	return true;
      } else {
	return redraw;
      }
    } 

    break;
    
  case KEY_escape:
    break;
  }

  return redraw;
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

    textboxfindstart(t);
    textboxpredraw(t);

    return true;
  } else {
    return false;
  }
}
