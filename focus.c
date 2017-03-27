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
    f->acursor = pos;
    
  } else {
    focustype = FOCUS_main;

    tp = findpos(f->main,
		x - p->x - PADDING,
		y - p->y - lineheight - pos - f->voff,
		p->width - PADDING * 2,
		&pos);

    if (tp == NULL) {
      pos = 0;
      for (tp = f->main; tp->next != NULL; tp = tp->next) {
	pos += tp->pl;
      }
    }

    f->mcursor = pos;
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

bool
handleactiontyping(uint8_t *s, size_t n)
{
  return false;
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
handlemaintyping(uint8_t *s, size_t n)
{
  printf("typed %i : %s into main\n", n, s);
  return false;
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
