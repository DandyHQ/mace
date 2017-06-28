#include "mace.h"

struct textbox *
textboxnew(struct mace *m, struct tab *tab,
                   struct colour *bg,
                   struct sequence *seq)
{
  struct textbox *t;
  
  t = calloc(1, sizeof(struct textbox));
  if (t == NULL) {
    return NULL;
  }
  
	t->mace = m;
	t->tab = tab;
  t->sequence = seq;
  t->tab = tab;
  
  t->cursor = 0;

  t->newselpos = SIZE_MAX;
  t->csel = NULL;

  t->yoff = 0;
	t->linewidth = 0;

  memmove(&t->bg, bg, sizeof(struct colour));

  return t;
}

void
textboxfree(struct textbox *t)
{
	struct selection *s, *sn;

	for (s = t->mace->selections; s != NULL; s = sn) {
		sn = s->next;
		if (s->textbox == t) {
			selectionremove(s);
		}
	}

  sequencefree(t->sequence);

  free(t);
}

bool
textboxresize(struct textbox *t, int lw)
{
	t->linewidth = lw;
  sequencesetlinewidth(t->sequence, lw - PAD * 2);

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
    while (t->mace->selections != NULL) {
      selectionremove(t->mace->selections);
    }
    
    t->mace->keyfocus = t;

    t->cursor = pos;
    t->newselpos = pos;

    return true;

  case 3:
    sel = inselections(t, pos);
    if (sel != NULL) {
      sel->type = SELECTION_command;
      return true;

    } else if (sequencefindword(t->sequence, pos, &start, &len)) {
      sel = selectionadd(t, SELECTION_command, start, start + len);
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
      len = sel->end - start;
    } else {
      len = 0;
    }

    for (sel = t->mace->selections; sel != NULL; sel = next) {
      next = sel->next;
      if (sel->type == SELECTION_command) {
				selectionremove(sel);
      }
    }

    if (len != 0) {
      buf = malloc(len);
      if (buf == NULL) {
				return false;
      }

      if (sequenceget(t->sequence, start, buf, len) == 0) {
				return false;
      }

      /* TODO: Show an error somehow if this returns false. */
      command(t->mace, buf);

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
    return true;

  } else if (t->csel != NULL) {
    pos = textboxfindpos(t, x, y);

    if (selectionupdate(t->csel, pos)) {
      /* TODO: set cursor to pos + next code point length */
      t->cursor = pos;
      return true;
    }
  }
  
  return false;
}

static bool
deleteselections(struct textbox *t)
{
  struct selection *s, *n;
    
  for (s = t->mace->selections; s != NULL; s = n) {
		n = s->next;
		if (s->textbox != t) continue;

    if (!sequencedelete(t->sequence, s->start, s->end - s->start)) {
			continue;
    }

    if (s->start <= t->cursor && t->cursor <= s->end) {
      t->cursor = s->start;
    }

    selectionremove(s);
  }

  return true;
}

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l)
{
	deleteselections(t);

  if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
    return true;
  }

  t->cursor += l;

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
		break;
    
  case KEY_pagedown:
		break;
    
  case KEY_home:
    break;
    
  case KEY_end:
    break;
    
  case KEY_tab:
    deleteselections(t);

    l = snprintf((char *) s, sizeof(s), "\t");
    
    if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
      return true;
    }

    t->cursor += l;

    return true;

  case KEY_return:
    deleteselections(t);

    l = snprintf((char *) s, sizeof(s), "\n");

    if (!sequenceinsert(t->sequence, t->cursor, s, l)) {
      return true;
    }

    t->cursor += l;

    return true;
  
  case KEY_delete:
    if (t->mace->selections != NULL) {
      return deleteselections(t);

    } else if (sequencedelete(t->sequence, t->cursor, 1)) {
      return true;
    } 

    break;
    
  case KEY_backspace:
    if (t->mace->selections != NULL) {
      return deleteselections(t);

    } else if (t->cursor > 0 && sequencedelete(t->sequence,
					       t->cursor - 1, 1)) {

      t->cursor--;
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
	int h;

	h = sequenceheight(t->sequence);

  if (yoff < 0) {
    yoff = 0;
  } else if (yoff > h - t->mace->font->lineheight) {
    yoff = h - t->mace->font->lineheight;
  }

  if (yoff != t->yoff) {
    t->yoff = yoff;
    return true;
  } else {
    return false;
  }
}
