#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

void
updatefocus(struct pane *p, int x, int y)
{
  struct piece *tp;
  struct tab *f;
  int pos;

  focus = p;
  
  f = p->norm.focus;

  if (y < p->y + lineheight + f->actionbarheight) {
    focustype = FOCUS_action;

    tp = findpos(f->action,
		 x - p->x - PADDING,
		 y - p->y - lineheight,
		 p->width - PADDING * 2,
		 &pos);

    if (tp != NULL) {
      f->acursor = pos;
    } else {
      printf("Something has gone wronge\n");
    }
  } else {
    focustype = FOCUS_main;

    tp = findpos(f->main,
		x - p->x - PADDING,
		y - p->y - lineheight - f->actionbarheight - f->voff,
		p->width - PADDING * 2,
		&pos);

    if (tp == NULL) {
      pos = 0;
      for (tp = f->main; tp->next != NULL; tp = tp->next) {
	pos += tp->pl;
      }
    }

    f->mcursor = pos;
  }

  panedraw(p);
}

bool
handlepanepress(struct pane *p, int x, int y,
		unsigned int button)
{
  updatefocus(p, x, y);

  return true;
}

bool
handlepanerelease(struct pane *p, int x, int y,
		  unsigned int button)
{
  return false;
}

bool
handlepanemotion(struct pane *p, int x, int y)
{
  return false;
}

bool
handleactiontyping(uint8_t *s, size_t l)
{
  struct piece *o, *n;
  struct tab *t;
  int i;

  t = focus->norm.focus;

  o = findpiece(t->action, t->acursor, &i);
  if (o == NULL) {
    return false;
  }

  n = pieceinsert(o, i, s, l);
  if (n == NULL) {
    return false;
  }

  t->acursor += l;
  
  panedraw(focus);
  return true;
}

bool
handleactionkeypress(keycode_t k)
{
  return false;
}

bool
handleactionkeyrelease(keycode_t k)
{
  return false;
}

bool
handlemaintyping(uint8_t *s, size_t l)
{
  struct piece *o, *n;
  struct tab *t;
  int i;

  t = focus->norm.focus;

  o = findpiece(t->main, t->mcursor, &i);
  if (o == NULL) {
    return false;
  }

  n = pieceinsert(o, i, s, l);
  if (n == NULL) {
    return false;
  }

  t->mcursor += l;
  
  panedraw(focus);
  return true;
}

bool
handlemainkeypress(keycode_t k)
{
  return false;
}

bool
handlemainkeyrelease(keycode_t k)
{
  return false;
}

bool (*typinghandlers[NFOCUS_T])(uint8_t *, size_t) = {
  [FOCUS_action]    = handleactiontyping,
  [FOCUS_main]      = handlemaintyping,
};

bool (*keypresshandlers[NFOCUS_T])(keycode_t) = {
  [FOCUS_action]    = handleactionkeypress,
  [FOCUS_main]      = handlemainkeypress,
};

bool (*keyreleasehandlers[NFOCUS_T])(keycode_t) = {
  [FOCUS_action]    = handleactionkeyrelease,
  [FOCUS_main]      = handlemainkeyrelease,
};

bool
handletyping(uint8_t *s, size_t n)
{
  return typinghandlers[focustype](s, n);
}

bool
handlekeypress(keycode_t k)
{
  return keypresshandlers[focustype](k);
}

bool
handlekeyrelease(keycode_t k)
{
  return keyreleasehandlers[focustype](k);
}
