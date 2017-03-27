#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

struct tab *
tabnew(uint8_t *name)
{
  struct tab *t;
  uint8_t *s;
  size_t n;

  t = malloc(sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }

  t->next = NULL;
  t->voff = 0;

  strlcpy((char *) t->name, (char *) name, NAMEMAX);

  t->buf = malloc(tabwidth * lineheight * 4);
  if (t->buf == NULL) {
    free(t);
    return NULL;
  }

  s = malloc(sizeof(uint8_t) * 128);
  if (s == NULL) {
    free(buf);
    free(t);
    return NULL;
  }
  
  n = snprintf((char *) s, sizeof(uint8_t) * 128,
	       "%s save copy cut search", name);

  t->action = piecenew(s, sizeof(uint8_t) * 128, n);
  if (t->action == NULL) {
    free(s);
    free(buf);
    free(t);
    return NULL;
  }

  t->action->prev = &t->action;
  t->action->next = NULL;
  
  t->acursor = n;
 
  s = malloc(sizeof(uint8_t) * 128);
  if (s == NULL) {
    free(buf);
    free(t);
    return NULL;
  }
  
  n = strlcpy((char *) s, "", sizeof(uint8_t) * 128);

  t->main = piecenew(s, sizeof(uint8_t) * 128, n);
  if (t->main == NULL) {
    piecefree(t->action);
    free(s);
    free(buf);
    free(t);
    return NULL;
  }

  t->main->prev = &t->main;
  t->main->next = NULL;

  t->mcursor = n;

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

  drawline(t->buf, tabwidth, lineheight,
	   0, 0,
	   tabwidth - 1, 0,
	   &border);

  drawline(t->buf, tabwidth, lineheight,
	   0, lineheight - 1,
	   tabwidth - 1, lineheight - 1,
	   &border);

  drawline(t->buf, tabwidth, lineheight,
	   tabwidth - 1, 0,
	   tabwidth - 1, lineheight - 1,
	   &border);

  drawline(t->buf, tabwidth, lineheight,
	   0, 0,
	   0, lineheight - 1,
	   &border);

  drawrect(t->buf, tabwidth, lineheight,
	   1, 1,
	   tabwidth - 2, lineheight - 2,
	   &bg);

  drawstring(t->buf, tabwidth, lineheight,
	     PADDING, 0,
	     0, 0,
	     tabwidth - PADDING, lineheight,
	     t->name, true, &fg);
}

