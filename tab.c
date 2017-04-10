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

  if (!textboxinit(&t->action, t, false, &abg)) {
    free(t);
    return NULL;
  }

  if (!textboxinit(&t->main, t, true, &bg)) {
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
  textboxresize(&t->action, w);
  textboxresize(&t->main, w);
}

void
tabdraw(struct tab *t, int x, int y, int w, int h)
{
  int ta, tm;
  
  drawline(buf, width, height,
	   x + w - 1, y,
	   x + w - 1, y + h - 1,
	   &fg);

  ta = t->action.height > h ? h : t->action.height;
  
  textboxdraw(&t->action, buf, width, height,
	      x, y, ta);
  
  drawline(buf, width, height,
	   x,
	   y + ta,
	   x + w - 2,
	   y + ta,
	   &fg);

  if (t->main.height - t->main.scroll > h - (ta + 1)) {
    tm = h - (ta + 1);
  } else {
    tm = t->main.height - t->main.scroll;
  }

  textboxdraw(&t->main, buf, width, height,
	      x, y + (ta + 1), tm);
  
  drawrect(buf, width, height,
	   x,
	   y + ta + 1 + tm,
	   x + w - 2,
	   y + h - 1,
	   &bg);
}

bool
tabscroll(struct tab *t, int x, int y, int dx, int dy)
{
  if (y < t->action.height) {
    if (textboxscroll(&t->action, dx, dy)) {
      return true;
    }
  } else {
    if (textboxscroll(&t->main, dx, dy)) {
      return true;
    }
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
  if (y < t->action.height) {
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

