#include "mace.h"

struct pane *
panenew(struct mace *mace)
{
  struct pane *p;
  
  p = calloc(1, sizeof(struct pane));
  if (p == NULL) {
    return NULL;
  }

  p->mace = mace;

  return p;
} 

void
panefree(struct pane *p)
{
  struct tab *t, *n;
  
  t = p->tabs;
  while (t != NULL) {
    n = t->next;
    tabfree(t);
    t = n;
  }
  
  free(p);
}

bool
paneresize(struct pane *p, int x, int y, int w, int h)
{
  struct tab *t;

  p->x = x;
  p->y = y;
  p->width = w;
  p->height = h;

  for (t = p->tabs; t != NULL; t = t->next) {
    if (!tabresize(t, x, y + p->mace->font->lineheight,
		   w, h - p->mace->font->lineheight)) {

      return false;
    }
  }

  return true;
}

void
paneaddtab(struct pane *p, struct tab *t, int pos)
{
  struct tab *prev;

  if (t->pane != NULL) {
    paneremovetab(t->pane, t);
  }
  
  if (pos == 0 || p->tabs == NULL) {
    t->next = p->tabs;
    p->tabs = t;
    goto clean;
  }
  
  for (prev = p->tabs; prev->next != NULL; prev = prev->next) {
    if (pos == 1) {
      break;
   } else {
      pos--;
    }
  }

  t->next = prev->next;
  prev->next = t;

 clean:
  t->pane = p;

  if (!tabresize(t, p->x, p->y + p->mace->font->lineheight,
		 p->width, p->height - p->mace->font->lineheight)) {
    fprintf(stderr, "Failed to resize tab to fit in pane!\n");
    /* What should happen here? */
  }
}

void
paneremovetab(struct pane *p, struct tab *t)
{
  struct tab *prev;
 
  if (p->tabs == t) {
    p->tabs = t->next;
    goto clean;
  }
  
  for (prev = p->tabs; prev->next != NULL; prev = prev->next) {
    if (prev->next == t) {
      prev->next = t->next;
      goto clean;
    }
  }

  return;
  
 clean:
  t->next = NULL;
  t->pane = NULL;

  if (p->focus == t) {
    /* This could be improved */
    p->focus = p->tabs;

    if (p->mace->mousefocus == t->action || p->mace->keyfocus == t->main) {
      if (p->focus != NULL) {
				p->mace->mousefocus = p->mace->keyfocus = p->focus->main;
      } else {
				p->mace->mousefocus = p->mace->keyfocus = NULL;
      }
    }
  }
}

/* The tab list stuff is shit but will do for now. It will need to be
   redone once we have multiple panes. */

bool
tablistbuttonpress(struct pane *p, int px, int py, int button)
{
  struct tab *t;
  int32_t code;
  size_t i, a;
  int x, ww;

  x = 0;
  for (t = p->tabs; t != NULL; t = t->next) {
    x += 5;
    
    i = 0;
    while (i < t->nlen) {
      a = utf8iterate(t->name + i, t->nlen - i, &code);
      if (a == 0) {
				i++;
				continue;
      }

      if (!loadglyph(p->mace->font->face, code)) {
				i += a;
				continue;
      }

      ww = p->mace->font->face->glyph->advance.x >> 6;

      x += ww;
      if (px < x) {
				if (p->focus != t) {
					p->focus = t;
					p->mace->mousefocus = NULL;
					p->mace->keyfocus = t->main;
					return true;
				} else {
					return false;
				}
      }

      i += a;
    }

    x += 5;

  }

  return false;
}

bool
tablistscroll(struct pane *p, int x, int y, int dx, int dy)
{
  return false;
}

static struct colour fg = { 0, 0, 0 };
static struct colour nbg = { 0.8, 0.8, 0.8 };
static struct colour fbg = { 1, 1, 1 };

void
tablistdraw(struct pane *p, cairo_t *cr)
{
  struct colour *bg;
  struct tab *t;
  int32_t code;
  size_t i, a;
  int x, ww;

  x = 0;
  for (t = p->tabs; t != NULL; t = t->next) {
    if (t == p->focus) {
      bg = &fbg;
    } else {
      bg = &nbg;
    }

    cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);
    cairo_rectangle(cr, p->x + x, p->y,
		    p->x + 5, p->y + p->mace->font->lineheight);
    cairo_fill(cr);

    x += 5;
    
    i = 0;
    while (i < t->nlen) {
      a = utf8iterate(t->name + i, t->nlen - i, &code);
      if (a == 0) {
	i++;
	continue;
      }

      if (!loadglyph(p->mace->font->face, code)) {
	i += a;
	continue;
      }

      ww = p->mace->font->face->glyph->advance.x >> 6;

      if (x + ww >= p->width) {
	return;
      }

      drawglyph(p->mace->font, cr, p->x + x, p->y, &fg, bg);
      
      i += a;
      x += ww;
    }

    cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);
    cairo_rectangle(cr, p->x + x, p->y,
		    p->x + 5, p->y + p->mace->font->lineheight);
    cairo_fill(cr);

    x += 5;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width (cr, 1.0);

    cairo_move_to(cr, p->x + x, p->y);
    cairo_line_to(cr, p->x + x, p->y + p->mace->font->lineheight);
    cairo_stroke(cr);
  }

  cairo_set_source_rgb(cr, nbg.r, nbg.g, nbg.b);
  cairo_rectangle(cr, p->x + x, p->y,
		  p->x + p->width,
		  p->y + p->mace->font->lineheight);
  cairo_fill(cr);
}

void
panedraw(struct pane *p, cairo_t *cr)
{
  tablistdraw(p, cr);

  if (p->focus != NULL) {
    tabdraw(p->focus, cr);
  } else {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, p->x, p->y + p->mace->font->lineheight,
		    p->x + p->width,
		    p->y + p->height);
    cairo_fill(cr);
  }
}
