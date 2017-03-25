#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

struct line *
linenew(char *s, size_t n)
{
  struct line *l;

  l = malloc(sizeof(struct line));
  if (l == NULL) {
    return NULL;
  }

  l->s = s;
  l->n = n;

  l->next = NULL;
  
  return l;
}

void
linefree(struct line *l)
{
  free(l->s);
  free(l);
}

struct line *
findpos(struct line *l,
	int x, int y, int linewidth,
	int *pos)
{
  int a, i, xx, yy, ww;

  xx = 0;
  yy = 0;

  while (l != NULL) {
    for (i = 0; i < l->n; i += a) {
      if ((a = loadglyph(l->s + i)) == -1) {
	a = 1;
	continue;
      }

      ww = face->glyph->advance.x >> 6;

      if (xx + ww >= linewidth) {
	if (yy < y && y <= yy + lineheight) {
	  *pos = i;
	  return l;
	} else {
	  yy += lineheight;
	  xx = 0;
	}
      }

      if (yy < y && y <= yy + lineheight && xx < x && x <= xx + ww) {
	*pos = i;
	return l;
      } else {
	xx += ww;
      }
    }
  
    if (yy < y && y <= yy + lineheight) {
      *pos = l->n;
      return l;
    } else {
      l = l->next;
      yy += lineheight;
      xx = 0;
    }
  }

  *pos = yy;
  return NULL;
}

