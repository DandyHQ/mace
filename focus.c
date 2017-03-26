#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

void
updatefocus(struct pane *p, int x, int y)
{
  struct piece *tp;
  struct tab *f;
  int pos;

  focus = p;
  
  f = p->norm.focus;

  tp = findpos(f->action,
	      x - p->x - PADDING,
	      y - p->y - lineheight,
	      p->width - PADDING * 2,
	      &pos);

  if (tp != NULL) {
    focustype = FOCUS_action;
    
    printf("clicked in actionbar at pos %i\n", pos);
    f->actionfocus.piece = tp;
    f->actionfocus.pos = pos;
    
  } else {
    focustype = FOCUS_main;

    tp = findpos(f->main,
		x - p->x - PADDING,
		y - p->y - lineheight - pos - f->voff,
		p->width - PADDING * 2,
		&pos);

    if (tp == NULL) {
      tp = f->main;
      while (tp->next != NULL)
	tp = tp->next;

      pos = tp->n;
    }

    f->actionfocus.piece = tp;
    f->mainfocus.pos = pos;
    printf("clicked in main at pos %i\n", pos);
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

struct piece *
insert(struct piece *po, int pos, char *a, size_t an)
{
  struct piece *pn;
  size_t n;
  char *s;

  n = po->n + an;
  s = malloc(sizeof(char) * n);
  if (s == NULL) {
    return NULL;
  }

  memmove(s, po->s, pos);
  memmove(s + pos, a, an);
  memmove(s + pos + an, po->s + pos, po->n - pos);

  pn = piecenew(s, n);
  if (pn == NULL) {
    free(s);
    return NULL;
  }

  pn->next = po->next;
  pn->prev = po->prev;

  return pn;
}

bool
handleactionkeypress(unsigned int code)
{
  struct piece *po, *pn;
  struct tab *f;
  char a[16];
  size_t an;
  int p;

  f = focus->norm.focus;

  po = f->actionfocus.piece;
  p = f->actionfocus.pos;
  
  printf("add char %i\n", code);

  /* TODO: convert code into utf8 sequence. */
  an = 1;
  a[0] = code;
  a[1] = 0;
  
  pn = insert(po, p, a, an);
  if (pn == NULL) {
    return false;
  } else {
    *po->prev = pn;

    f->actionfocus.piece = pn;
    f->actionfocus.pos = p + an;

    panedraw(focus);

    return true;
  }
}

bool
handleactionkeyrelease(unsigned int code)
{
  return false;
}

bool
handlemainkeypress(unsigned int code)
{
  struct piece *po, *pn;
  struct tab *f;
  char a[16];
  size_t an;
  int p;

  f = focus->norm.focus;

  po = f->mainfocus.piece;
  p = f->mainfocus.pos;
  
  printf("add char %i\n", code);

  /* TODO: convert code into utf8 sequence. */
  an = 1;
  a[0] = code;
  a[1] = 0;
  
  pn = insert(po, p, a, an);
  if (pn == NULL) {
    return false;
  } else {
    *po->prev = pn;

    f->mainfocus.piece = pn;
    f->mainfocus.pos = p + an;

    panedraw(focus);

    return true;
  }
}

bool
handlemainkeyrelease(unsigned int code)
{
  return false;
}

bool (*keypresshandlers[NFOCUS_T])(unsigned int) = {
  [FOCUS_action]    = handleactionkeypress,
  [FOCUS_main]      = handlemainkeypress,
};

bool (*keyreleasehandlers[NFOCUS_T])(unsigned int) = {
  [FOCUS_action]    = handleactionkeyrelease,
  [FOCUS_main]      = handlemainkeyrelease,
};

bool
handlekeypress(unsigned int code)
{
  return keypresshandlers[focustype](code);
}

bool
handlekeyrelease(unsigned int code)
{
  return keyreleasehandlers[focustype](code);
}
