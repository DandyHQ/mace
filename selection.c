#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mace.h"

static struct selection *
selectionnew(struct textbox *t, int32_t pos)
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

  return s;
}

struct selection *
selectionadd(struct textbox *t, int32_t pos)
{
  struct selection *s;

  s = selectionnew(t, pos);
  if (s == NULL) {
    return NULL;
  }

  s->next = mace->selections;
  mace->selections = s;

  return s;
}

struct selection *
selectionreplace(struct textbox *t, int32_t pos)
{
  struct selection *s, *o;

  s = mace->selections;
  while (s != NULL) {
    o = s->next;
    selectionremove(s);
    s = o;
  }
  
  s = selectionnew(t, pos);
  if (s == NULL) {
    return NULL;
  }
  
  s->next = NULL;
  mace->selections = s;

  return s;
}

void
selectionremove(struct selection *s)
{
  struct selection *o;

  if (mace->selections == s) {
    mace->selections = s->next;
  } else {
    for (o = mace->selections; o->next != s; o = o->next)
      ;

    o->next = s->next;
  }
  
  textboxpredraw(s->textbox, false, false);

  luaremove(s);
  free(s);
}

bool
selectionupdate(struct selection *s, int32_t pos)
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
inselections(struct textbox *t, int32_t pos)
{
  struct selection *s;

  for (s = mace->selections; s != NULL; s = s->next) {
    if (s->textbox == t && s->start <= pos && pos <= s->end) {
      return s;
    }
  }

  return NULL;
}

