#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

/* TODO: Make pretty */

int
panedrawtablist(struct pane *p)
{
  struct tab *t;
  int x, w;

  drawrect(buf, width, height,
	   p->x, p->y,
	   p->x + p->width - 2, p->y + lineheight - 1,
	   &bg);

  drawline(buf, width, height,
	   p->x + p->width - 1, p->y,
	   p->x + p->width - 1, p->y + lineheight - 1,
	   &fg);
 
  x = 0;
  for (t = p->norm.tabs; t != NULL && x < p->width; t = t->next) {
    if (x + tabwidth < p->width) {
      w = tabwidth;
    } else {
      w = p->width - x;
    }

    drawstring(buf, width, height,
	       p->x + x, p->y,
	       0, 0,
	       w, lineheight,
	       t->name, true,
	       &fg);

    drawline(buf, width, height,
	     p->x + x + tabwidth, p->y,
	     p->x + x + tabwidth, p->y + lineheight - 1,
	     &fg);

    drawline(buf, width, height,
	     p->x + x, p->y,
	     p->x + x + w, p->y,
	     &fg);

    if (p->norm.focus == t) {
      drawline(buf, width, height,
	       p->x + x + 1, p->y + lineheight - 1,
	       p->x + w - 1, p->y + lineheight - 1,
	       &bg);
    } else {
      drawline(buf, width, height,
	       p->x + x, p->y + lineheight - 1,
	       p->x + w, p->y + lineheight - 1,
	       &fg);
    }

    x += tabwidth;
  }

  if (x < p->width) {
    drawline(buf, width, height,
	     p->x + x, p->y + lineheight - 1,
	     p->x + p->width - 1, p->y + lineheight - 1,
	     &fg);
  }

  return lineheight;
}

void
panedrawfocus(struct pane *p)
{
  tabdraw(p->norm.focus,
	  p->x, p->y + lineheight,
	  p->width, p->height - lineheight);
}

void
panedraw(struct pane *p)
{
  switch (p->type) {
  case PANE_norm:
    panedrawtablist(p);
    panedrawfocus(p);
    break;

  case PANE_hsplit:
  case PANE_vsplit:
    panedraw(p->split.a);
    panedraw(p->split.b);
    break;
  }
}
