#include "mace.h"

struct selection *
selectionadd(struct textbox *t, selection_t type,
             size_t pos1, size_t pos2)
{
  struct selection *s;

  s = malloc(sizeof(struct selection));
  if (s == NULL) {
    return NULL;
  }

  s->textbox = t;
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

  s->next = t->selections;
	t->selections = s;
  
  return s;
}

void
selectionremove(struct textbox *t, struct selection *s)
{
	struct selection *p;

	if (t->selections == s) {
		t->selections = s->next;
	} else {
		for (p = t->selections; p->next != s && p->next != NULL; p = p->next)
			;

		if (p->next == s) {
			p->next = s->next;
		}
	}

  free(s);
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
inselections(struct textbox *t, size_t pos)
{
  struct selection *s;

  for (s = t->selections; s != NULL; s = s->next) {
    if (s->start <= pos && pos <= s->end) {
      return s;
    }
  }

  return NULL;
}

