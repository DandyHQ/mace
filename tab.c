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
tabnew(char *name)
{
  struct tab *t;
  size_t n;
  char *s;

  t = malloc(sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }

  t->next = NULL;
  t->voff = 0;

  strlcpy(t->name, name, NAMEMAX);

  t->buf = malloc(tabwidth * lineheight * 4);
  if (t->buf == NULL) {
    free(t);
    return NULL;
  }

  s = malloc(sizeof(char) * 128);
  if (s == NULL) {
    free(buf);
    free(t);
    return NULL;
  }
  
  n = snprintf(s, sizeof(char) * 128,
	       "%s save copy cut search", name);

  t->action = piecenew(s, n);
  if (t->action == NULL) {
    free(s);
    free(buf);
    free(t);
    return NULL;
  }

  t->action->prev = &t->action;
  t->action->next = NULL;
  
  t->actionfocus.piece = t->action;
  t->actionfocus.pos = n;
 
 
  s = malloc(sizeof(char) * 128);
  if (s == NULL) {
    free(buf);
    free(t);
    return NULL;
  }
  
  n = strlcpy(s, "", sizeof(char) * 128);

  t->main = piecenew(s, n);
  if (t->main == NULL) {
    piecefree(t->action);
    free(s);
    free(buf);
    free(t);
    return NULL;
  }

  t->main->prev = &t->main;
  t->main->next = NULL;

  t->mainfocus.piece = t->main;
  t->mainfocus.pos = n;

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

