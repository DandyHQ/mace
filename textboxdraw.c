#include "mace.h"

static struct colour nfg = { 0, 0, 0 };
static struct colour sbg = { 0.5, 0.8, 0.7 };

/* 

   Here be dragons. 
   Lots of fucking dragons.
   This entire file is horrible and hacky. 
   It needs to be done from scratch intelligently.
   Possibly utilizing pango or harfbuzz.

*/

static void
drawcursor(struct textbox *t, int x, int y)
{
  cairo_set_source_rgb(t->cr, 0, 0, 0);
  cairo_set_line_width (t->cr, 1.0);

  cairo_move_to(t->cr, x, y);
  cairo_line_to(t->cr, x, y + t->font->lineheight - 1);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y);
  cairo_line_to(t->cr, x + 2, y);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y + t->font->lineheight - 1);
  cairo_line_to(t->cr, x + 2, y + t->font->lineheight - 1);
  cairo_stroke(t->cr);
}

void
textboxpredraw(struct textbox *t)
{
  struct selection *sel;
  struct sequence *s;
  struct colour *bg;
  struct piece *p;
  int32_t code, a;
  int x, y, ww;
  size_t i;

  s = t->sequence;

  cairo_set_source_rgb(t->cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(t->cr);

  p = &s->pieces[t->startpiece];
  i = t->startindex;
  x = t->startx;
  y = t->starty;

  while (true) {
    while (i < p->len) {
      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
	i++;
	continue;
      }

      sel = inselections(t, p->pos + i);
      
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

	if (sel != NULL) {
	  cairo_set_source_rgb(t->cr, sbg.r, sbg.g, sbg.b);

	  cairo_rectangle(t->cr, x, y - t->yoff,
			  t->linewidth - x,
			  t->font->lineheight);

	  cairo_fill(t->cr);
	}
	    
	if (p->pos + i == t->cursor) {
	  drawcursor(t, x, y - t->yoff);
	}

	x = 0;
	y += t->font->lineheight;

	if (y - t->yoff >= t->maxheight) {
	  return;
	}
      }

      if (!loadglyph(t->font->face, code)) {
	i += a;
	continue;
      }

      ww = t->font->face->glyph->advance.x >> 6;

      if (x + ww >= t->linewidth) {
	if (sel != NULL) {
	  cairo_set_source_rgb(t->cr, sbg.r, sbg.g, sbg.b);

	  cairo_rectangle(t->cr, x, y - t->yoff,
			  t->linewidth - x,
			  t->font->lineheight);

	  cairo_fill(t->cr);
	}

	x = 0;
	y += t->font->lineheight;

	if (y - t->yoff >= t->maxheight) {
	  return;
	}
      }

      if (sel != NULL) {
	bg = &sbg;
      } else {
	bg = &t->bg;
      }

      drawglyph(t->font, t->cr, x, y - t->yoff, &nfg, bg);

      if (p->pos + i == t->cursor) {
	drawcursor(t, x, y - t->yoff);
      }

      i += a;
      x += ww;
    }

    if (p->next == -1) {
      break;
    } else {
      p = &s->pieces[p->next];
      x = p->x;
      y = p->y;
      i = 0;
    }
  }

  sel = inselections(t, p->pos);
  if (sel != NULL) {
    cairo_set_source_rgb(t->cr, sbg.r, sbg.g, sbg.b);

    cairo_rectangle(t->cr, x, y - t->yoff,
		    t->linewidth - x,
		    t->font->lineheight);

    cairo_fill(t->cr);
  }
  
  if (p->pos + i == t->cursor) {
    drawcursor(t, x, y - t->yoff);
  }
} 

size_t
textboxfindpos(struct textbox *t, int lx, int ly)
{
  struct sequence *s;
  int32_t code, a;
  struct piece *p;
  int x, y, ww;
  size_t i;

  ly += t->yoff;

  s = t->sequence;
  p = &s->pieces[t->startpiece];
  i = t->startindex;
  x = t->startx;
  y = t->starty;

  while (true) {
    while (i < p->len) {
      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
	i++;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

	if (ly < y + t->font->lineheight) {
	  return p->pos + i;
	} else {
	  x = 0;
	  y += t->font->lineheight;
	}
      }
     
      if (!loadglyph(t->font->face, code)) {
	i += a;
	continue;
      }

      ww = t->font->face->glyph->advance.x >> 6;

      /* Wrap Line. */
      if (x + ww >= t->linewidth) {
	if (ly < y + t->font->lineheight) {
	  return p->pos + i;
	} else {
	  x = 0;
	  y += t->font->lineheight;
	}
      }

      x += ww;

      if (ly < y + t->font->lineheight && lx < x) {
	return p->pos + i;
      }
	
      i += a;
    }

    if (p->next == -1) {
      break;
    }
  
    p = &s->pieces[p->next];
    i = 0;
  }
  return p->pos + i;
}

/* TODO: improve speed. Stop it from calculating things it doesnt need
   to. eg: just push pieces down rather than recalculating their
   width. */

void
textboxcalcpositions(struct textbox *t, size_t pos)
{
  struct sequence *s;
  struct piece *p;
  int32_t code, a;
  int x, y, ww;
  ssize_t pi;
  size_t i;
  
  s = t->sequence;

  pi = SEQ_start;
  i = 0;
  p = &s->pieces[pi];

  /*
  pi = sequencefindpiece(s, pos, &i);
  if (pi == -1) {
    return;
  }
  
  p = &s->pieces[pi];
  if (p->prev != -1) {
    p = &s->pieces[p->prev];
  }
  */
  
  x = p->x;
  y = p->y;
  
  while (true) {
    p->width = 0;

    i = 0;
    while (i < p->len) {
      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
	i++;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

	if (p->width == 0) {
	  p->width = t->linewidth - p->x;
	} else {
	  p->width += t->linewidth;
	}
	
	x = 0;
	y += t->font->lineheight;
      }
     
      if (!loadglyph(t->font->face, code)) {
	i += a;
	continue;
      }

      ww = t->font->face->glyph->advance.x >> 6;

      /* Wrap Line. */
      if (x + ww >= t->linewidth) {
	if (p->width == 0) {
	  p->width = t->linewidth - p->x;
	} else {
	  p->width += t->linewidth;
	}

	x = 0;
	y += t->font->lineheight;
      }

      x += ww;
      i += a;
    }

    if (p->width == 0) {
      p->width = x - p->x;
    } else {
      p->width += x;
    }

    if (p->next == -1) {
      break;
    } else {
      p = &s->pieces[p->next];
      p->x = x;
      p->y = y;
    }
  }

  t->height = y + t->font->lineheight;
}

void
textboxfindstart(struct textbox *t)
{
  struct sequence *s;
  struct piece *p;
  int32_t code, a;
  int x, y, ww;
  ssize_t pi;
  size_t i;

  s = t->sequence;

  pi = SEQ_start;
  p = &s->pieces[pi];
  
  /*
    This optimasation is not complete. It only works if you are scrolling
    up. 

    pi = t->startpiece;
    p = &s->pieces[pi];

    while (p->prev != -1) {
    if (p->y + t->font->lineheight < t->yoff) {
    break;
    }

    pi = p->prev;
    p = &s->pieces[pi];
    }
  */
    
  while (true) {
    i = 0;
    x = p->x;
    y = p->y;

    if (y + t->font->lineheight >= t->yoff) {
      goto done;
    }
    
    while (i < p->len) {
      a = utf8iterate(s->data + p->off + i,
			   p->len - i, &code);
      if (a <= 0) {
	i++;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

	x = 0;
	y += t->font->lineheight;

	if (y + t->font->lineheight >= t->yoff) {
	  i += a;
	  goto done;
	}
      }
     
      if (!loadglyph(t->font->face, code)) {
	i += a;
	continue;
      }

      ww = t->font->face->glyph->advance.x >> 6;

      /* Wrap Line. */
      if (x + ww >= t->linewidth) {
	x = 0;
	y += t->font->lineheight;

	if (y + t->font->lineheight >= t->yoff) {
	  goto done;
	}
      }

      x += ww;
      i += a;
    }

    if (p->next == -1) {
      goto done;
    } else {
      pi = p->next;
      p = &s->pieces[pi];
    }
  }

 done:
  t->startpiece = pi;
  t->startindex = i;
  t->startx = x;
  t->starty = y;
}

