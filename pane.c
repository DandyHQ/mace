#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

struct pane *
panenew(struct pane *parent, struct tab *tabs)
{
  struct pane *p;
  
  p = malloc(sizeof(struct pane));
  if (p == NULL) {
    return NULL;
  }

  p->parent = parent;
  p->type = PANE_norm;
  p->norm.loff = 0;

  p->norm.tabs = p->norm.focus = tabs;

  return p;
}

void
panefree(struct pane *p)
{
  free(p);
}

struct pane *
findpane(struct pane *p,
	 int x, int y)
{
  int r;
  
  switch (p->type) {
  case PANE_norm:
    return p;

  case PANE_hsplit:
    r = (int) (p->split.ratio * p->width);

    if (x < p->x + r) {
      return findpane(p->split.a,
		      x, y);
    } else {
      return findpane(p->split.b,
		      x, y);
    }
    
  case PANE_vsplit:
    r = (int) (p->split.ratio * p->height);

    if (y < p->y + r) {
      return findpane(p->split.a,
		      x, y);
    } else {
      return findpane(p->split.b,
		      x, y);
    }

  default:
    return NULL;
  }
}

struct pane *
panesplit(struct pane *h, struct tab *t, pane_t type, bool na)
{
  struct pane *a, *b, *n, *o;
  int r;

  if (na) {
    n = a = panenew(h, t);
    if (a == NULL) {
      return NULL;
    }
    
    o = b = panenew(h, h->norm.tabs);
    if (b == NULL) {
      free(a);
      return NULL;
    }
  } else {
    o = a = panenew(h, h->norm.tabs);
    if (a == NULL) {
      return NULL;
    }

    n = b = panenew(h, t);
    if (b == NULL) {
      free(a);
      return NULL;
    }
  }

  o->norm.tabs = h->norm.tabs;
  o->norm.focus = h->norm.focus;
  o->norm.loff = h->norm.loff;
  
  n->norm.tabs = n->norm.focus = t;
  n->norm.loff = 0;

  h->split.ratio = 0.5f;
  h->split.a = a;
  h->split.b = b;
  h->type = type;

  a->x = h->x;
  a->y = h->y;

  switch (type) {
  case PANE_hsplit:
    r = (int) (h->split.ratio * (float) h->width);
    a->width = r;
    a->height = h->height;

    b->x = h->x + r;
    b->y = h->y;

    b->width = h->width - r;
    b->height = h->height;
    break;

  case PANE_vsplit:
    r = (int) (h->split.ratio * (float) h->height);
    a->width = h->width;
    a->height = r;

    b->x = h->x;
    b->y = h->y + r;

    b->width = h->width;
    b->height = h->height - r;
    break;

  default:
    err(1, "Invalid pane type!\n");
  }

  return n;
}

void
paneresize(struct pane *p, int x, int y, int w, int h)
{
  struct tab *t;
  int r;

  p->x = x;
  p->y = y;
  p->width = w;
  p->height = h;

  switch (p->type) {
  case PANE_norm:
    for (t = p->norm.tabs; t != NULL; t = t->next) {
      t->action.width = w;
      t->main.width = w;
    }
    
    break;

  case PANE_hsplit:
    r = (int) (p->split.ratio * (float) w);

    paneresize(p->split.a,
	       x, y,
	       r, h);

    paneresize(p->split.b,
	       x + r, y,
	       w - r, h);

    break;

  case PANE_vsplit:
    r = (int) (p->split.ratio * (float) h);

    paneresize(p->split.a,
	       x, y,
	       w, r);
    
    paneresize(p->split.b,
	       x, y + r,
	       w, h - r);
    break;
  }
}

void
paneremovetab(struct pane *p, struct tab *t)
{
  struct tab **tp;

  tp = &p->norm.tabs;

  if (p->norm.focus == t) {
    if (t->next != NULL) {
      p->norm.focus = t->next;
    } else {
      while ((*tp) != t && (*tp)->next != t)
	tp = &(*tp)->next;

      if (*tp == t) {
	p->norm.focus = (*tp)->next;
      } else {
	p->norm.focus = *tp;
      }
    }
  }
  
  while (*tp != t)
    tp = &(*tp)->next;

  *tp = t->next;
  t->next = NULL;
}

void
paneremove(struct pane *p)
{
  struct pane *parent, *s;

  parent = p->parent;

  if (p == parent->split.a) {
    s = parent->split.b;
  } else {
    s = parent->split.a;
  }

  parent->norm.tabs = s->norm.tabs;
  parent->norm.focus = s->norm.focus;
  parent->norm.loff = s->norm.loff;
  parent->type = s->type;
  
  s->norm.tabs = NULL;
  s->norm.focus = NULL;

  panefree(s);
  panefree(p);
}
