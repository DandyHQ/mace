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
  struct piece *ab, *ae, *mb, *me;
  uint8_t s[128];
  struct tab *t;
  size_t n;

  t = malloc(sizeof(struct tab));
  if (t == NULL) {
    goto error0;
  }

  t->buf = malloc(tabwidth * lineheight * 4);
  if (t->buf == NULL) {
    goto error1;
  }

  mb = piecenewtag();
  if (mb == NULL) {
    goto error5;
  }

  me = piecenewtag();
  if (me == NULL) {
    goto error6;
  }

  mb->next = me;
  me->prev = mb;
  t->main = mb;
  t->mcursor = 0;

  ab = piecenewtag();
  if (ab == NULL) {
    goto error2;
  }

  ae = piecenewtag();
  if (ae == NULL) {
    goto error3;
  }

  ab->next = ae;
  ae->prev = ab;
  
  n = snprintf((char *) s, sizeof(s),
	       "%s save cut copy paste search", name);

  if (pieceinsert(ae, 0, s, n) == NULL) {
    goto error4;
  }

  t->action = ab;
  t->acursor = n;
  t->actionbarheight = lineheight;
  
  t->next = NULL;
  t->voff = 0;

  strlcpy((char *) t->name, (char *) name, NAMEMAX);

  tabprerender(t);
 
  return t;

 error6:
  piecefree(me);
 error5:
  piecefree(mb);
 error4:
  piecefree(ae);
 error3:
  piecefree(ab);
 error2:
  free(buf);
 error1:
  free(t);
 error0:
  return NULL;
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

