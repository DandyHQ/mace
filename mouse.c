#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

typedef enum { LEFT, RIGHT, TOP, BOTTOM, NONE } pos_t;

static pos_t position;
static struct tab *selected;
static struct pane *from;
static int xoff, yoff;

static bool
handlefocuspress(struct pane *p, int x, int y,
	       unsigned int button)
{
  return false;
}

static bool
handlefocusrelease(struct pane *p, int x, int y,
		 unsigned int button)
{
  return false;
}

static bool
handlefocusmotion(struct pane *p, int x, int y)
{
  return false;
}

bool
placeselected(struct pane *p, int x, int y)
{
  if (from != NULL) {
    /* Tab order may have change so change focus. */

    printf("update focus\n");
    from->norm.focus = selected;
    drawpane(from);

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
    case NONE:
      break;
    };

    drawpane(root);
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

  case NONE:
    if (y > p->y + listheight) {
      printf("moved into top\n");
      return TOP;
    }

    break;
  };

  return old;
}

static void
drawhint(struct pane *p, pos_t pos)
{
  struct colour hint = { 0.8, 0.3, 0.1, 0.5 };

  switch (pos) {
  case LEFT:
    drawrect(buf, width, height,
	     p->x, p->y + listheight,
	     p->x + p->width / 2, p->y + p->height,
	     &hint);
    break;

  case RIGHT:
    drawrect(buf, width, height,
	     p->x + p->width / 2, p->y + listheight,
	     p->x + p->width, p->y + p->height,
	     &hint);
    break;

  case TOP:
    drawrect(buf, width, height,
	     p->x, p->y + listheight,
	     p->x + p->width, p->y + p->height / 2,
	     &hint);
    break;

  case BOTTOM:
    drawrect(buf, width, height,
	     p->x, p->y + p->height / 2,
	     p->x + p->width, p->y + p->height,
	     &hint);
    break;

  case NONE:
    printf("pos is none\n");
    break;
  };
}

static bool
moveselected(struct pane *p, int x, int y)
{
  if (from == NULL
      && p->x < x && x < p->x + p->width
      && p->y < y && y < p->y + listheight) {

    /* Put tab into a new pane's list. */

    position = NONE;
    from = p;

    selected->next = from->norm.tabs;
    from->norm.tabs = selected;
  }
  
  if (from != NULL) {
    if (from->x < x && x < from->x + from->width
	&& from->y < y && y < from->y + listheight) {

      /* Rearanging tabs in pane. */

    } else {
      /* Remove tab from pane and maybe remove pane. */
      
      paneremovetab(from, selected);

      if (from->norm.tabs == NULL) {
	paneremove(from);
	resizepane(root, 0, 0, width, height);

	if (p == from) {
	  p = findpane(root, x, y);
	}
      }

      position = TOP;
      from = NULL;
    }
  }

  drawpane(root);

  if (from == NULL) {
    position = updateposition(p, position, x, y);
    drawhint(p, position);
  }

  drawprerender(buf, width, height,
		x - xoff, y - yoff,
		selected->buf, tabwidth, listheight,
		0, 0,
		tabwidth, listheight);
 
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

  position = NONE;
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
  switch (button) {
  case 1:
    return initselected(p, x, y);

  case 4:
    panetablistscroll(p, -5);
    drawtablist(p);
    return true;
    
  case 5:
    panetablistscroll(p, 5);
    drawtablist(p);
    return true;

  default:
    return false;
  }
}

static bool
handletablistrelease(struct pane *p, int x, int y,
		     unsigned int button)
{
  struct tab *t, *tp;

  if (button == 1) {
    t = tabnew("new");
    if (t == NULL) {
      return false;
    }

    for (tp = p->norm.tabs; tp->next != NULL; tp = tp->next)
      ;

    tp->next = t;
    p->norm.focus = t;
  
    drawpane(p);

    return true;
  } else {
    return false;
  }
}

bool
handlebuttonpress(int x, int y, unsigned int button)
{
  struct pane *p;

  p = findpane(root, x, y);
  if (p == NULL) {
    return false;
  }

  if (y < p->y + listheight) {
    return handletablistpress(p, x, y, button);
  } else {
    return handlefocuspress(p, x, y, button);
  }
}

bool
handlebuttonrelease(int x, int y, unsigned int button)
{
  struct pane *p;

  p = findpane(root, x, y);
  if (p == NULL) {
    return false;
  }

  if (selected != NULL) {
    return placeselected(p, x, y);
  } else if (y < p->y + listheight) {
    return handletablistrelease(p, x, y, button);
  } else {
    return handlefocusrelease(p, x, y, button);
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
  } else if (y > p->y + listheight) {
    return handlefocusmotion(p, x, y);
  } else {
    return false;
  } 
}

