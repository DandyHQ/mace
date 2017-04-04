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
selectionnew(unsigned int start, unsigned int end)
{
  struct selection *s;

  s = malloc(sizeof(struct selection));
  if (s == NULL) {
    return NULL;
  }

  s->start = start;
  s->end = end;
  s->increasing = true;

  s->next = NULL;
  
  return s;
}

void
selectionfree(struct selection *s)
{
  free(s);
}

void
selectionupdate(struct selection *s, unsigned int end)
{
  if (s->increasing && end < s->start) {
    s->end = s->start;
    s->start = end;
    s->increasing = false;
  } else if (!s->increasing && end > s->start) {
    s->start = s->end;
    s->end = end;
    s->increasing = true;
  } else if (!s->increasing && end != s->start) {
    s->start = end;
  } else if (s->increasing && end != s->end) {
    s->end = end;
  }
}
