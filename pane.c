#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

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
resizepane(struct pane *p, int x, int y, int w, int h)
{
  int r;

  p->x = x;
  p->y = y;
  p->width = w;
  p->height = h;

  switch (p->type) {
  case PANE_norm:
    break;

  case PANE_hsplit:
    r = (int) (p->split.ratio * (float) w);

    resizepane(p->split.a,
	       x, y,
	       r, h);

    resizepane(p->split.b,
	       x + r, y,
	       w - r, h);

    break;

  case PANE_vsplit:
    r = (int) (p->split.ratio * (float) h);

    resizepane(p->split.a,
	       x, y,
	       w, r);
    
    resizepane(p->split.b,
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

void
panetablistscroll(struct pane *p, int s)
{
  struct tab *t;
  int w;

  w = 0;
  for (t = p->norm.tabs; t != NULL; t = t->next)
    w += tabwidth;

  if (p->norm.loff + s > 0) {
    p->norm.loff = 0;
  } else if (p->norm.loff + s < -w) {
    p->norm.loff = -w;
  } else {
    p->norm.loff += s;
  }
}
  
int
panedrawtablist(struct pane *p)
{
  struct tab *t;
  int xo, x, w;

  xo = p->norm.loff;

  for (t = p->norm.tabs; t != NULL && xo < p->width; t = t->next) {
    if (xo + tabwidth > 0) {
      if (xo < 0) {
	x = -xo;
      } else {
	x = 0;
      }

      if (xo + tabwidth < p->width) {
	w = tabwidth;
      } else {
	w = p->width - xo;
      }

      drawprerender(buf, width, height,
		    p->x + xo + x, p->y,
		    t->buf, tabwidth, lineheight,
		    x, 0,
		    w, lineheight);

      if (p->norm.focus == t) {
	drawline(buf, width, height,
		 p->x + xo + x, p->y + lineheight - 1,
		 p->x + xo + w, p->y + lineheight - 1,
		 &bg);
      } else {
	drawline(buf, width, height,
		 p->x + xo + x, p->y + lineheight - 1,
		 p->x + xo + w, p->y + lineheight - 1,
		 &fg);
      }
    }

    xo += tabwidth;
  }

  if (xo > 0 && xo < p->width) {
    drawrect(buf, width, height,
	     p->x + xo, p->y,
	     p->x + p->width, p->y + lineheight - 1,
	     &bg);
    
    drawline(buf, width, height,
	     p->x + xo - 1, p->y,
	     p->x + p->width, p->y,
	     &fg);

    drawline(buf, width, height,
	     p->x + xo - 1, p->y + lineheight - 1,
	     p->x + p->width, p->y + lineheight - 1,
	     &fg);
  }

  drawline(buf, width, height,
	   p->x + p->width, p->y,
	   p->x + p->width, p->y + lineheight,
	   &fg);
 
  drawline(buf, width, height,
	   p->x, p->y,
	   p->x, p->y + lineheight,
	   &fg);

  return lineheight;
}

static void
drawactionoutline(struct pane *p, int y, int yy)
{
  drawline(buf, width, height,
	   p->x + 1, p->y + yy - 1,
	   p->x + p->width - 1, p->y + yy - 1,
	   &fg);

  drawline(buf, width, height,
	   p->x + p->width, p->y + y,
	   p->x + p->width, p->y + yy,
	   &fg);
 
  drawline(buf, width, height,
	   p->x, p->y + y,
	   p->x, p->y + yy,
	   &fg);
}

static void
drawmainoutline(struct pane *p, int y)
{
  drawrect(buf, width, height,
	   p->x, p->y + y,
	   p->x + p->width - 1, p->y + p->height - 1,
	   &bg);

  drawline(buf, width, height,
	   p->x + p->width, p->y + y,
	   p->x + p->width, p->y + p->height - 1,
	   &fg);
 
  drawline(buf, width, height,
	   p->x, p->y + y,
	   p->x, p->y + p->height - 1,
	   &fg);
}


int
panedrawaction(struct pane *p, int y)
{
  int i, a, xx, yy, ww;
  struct line *l;
  struct tab *t;

  t = p->norm.focus;
  yy = y;

  for (l = t->action; l != NULL; l = l->next) {
    xx = 0;
    drawrect(buf, width, height,
	     p->x, p->y + yy,
	     p->x + p->width - 1, p->y + yy + lineheight,
	     &abg);

    for (i = 0; i < l->n; i += a) {
      a = loadglyph(l->s + i);
      if (a == -1) {
	a = 1;
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      if (PADDING + xx + ww >= p->width - PADDING) {
	xx = 0;
	yy += lineheight;

	drawrect(buf, width, height,
		 p->x, p->y + yy,
		 p->x + p->width - 1, p->y + yy + lineheight,
		 &abg);
      }

      drawglyph(buf, width, height,
		p->x + PADDING + xx + face->glyph->bitmap_left,
		p->y + yy + baseline - face->glyph->bitmap_top,
		0, 0,
		face->glyph->bitmap.width, face->glyph->bitmap.rows,
		&fg);

      xx += ww;
    }

    yy += lineheight;
  }

  drawactionoutline(p, y, yy);
  
  return yy;
}

void
panedrawmain(struct pane *p, int y)
{
  int i, a, xx, yy, ww, ty;
  struct line *l;
  struct tab *t;

  drawmainoutline(p, y);

  t = p->norm.focus;
  yy = 0;

  for (l = t->main; l != NULL; l = l->next) {
    xx = 0;

    for (i = 0; i < l->n; i += a) {
      a = loadglyph(l->s + i);
      if (a == -1) {
	a = 1;
	continue;
      } 

      ww = face->glyph->advance.x >> 6;

      if (PADDING + xx + ww >= p->width - PADDING) {
	xx = 0;
	yy += lineheight;
      }

      if (y + yy - t->voff >= p->height) {
	return;
      }
      
      if (yy + lineheight < t->voff) {
	xx += ww;
	continue;
      } else if (yy < t->voff) {
	ty = t->voff - yy;
	yy = t->voff;
      } else {
	ty = 0;
      }

      drawglyph(buf, width, height,
		p->x + PADDING + xx + face->glyph->bitmap_left,
		p->y + y + yy - t->voff
		+ baseline - face->glyph->bitmap_top,
		0, ty,
		face->glyph->bitmap.width,
		face->glyph->bitmap.rows - ty,
		&fg);

      xx += ww;
    }

    yy += lineheight;
  }
}

void
panedraw(struct pane *p)
{
  int y;
  
  switch (p->type) {
  case PANE_norm:
    y = panedrawtablist(p);
    y = panedrawaction(p, y);
    panedrawmain(p, y);
    break;

  case PANE_hsplit:
  case PANE_vsplit:
    panedraw(p->split.a);
    panedraw(p->split.b);
    break;
  }
}
