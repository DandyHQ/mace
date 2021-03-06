#include "mace.h"

struct textbox *
textboxnew(struct mace *m,
           struct colour *bg,
           struct sequence *seq)
{
  struct textbox *t;
  t = calloc(1, sizeof(struct textbox));

  if (t == NULL) {
    return NULL;
  }

  t->mace = m;
  t->sequence = seq;
  t->curcs = NULL;
  t->start = 0;
  t->linewidth = 0;
  t->nglyphs = 0;
  t->nglyphsmax = 1024;
  t->glyphs = malloc(sizeof(cairo_glyph_t) * t->nglyphsmax);

  if (t->glyphs == NULL) {
    free(t);
    return NULL;
  }

  memmove(&t->bg, bg, sizeof(struct colour));
  return t;
}

void
textboxfree(struct textbox *t)
{
  struct cursel *c, *cn;

  for (c = t->mace->cursels; c != NULL; c = cn) {
    cn = c->next;

    if (c->tb == t) {
      curselremove(t->mace, c);
    }
  }

  sequencefree(t->sequence);
  free(t->glyphs);
  free(t);
}

bool
textboxresize(struct textbox *t, int lw, int h)
{
  t->linewidth = lw;
  t->maxheight = h;
  t->nglyphs = 0;

  textboxplaceglyphs(t);
  return true;
}

bool
textboxbuttonpress(struct textbox *t, int x, int y,
                   unsigned int button)
{
  struct cursel *c;
  size_t pos, start, len;
  pos = textboxfindpos(t, x, y);

  switch (button) {
  case 1:
    curselremoveall(t->mace);
    t->curcs = curseladd(t->mace, t, CURSEL_nrm, pos);
    return true;

  case 2:
    /* Temparary */
    t->curcs = curseladd(t->mace, t, CURSEL_nrm, pos);
    return true;

  case 3:
    c = curselat(t->mace, t, pos);

    if (c != NULL && c->start != c->end) {
      c->type |= CURSEL_cmd;
      return true;
    } else if (sequencefindword(t->sequence, pos, &start,
                                &len)) {
      c = curseladd(t->mace, t, CURSEL_cmd, start);
      curselupdate(c, start + len);
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
  struct cursel *c, *cn;
  size_t pos, start, len;
  uint8_t *buf;
  t->curcs = NULL;

  switch (button) {
  case 1:
  case 2:
    break;

  case 3:
    pos = textboxfindpos(t, x, y);
    c = curselat(t->mace, t, pos);

    if (c != NULL && (c->type & CURSEL_cmd) != 0) {
      start = c->start;
      len = c->end - start;
    } else {
      len = 0;
    }

    for (c = t->mace->cursels; c != NULL; c = cn) {
      cn = c->next;

      if ((c->type & CURSEL_cmd) != 0) {
        if ((c->type & CURSEL_nrm) != 0) {
          c->type &= ~CURSEL_cmd;
        } else {
          curselremove(t->mace, c);
        }
      }
    }

    if (len > 0) {
      buf = malloc(len + 1);

      if (buf == NULL) {
        return false;
      }

      if (sequenceget(t->sequence, start, buf, len) == 0) {
        free(buf);
        return false;
      }

      buf[len] = 0;
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

  if (t->curcs != NULL) {
    pos = textboxfindpos(t, x, y);
    return curselupdate(t->curcs, pos);
  } else {
    return false;
  }
}

static size_t
scrollnewindex(struct textbox *t, size_t i, int lines)
{
  if (lines > 0) {
    while (lines-- > 0) {
      i = textboxindexbelow(t, i);
    }
  } else if (lines < 0) {
    while (lines++ < 0) {
      i = textboxindexabove(t, i);
    }
  }

  return i;
}

bool
textboxscroll(struct textbox *t, int lines)
{
  size_t newstart;
  newstart = scrollnewindex(t, t->start, lines);

  if (newstart != t->start) {
    t->start = newstart;
    textboxplaceglyphs(t);
    return true;
  } else {
    return false;
  }
}
