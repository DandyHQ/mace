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
		if (s->tb == t) {
			selectionremove(t->mace, s);
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
    selectionremoveall(t->mace);
		cursorremoveall(t->mace);

		t->ccur = cursoradd(t->mace, t, pos);
    t->newselpos = pos;

    return true;

	case 2:
		/* Temparary */

		t->ccur = cursoradd(t->mace, t, pos);
		t->newselpos = pos;

    return true;

  case 3:
    sel = inselections(t->mace, t, pos);
    if (sel != NULL) {
      sel->type = SELECTION_command;
      return true;

    } else if (sequencefindword(t->sequence, pos, &start, &len)) {
      selectionadd(t->mace, t, SELECTION_command,
			                    start, start + len);
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
  struct selection *sel, *seln;
  size_t pos, start, len;
  uint8_t *buf;

  t->csel = NULL;
	t->ccur = NULL;
  t->newselpos = SIZE_MAX;

  switch (button) {
  case 1:
  case 2:
    break;

  case 3:
	  pos = textboxfindpos(t, x, y);

    sel = inselections(t->mace, t, pos);
    if (sel != NULL && sel->type == SELECTION_command) {
      start = sel->start;
      len = sel->end - start;
		} else {
			len = 0;
		}

		sel = t->mace->selections;
		while (sel != NULL) {
			seln = sel->next;
			if (sel->type == SELECTION_command) {
				selectionremove(t->mace, sel);
			}
			sel = seln;
		}
    
		if (len > 0) {
      buf = malloc(len);
      if (buf == NULL) {
				return false;
      }

      if (sequenceget(t->sequence, start, buf, len) == 0) {
				free(buf);
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

    t->csel = selectionadd(t->mace, t, SELECTION_normal,
		                                   t->newselpos, pos);
    if (t->csel == NULL) {
      return false;
    }

    t->newselpos = SIZE_MAX;
    return true;

  } else if (t->csel != NULL && t->ccur != NULL) {
    pos = textboxfindpos(t, x, y);

		t->ccur->pos = pos;

    if (selectionupdate(t->csel, pos)) {
      return true;
    }
  }
  
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
