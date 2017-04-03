#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

typedef enum { LEFT, RIGHT, TOP, BOTTOM, LIST } pos_t;

static pos_t position;
static struct tab *selected;
static struct pane *from;
static int xoff, yoff;

bool
placeselected(struct pane *p, int x, int y)
{
  if (from != NULL) {
    /* Tab order may have change so change focus. */

    from->norm.focus = selected;
    panedraw(from);

  } else {
    switch (position) {
    case LEFT:
      panesplit(p, selected, PANE_hsplit, true);
      break;
    case RIGHT:
      panesplit(p, selected, PANE_hsplit, false);
      break;
    case TOP:
      panesplit(p, selected, PANE_vsplit, true);
      break;
    case BOTTOM:
      panesplit(p, selected, PANE_vsplit, false);
      break;
    case LIST:
      break;
    };

    panedraw(root);
  }
    
  selected = NULL;

  return true;
}

static pos_t
updateposition(struct pane *p, pos_t old, int x, int y)
{
  /* TODO: add some sort of hysterias so it doesn't alternate between
     two states when it is on a borderline. */
  
  switch (old) {
  case LEFT:
    if (x > p->x + p->width / 3) {
      return RIGHT;
    } else if (y > p->y + p->height / 3 * 2) {
      return BOTTOM;
    } else if (y < p->y + p->height / 3) {
      return TOP;
    }

    break;

  case RIGHT:
    if (x < p->x + p->width / 3) {
      return LEFT;
    } else if (y > p->y + p->height / 3 * 2) {
      return BOTTOM;
    } else if (y < p->y + p->height / 3) {
      return TOP;
    }

    break;

  case TOP:
    if (y > p->y + p->height / 3 * 2) {
      return BOTTOM;
    } else if (x > p->x + p->width / 3 * 2) {
      return RIGHT;
    } else if (x < p->x + p->width / 3) {
      return LEFT;
    }

    break;

  case BOTTOM:
    if (y < p->y + p->height / 3) {
      return TOP;
    } else if (x > p->x + p->width / 3 * 2) {
      return RIGHT;
    } else if (x < p->x + p->width / 3) {
      return LEFT;
    }

    break;

  case LIST:
    if (y > p->y + lineheight) {
      return TOP;
    }

    break;
  };

  return old;
}

static void
inserttab(struct pane *p, struct tab *t, int x)
{
  struct tab **tt;
  int xx;

  xx = p->x + p->norm.loff;
  tt = &p->norm.tabs;
  while (*tt != NULL && !(xx < x && x < xx + tabwidth)) {
    tt = &(*tt)->next;
    xx += tabwidth;
  }
    
  t->next = *tt;
  *tt = t;

  p->norm.focus = t;
}

static bool
moveselected(struct pane *p, int x, int y)
{
  if (from == NULL
      && p->x < x && x < p->x + p->width
      && p->y < y && y < p->y + lineheight) {

    /* Put tab into a new pane's list. */

    position = LIST;
    from = p;
    inserttab(p, selected, x);
  }
  
  if (from != NULL) {
    if (from->x < x && x < from->x + from->width
	&& from->y < y && y < from->y + lineheight) {

      paneremovetab(from, selected);
      inserttab(from, selected, x);

    } else {
      paneremovetab(from, selected);

      if (from->norm.tabs == NULL) {
	paneremove(from);
	paneresize(root, 0, 0, width, height);

	if (p == from) {
	  p = findpane(root, x, y);
	}
      }

      position = TOP;
      from = NULL;
    }
  }

  panedraw(root);

  if (from == NULL) {
    position = updateposition(p, position, x, y);

    drawrect(buf, width, height,
	     x - 5, y - 5,
	     x + 5, y + 5,
	     &fg);
  }
 
  return true;
}

static bool
initselected(struct pane *p, int x, int y)
{
  struct tab *t;
  int xx;

  xx = p->x + p->norm.loff;
  for (t = p->norm.tabs; t != NULL; t = t->next) {
    if (xx < x && x < xx + tabwidth) {
      break;
    } else {
      xx += tabwidth;
    }
  } 

  if (t == NULL) {
    return false;
  }

  position = LIST;
  from = p;
  selected = t;
  xoff = x - xx;
  yoff = y - p->y;

  return true;
}

static bool
handletablistpress(struct pane *p, int x, int y,
		   unsigned int button)
{
  struct tab *t, *tp;
  
  switch (button) {
  case 1:
    return initselected(p, x, y);

  case 2:
    t = tabnew((uint8_t *) "new");
    if (t == NULL) {
      return false;
    }

    for (tp = p->norm.tabs; tp->next != NULL; tp = tp->next)
      ;

    tp->next = t;
    p->norm.focus = t;
    panedraw(p);
    
    return true;

  default:
    return false;
  }
}

bool
handlebuttonpress(int x, int y, int button)
{
  struct pane *p;

  p = findpane(root, x, y);
  if (p == NULL) {
    return false;
  }

  if (y < p->y + lineheight) {
    return handletablistpress(p, x, y, button);
  } else {
    return handlepanepress(p, x, y, button);
  }
}

bool
handlebuttonrelease(int x, int y, int button)
{
  struct pane *p;

  switch (button) {
  case 1:
    p = findpane(root, x, y);
    if (p == NULL) {
      return false;
    }

    if (selected != NULL) {
      return placeselected(p, x, y);
    } else if (y >= p->y + lineheight) {
      return handlepanerelease(p, x, y, button);
    } else {
      return false;
    }

  case 2:
  case 3:
    return false;

  default:
    return false;
  }
}

bool
handlescroll(int x, int y, int dx, int dy)
{
  struct pane *p;

  p = findpane(root, x, y);
  if (p == NULL) {
    return false;
  }

  if (y < p->y + lineheight) {
    if (handlepanetablistscroll(p, x, y, dx, dy)) {
      panedrawtablist(p);
      return true;
    } else {
      return false;
    }
  } else {
    if (handlepanescroll(p, x, y, dx, dy)) {
      panedraw(p);
      return true;
    } else {
      return false;
    }
  }
}

bool
handlemotion(int x, int y)
{
  struct pane *p;

  p = findpane(root, x, y);
  if (p == NULL) {
    return false;
  }

  if (selected != NULL) {
    return moveselected(p, x, y);
  } else if (y > p->y + lineheight) {
    return handlepanemotion(p, x, y);
  } else {
    return false;
  } 
}

