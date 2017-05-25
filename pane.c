#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

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
  
  luaremove(p->mace->lua, p);

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

static struct colour fg = { 0, 0, 0 };
static struct colour nbg = { 0.8, 0.8, 0.8 };
static struct colour fbg = { 1, 1, 1 };

/* This is shit but will do for now. This will not work once we get
   multiple panes. */

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
  tabdraw(p->focus, cr);
}
