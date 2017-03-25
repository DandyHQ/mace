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
  struct tab *f;
  struct line *l;
  int pos;

  focus = p;
  
  f = p->norm.focus;

  l = findpos(f->action,
	      x - p->x - PADDING,
	      y - p->y - lineheight,
	      p->width - PADDING * 2,
	      &pos);

  if (l != NULL) {
    focustype = FOCUS_action;
    
    printf("clicked in actionbar at pos %i\n", pos);
    f->actionfocus.line = l;
    f->actionfocus.pos = pos;
    
  } else {
    focustype = FOCUS_main;

    l = findpos(f->main,
		x - p->x - PADDING,
		y - p->y - lineheight - pos - f->voff,
		p->width - PADDING * 2,
		&pos);

    if (l == NULL) {
      l = f->main;
      while (l->next != NULL)
	l = l->next;

      pos = l->n;
    }

    f->mainfocus.line = l;
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

struct line *
insert(struct line *lo, int p, char *a, size_t an)
{
  struct line *ln;
  size_t n;
  char *s;

  n = lo->n + an;
  s = malloc(sizeof(char) * n);
  if (s == NULL) {
    return NULL;
  }

  memmove(s, lo->s, p);
  memmove(s + p, a, an);
  memmove(s + p + an, lo->s + p, lo->n - p);

  ln = linenew(s, n);
  if (ln == NULL) {
    free(s);
    return NULL;
  }

  ln->next = lo->next;
  ln->prev = lo->prev;

  return ln;
}

bool
handleactionkeypress(unsigned int code)
{
  return false;
}

bool
handleactionkeyrelease(unsigned int code)
{
  return false;
}

bool
handlemainkeypress(unsigned int code)
{
  struct line *lo, *ln;
  struct tab *f;
  char a[16];
  size_t an;
  int p;

  f = focus->norm.focus;

  lo = f->mainfocus.line;
  p = f->mainfocus.pos;
  
  printf("add char %i\n", code);

  /* TODO: convert code into utf8 sequence. */
  an = 1;
  a[0] = code;
  a[1] = 0;
  
  ln = insert(lo, p, a, an);
  if (ln == NULL) {
    return false;
  } else {
    *lo->prev = ln;

    f->mainfocus.line = ln;
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
