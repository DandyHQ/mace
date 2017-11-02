#include "mace.h"

struct cursel *
curseladd(struct mace *m, struct textbox *t,
          int type, size_t pos)
{
  struct cursel *c, **h;
  c = malloc(sizeof(struct cursel));

  if (c == NULL) {
    return NULL;
  }

  c->tb = t;
  c->type = type | CURSEL_right;
  c->cur = 0;
  c->start = pos;
  c->end = pos;
  /* Insert new cursel in order */
  h = &m->cursels;

  while (*h != NULL) {
    if ((*h)->tb == c->tb && c->start < (*h)->start) {
      break;
    }

    h = &(*h)->next;
  }

  c->next = *h;
  *h = c;
  return c;
}

void
curselremove(struct mace *m, struct cursel *c)
{
  struct cursel **p;

  if (c == NULL) {
    return;
  }

  for (p = &m->cursels; *p != NULL
       && *p != c; p = &(*p)->next)
    ;

  if (*p == c) {
    *p = c->next;
  }

  free(c);
}

void
curselremoveall(struct mace *m)
{
  struct cursel *s, *n;

  for (s = m->cursels; s != NULL; s = n) {
    n = s->next;
    free(s);
  }

  m->cursels = NULL;
}

bool
curselupdate(struct cursel *s, size_t pos)
{
  if ((s->type & CURSEL_right) == CURSEL_right) {
    if (pos < s->start) {
      /* Going left */
      s->type &= ~CURSEL_right;
      s->end = s->start;
      s->start = pos;
      s->cur = 0;
      return true;

    } else if (pos != s->end) {
      /* Updating end */
      s->end = pos;
      s->cur = s->end - s->start;
      return true;
    }

  } else {
    if (pos > s->end) {
      /* Going right. */
      s->type |= CURSEL_right;
      s->start = s->end;
      s->end = pos;
      s->cur = s->end - s->start;
      return true;

    } else if (pos != s->start) {
      /* Changing start */
      s->start = pos;
      s->cur = 0;
      return true;
    }
  }

  /* No change */
  return false;
}

struct cursel *
curselat(struct mace *m, struct textbox *t, size_t pos)
{
  struct cursel *s;

  for (s = m->cursels; s != NULL; s = s->next) {
    if (s->tb == t && s->start <= pos && pos <= s->end) {
      return s;
    }
  }

  return NULL;
}

/* TODO: Improve this. */

void
shiftcursels(struct mace *m, struct textbox *t,
             size_t from, int dist)
{
  struct cursel *s;

  for (s = m->cursels; s != NULL; s = s->next) {
    if (s->tb != t) {
      continue;
    }

    if (s->start >= from) {
      s->start += dist;
      s->end += dist;
    }
  }
}
