#include "mace.h"

struct selection *
selectionnew(struct textbox *t, size_t pos)
{
  struct selection *s;

  s = malloc(sizeof(struct selection));
  if (s == NULL) {
    return NULL;
  }

  s->textbox = t;
  s->start = pos;
  s->end = pos;
  s->direction = SELECTION_right;
  s->next = NULL;
  
  return s;
}

void
selectionfree(struct selection *s)
{
  luaremove(s->textbox->tab->mace->lua, s);

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

