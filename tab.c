#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

struct tab *
tabnew(uint8_t *name)
{
  uint8_t s[128];
  struct tab *t;
  size_t n;

  t = malloc(sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }

  if (!textboxinit(&t->action, t, &abg, &sfg, &sbg)) {
    free(t);
    return NULL;
  }

  if (!textboxinit(&t->main, t, &bg, &sfg, &sbg)) {
    textboxfree(&t->action);
    free(t);
    return NULL;
  }
  
  n = snprintf((char *) s, sizeof(s),
	       "%s save cut copy paste search", name);

  if (pieceinsert(t->action.pieces->next, 0, s, n) == NULL) {
    textboxfree(&t->main);
    textboxfree(&t->action);
    free(t);
    return NULL;
  }

  t->action.cursor = n;

  t->scroll = 0;
  
  t->next = NULL;

  strncpy((char *) t->name, (char *) name, NAMEMAX);

  return t;
} 

void
tabfree(struct tab *t)
{
  textboxfree(&t->action);
  textboxfree(&t->main);
  free(t);
}

void
tabresize(struct tab *t, int w, int h)
{
  textboxresize(&t->action, w - 1);
  textboxresize(&t->main, w - 1);
}

bool
tabscroll(struct tab *t, int x, int y, int dx, int dy)
{
  if (y > t->action.height) {
    t->scroll += dy;
    if (t->scroll < 0) {
      t->scroll = 0;
    } else if (t->scroll > t->main.height - lineheight) {
      t->scroll = t->main.height - lineheight;
    }

    printf("tab scrolled to %i\n", t->scroll);
    return true;
  }
  
  return false;
}

bool
tabpress(struct tab *t, int x, int y,
	 unsigned int button)
{
  if (y < t->action.height) {
    focus = &t->action;
    return textboxbuttonpress(&t->action, x, y, button);
  } else {
    focus = &t->main;
    return textboxbuttonpress(&t->main, x, y - t->action.height,
			      button);
  }
}

bool
tabrelease(struct tab *t, int x, int y,
	   unsigned int button)
{
  if (focus == &t->action) {
    return textboxbuttonrelease(&t->action, x, y, button);
  } else {
    return textboxbuttonrelease(&t->main, x, y - t->action.height,
				button);
  }
}

bool
tabmotion(struct tab *t, int x, int y)
{
  if (y < t->action.height) {
    return textboxmotion(&t->action, x, y);
  } else {
    return textboxmotion(&t->main, x, y - t->action.height);
  }
}

void
tabdraw(struct tab *t, int x, int y, int w, int h)
{
  int ta, tm;

  drawline(buf, width, height,
	   x + w - 1, y,
	   x + w - 1, y + h - 1,
	   &fg);
  
  ta = t->action.height > h - 1 ? h - 1 : t->action.height;

  drawbuffer(buf, width, height,
	     x, y,
	     t->action.buf, t->action.width, t->action.height,
	     0, 0,
	     t->action.width, ta);

  if (ta == h - 1) {
    return;
  }

  drawline(buf, width, height,
	   x,
	   y + ta,
	   x + w - 2,
	   y + ta,
	   &fg);
  
  if (t->main.height - t->scroll > h - 1 - ta - 1) {
    tm = h - 1 - ta - 1;
  } else {
    tm = t->main.height - t->scroll;
  }

  drawbuffer(buf, width, height,
	     x, y + ta + 1,
	     t->main.buf, t->main.width, t->main.height,
	     0, t->scroll,
	     t->main.width, tm);

  if (ta + 1 + tm < h - 1) {
    drawrect(buf, width, height,
	     x,
	     y + ta + 1 + tm,
	     x + w - 2,
	     y + h - 1,
	     &bg);
  }
}

