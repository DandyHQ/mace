#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

struct tab *
tabnew(uint8_t *name)
{
  struct tab *t;

  t = malloc(sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }

  t->next = NULL;
  t->voff = 0;

  strlcpy(t->name, name, NAMEMAX);

  t->buf = malloc(tabwidth * listheight * 4);
  if (t->buf == NULL) {
    free(t);
    return NULL;
  }
 
  tabprerender(t);
  
  return t;
}

void
tabfree(struct tab *t)
{
  free(t->buf);
  free(t);
}

void
tabprerender(struct tab *t)
{
  struct colour border = { 100, 100, 100, 255 };
  int xx, ww;
  char *s;

  drawline(t->buf, tabwidth, listheight,
	   0, 0,
	   tabwidth - 1, 0,
	   &border);

  drawline(t->buf, tabwidth, listheight,
	   0, listheight - 1,
	   tabwidth - 1, listheight - 1,
	   &border);

  drawline(t->buf, tabwidth, listheight,
	   tabwidth - 1, 0,
	   tabwidth - 1, listheight - 1,
	   &border);

  drawline(t->buf, tabwidth, listheight,
	   0, 0,
	   0, listheight - 1,
	   &border);

  drawrect(t->buf, tabwidth, listheight,
	   1, 1,
	   tabwidth - 2, listheight - 2,
	   &bg);
 
  xx = 10; /* Padding */
  for (s = t->name; *s && xx < tabwidth; s++) {
    if (!loadchar(*s)) {
      printf("failed to load glyph for %c\n", *s);
      continue;
    }

    ww = face->glyph->advance.x >> 6;

    drawglyph(t->buf, tabwidth, listheight,
	      xx + face->glyph->bitmap_left,
	      listheight - 4 - face->glyph->bitmap_top,
	      0, 0,
	      face->glyph->bitmap.width,
	      face->glyph->bitmap.rows,
	      &fg);

    xx += ww;
  }
}

