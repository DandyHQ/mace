#include "mace.h"

struct cursor *
cursoradd(struct mace *m, struct textbox *t, 
                size_t pos)
{
  struct cursor *c;

  c = malloc(sizeof(struct cursor));
  if (c == NULL) {
    return NULL;
  }

  c->next = m->cursors;
	m->cursors = c;

  c->tb= t;
	c->pos = pos;

  return c;
}

void
cursorremove(struct mace *m, struct cursor *c)
{
	struct cursor *p;

	if (m->cursors == c) {
		m->cursors = c->next;
	} else {
		for (p = m->cursors; p->next != c && p->next != NULL; p = p->next)
			;

		if (p->next == c) {
			p->next = c->next;
		}
	}

  free(c);
}

void
cursorremoveall(struct mace *m)
{
	struct cursor *c, *n;

	c = m->cursors;
	m->cursors = NULL;
	while (c != NULL) {
		n = c->next;
		free(c);
		c = n;
	}
}

struct cursor *
cursorat(struct mace *m, struct textbox *t, size_t pos)
{
  struct cursor *c;

  for (c = m->cursors; c != NULL; c = c->next) {
    if (c->tb == t && c->pos == pos) {
      return c;
    }
  }

  return NULL;
}

void
shiftcursors(struct mace *m, struct textbox *t,
                   size_t from, int dist)
{
	struct cursor *c;

  for (c = m->cursors; c != NULL; c = c->next) {
    if (c->tb == t) {
			if (c->pos >= from) {
				if (dist < 0 && c->pos < -dist) {
					c->pos = 0;
				} else {
					c->pos += dist;
				}
			}
    }
  }
}
