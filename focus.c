#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

bool
handlepanetablistscroll(struct pane *p, int x, int y, int dx, int dy)
{
  struct tab *t;
  int w;

  w = 0;
  for (t = p->norm.tabs; t != NULL; t = t->next)
    w += tabwidth;

  if (p->norm.loff + dy > 0) {
    p->norm.loff = 0;
  } else if (p->norm.loff + dy < -w) {
    p->norm.loff = -w;
  } else {
    p->norm.loff += dy;
  }

  return true;
}

bool
handlepanescroll(struct pane *p, int x, int y, int dx, int dy)
{
  return false;
}

bool
handlepanepress(struct pane *p, int x, int y,
		unsigned int button)
{
  struct tab *f;

  focus = p;
  
  f = p->norm.focus;

  if (y < p->y + lineheight + f->action.height) {
    focustype = FOCUS_action;

    if (textboxbuttonpress(&f->action,
			   x - p->x,
			   y - p->y - lineheight,
			   button)) {
      panedraw(p);
      return true;
    }
  } else {
    focustype = FOCUS_main;

    if (textboxbuttonpress(&f->main,
			   x - p->x,
			   y - p->y - lineheight - f->action.height,
			   button)) {
      panedraw(p);
      return true;
    }
  }

  return false;
}

bool
handlepanerelease(struct pane *p, int x, int y,
		  unsigned int button)
{
  struct tab *f;

  f = p->norm.focus;

  if (y < p->y + lineheight + f->action.height) {
    if (textboxbuttonrelease(&f->action,
			     x - p->x,
			     y - p->y - lineheight,
			     button)) {
      panedraw(p);
      return true;
    }
  } else {
    if (textboxbuttonrelease(&f->main,
			     x - p->x,
			     y - p->y - lineheight - f->action.height,
			     button)) {
      panedraw(p);
      return true;
    }
  }

  return false;
}

bool
handlepanemotion(struct pane *p, int x, int y)
{
  struct tab *f;

  f = p->norm.focus;

  if (y < p->y + lineheight + f->action.height) {
    if (textboxmotion(&f->action,
		      x - p->x,
		      y - p->y - lineheight)) {
      panedraw(p);
      return true;
    }
  } else {
    if (textboxmotion(&f->main,
		      x - p->x,
		      y - p->y - lineheight - f->action.height)) {
      panedraw(p);
      return true;
    }
  }

  return false;
}

bool
handletyping(uint8_t *s, size_t n)
{
  struct textbox *tb;
  struct tab *f;

  f = focus->norm.focus;

  switch (focustype) {
  case FOCUS_action:
    tb = &f->action;
    break;
  case FOCUS_main:
    tb = &f->main;
    break;
  default:
    return false;
  }
  
  if (textboxtyping(tb, s, n)) {
    panedraw(focus);
    return true;
  } else {
    return false;
  }
}

bool
handlekeypress(keycode_t k)
{
  struct textbox *tb;
  struct tab *f;

  f = focus->norm.focus;

  switch (focustype) {
  case FOCUS_action:
    tb = &f->action;
    break;
  case FOCUS_main:
    tb = &f->main;
    break;
  default:
    return false;
  }
  
  if (textboxkeypress(tb, k)) {
    panedraw(focus);
    return true;
  } else {
    return false;
  }
}

bool
handlekeyrelease(keycode_t k)
{
  struct textbox *tb;
  struct tab *f;

  f = focus->norm.focus;

  switch (focustype) {
  case FOCUS_action:
    tb = &f->action;
    break;
  case FOCUS_main:
    tb = &f->main;
    break;
  default:
    return false;
  }
  
  if (textboxkeyrelease(tb, k)) {
    panedraw(focus);
    return true;
  } else {
    return false;
  }
}
