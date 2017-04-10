#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

struct selection *
selectionnew(struct textbox *t,
	     struct colour *fg, struct colour *bg,
	     unsigned int start, unsigned int end)
{
  struct selection *s;

  s = malloc(sizeof(struct selection));
  if (s == NULL) {
    return NULL;
  }

  s->textbox = t;
  s->start = start;
  s->end = end;
  s->increasing = true;

  memmove(&s->fg, fg, sizeof(struct colour));
  memmove(&s->bg, bg, sizeof(struct colour));
  
  s->next = NULL;
  
  return s;
}

void
selectionfree(struct selection *s)
{
  free(s);
}

bool
selectionupdate(struct selection *s, unsigned int end)
{
  if (s->increasing && end < s->start) {
    s->end = s->start;
    s->start = end;
    s->increasing = false;
    return true;
  } else if (!s->increasing && end > s->start) {
    s->start = s->end;
    s->end = end;
    s->increasing = true;
    return true;
  } else if (!s->increasing && end != s->start) {
    s->start = end;
    return true;
  } else if (s->increasing && end != s->end) {
    s->end = end;
    return true;
  } else {
    return false;
  }
}

struct selection *
inselections(struct textbox *t, unsigned int pos)
{
  struct selection *s;

  for (s = selections; s != NULL; s = s->next) {
    if (s->textbox == t && s->start <= pos && pos <= s->end) {
      return s;
    }
  }

  return NULL;
}

uint8_t *
selectiontostring(struct selection *s)
{
  struct piece *p;
  int pos, b, l;
  uint8_t *buf;

  buf = malloc(sizeof(uint8_t) * (s->end - s->start + 1));
  if (buf == NULL) {
    return NULL;
  }
  
  pos = 0;
  for (p = s->textbox->pieces; p != NULL; p = p->next) {
    if (pos > s->end) {
      break;
    } else if (pos + p->pl < s->start) {
      pos += p->pl;
      continue;
    }

    if (pos < s->start) {
      b = s->start - pos;
    } else {
      b = 0;
    }

    if (pos + p->pl > s->end + 1) {
      l = s->end + 1 - pos - b;
    } else {
      l = p->pl - b;
    }

    memmove(buf + (pos + b - s->start), p->s + b, l);
    pos += b + l;
  }

  buf[pos - s->start] = 0;
  return buf;
}

