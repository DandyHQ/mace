#include "mace.h"

struct selection *
selectionadd(struct mace *m, struct textbox *t, 
                    selection_t type, size_t pos1, size_t pos2)
{
  struct selection *s;

  s = malloc(sizeof(struct selection));
  if (s == NULL) {
    return NULL;
  }

  s->next = m->selections;
	m->selections = s;

  s->tb= t;
	s->type = type;

	if (pos1 < pos2) {
  	s->start = pos1;
  	s->end = pos2;
 	 s->direction = SELECTION_right;
	} else {
		s->start = pos2;
		s->end = pos1;
		s->direction = SELECTION_left;
	}

  return s;
}

void
selectionremove(struct mace *m, struct selection *s)
{
	struct selection **p;

	for (p = &m->selections; *p != NULL && *p != s; p = &(*p)->next)
		;

	*p = s->next;

  free(s);
}

void
selectionremoveall(struct mace *m)
{
	struct selection *s, *n;

	s = m->selections;
	m->selections = NULL;
	while (s != NULL) {
		n = s->next;
		free(s);
		s = n;
	}
}

bool
selectionupdate(struct selection *s, size_t pos)
{
  if (s->direction == SELECTION_right) {
    if (pos < s->start) {
      s->direction = SELECTION_left;
      s->end = s->start;
      s->start = pos;
      return true;
    } else if (pos != s->end) {
      s->end = pos;
      return true;
    } else {
      return false;
    }
  } else {
    if (pos > s->end) {
      s->direction = SELECTION_right;
      s->start = s->end;
      s->end = pos;
      return true;
    } else if (pos != s->start) {
      s->start = pos;
      return true;
    } else {
      return false;
    }
  }
}

struct selection *
inselections(struct mace *m, struct textbox *t, size_t pos)
{
  struct selection *s;

  for (s = m->selections; s != NULL; s = s->next) {
    if (s->tb == t && s->start <= pos && pos < s->end) {
      return s;
    }
  }

  return NULL;
}

/* This isn't very well done. */

void
shiftselections(struct mace *m, struct textbox *t,
                      size_t from, int dist)
{
	struct selection *s;

	for (s = m->selections; s != NULL; s = s->next) {
		if (s->tb != t) continue;

		if (s->start >= from) {
			s->start += dist;
			s->end += dist;
		}
	}
}
