#include "mace.h"

/* TODO: 

   Improve speed. Stop textboxcalcpositions from calculating 
   things it doesnt need to. eg: just push pieces down rather than 
   recalculating their width. 

   Reimpliment something like textboxfindstart to stop drawing
   unnessesary glyphs.
*/


static struct colour nfg = { 0, 0, 0 };
static struct colour sbg = { 0.5, 0.8, 0.7 };

static void
drawcursor(struct textbox *t, int x, int y, int h)
{
  cairo_set_source_rgb(t->cr, 0, 0, 0);
  cairo_set_line_width (t->cr, 1.0);

  cairo_move_to(t->cr, x, y);
  cairo_line_to(t->cr, x, y + h);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y);
  cairo_line_to(t->cr, x + 2, y);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y + h);
  cairo_line_to(t->cr, x + 2, y + h);
  cairo_stroke(t->cr);
}

static void
drawglyphs(struct textbox *t, cairo_glyph_t *glyphs, size_t len,
	   struct colour *fg, struct colour *bg,
	   int ay, int by)
{
  cairo_text_extents_t extents;
  size_t g, start;

  cairo_set_source_rgb(t->cr, bg->r, bg->g, bg->b);

  /* Go through glyphs and find lines, draw background. */
  start = 0;
  for (g = 0; g < len; g++) {
    if (glyphs[g].x != 0) {
      continue;
    }

    cairo_rectangle(t->cr,
		  glyphs[start].x,
		  glyphs[start].y - ay,
		  t->linewidth - glyphs[start].x,
		  ay + by);
		  
    cairo_fill(t->cr);

    start = g;
  }

  /* Draw background of remaining glyphs. */

  if (start < g) {
    cairo_glyph_extents(t->cr, &glyphs[start], g - start, &extents);

    cairo_rectangle(t->cr,
		  glyphs[start].x,
		  glyphs[start].y - ay,
		  extents.x_advance,
		  ay + by);
		  
    cairo_fill(t->cr);
  }

  /* Draw glyphs */
  cairo_set_source_rgb(t->cr, fg->r, fg->g, fg->b);
  cairo_show_glyphs(t->cr, glyphs, len);
}

void
textboxpredraw(struct textbox *t)
{
  struct selection *sel, *nsel;
  cairo_text_extents_t extents;
  struct sequence *s;
  size_t i, g, start;
  struct colour *bg;
  struct piece *p;
  int32_t code, a;
  int ay, by;

  cairo_set_source_rgb(t->cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(t->cr);

  cairo_set_font_face(t->cr, t->font->cface);
  cairo_set_font_size(t->cr, t->font->size);

  cairo_translate(t->cr, 0, -t->yoff);

  ay = (t->font->face->size->metrics.ascender >> 6) + 1;
  by = -(t->font->face->size->metrics.descender >> 6) + 1;
	 
  s = t->sequence;
  p = &s->pieces[SEQ_start];
  i = 0;
  g = 0;
  start = 0;

  sel = NULL;
  bg = &t->bg;

  while (true) {
    while (i < p->len && g < p->nglyphs
	   && p->glyphs[g].y < t->yoff + t->maxheight) {

      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
	i++;
	continue;
      }

      nsel = inselections(t, p->pos + i);
      if (nsel != sel) {
	if (start < g) {
	  drawglyphs(t, &p->glyphs[start], g - start,
		     &nfg, bg, ay, by);
	}

	start = g;

	sel = nsel;
	if (sel != NULL) {
	  bg = &sbg;
	} else {
	  bg = &t->bg;
	}
      }

      /* Line Break. */
      if (p->glyphs[g].index == 0) {
	if (start < g) {
	  drawglyphs(t, &p->glyphs[start], g - start,
		     &nfg, bg, ay, by);
	}

	if (sel != NULL) {
	  cairo_set_source_rgb(t->cr, bg->r, bg->g, bg->b);
	  cairo_rectangle(t->cr,
			  p->glyphs[g].x,
			  p->glyphs[g].y - ay,
			  t->linewidth - p->glyphs[g].x,
			  ay + by);
		  
	  cairo_fill(t->cr); 
	}
	
	if (p->pos + i == t->cursor) {
	  drawcursor(t, p->glyphs[g].x, p->glyphs[g].y - ay,
		     ay + by);
	}

	/* Update's a if need be. */
	islinebreak(code, s->data + p->off + i, p->len - i, &a);

	start = g + 1;

      } else if (p->pos + i == t->cursor) {
	drawglyphs(t, &p->glyphs[start], g - start + 1,
		   &nfg, bg, ay, by);

	drawcursor(t, p->glyphs[g].x, p->glyphs[g].y - ay,
		   ay + by);

	start = g + 1;
      }

      i += a;
      g++;
    }

    if (start < g) {
      drawglyphs(t, &p->glyphs[start], g - start,
		 &nfg, bg, ay, by);
    }

    /* Kinda hacky. Draws the cursor if it is one past the end. */
    if (p->pos + i + 1 == t->cursor) {
      if (g > 0 && g - 1 < p->nglyphs) {
	cairo_glyph_extents(t->cr, &p->glyphs[g-1], 1, &extents);

	drawcursor(t, p->glyphs[g-1].x + extents.x_advance,
		   p->glyphs[g-1].y - ay,
		   ay + by);
      }
    }

    if (g < p->nglyphs && p->glyphs[g].y >= t->yoff + t->maxheight) {
      break;
    } else if (p->next != SEQ_end) {
      p = &s->pieces[p->next];
      i = 0;
      start = 0;
      g = 0;
    } else {
      break;
    }
  }

  cairo_translate(t->cr, 0, t->yoff);
} 

size_t
textboxfindpos(struct textbox *t, int lx, int ly)
{
  struct sequence *s;
  struct piece *p;
  int32_t code, a;
  int ww, ay, by;
  size_t i, g;

  ly += t->yoff;
  
  ay = (t->font->face->size->metrics.ascender >> 6) + 1;
  by = -(t->font->face->size->metrics.descender >> 6) + 1;
	 
  s = t->sequence;
  p = &s->pieces[SEQ_start];
  i = 0;
  g = 0;

  while (true) {
    while (i < p->len && g < p->nglyphs) {
      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
	i++;
	continue;
      }

      /* Line Break. */
      if (p->glyphs[g].index == 0) {
	/* Update's a if need be. */
	islinebreak(code, s->data + p->off + i, p->len - i, &a);

	if (p->glyphs[g].y - ay <= ly && ly <= p->glyphs[g].y + by) {
	  return p->pos + i;
	} else {
	  i += a;
	  g++;
	  continue;
	}
      }

      if (FT_Load_Glyph(t->font->face, p->glyphs[g].index,
			FT_LOAD_DEFAULT) != 0) {
	i += a;
	continue;
      }

      ww = t->font->face->glyph->advance.x >> 6;

      if (p->glyphs[g].y - ay <= ly && ly <= p->glyphs[g].y + by) {
	if (p->glyphs[g].x <= lx && lx <= p->glyphs[g].x + ww) {
	  return p->pos + i;
	}
      }

      i += a;
      g++;
    }

    if (p->next != -1) {
      p = &s->pieces[p->next];
      i = 0;
      g = 0;
    } else {
      break;
    }
  }

  return p->pos + i;
}

void
textboxcalcpositions(struct textbox *t, size_t pos)
{
  struct sequence *s;
  struct piece *p;
  int32_t code, a;
  int x, y, ww;
  size_t i, g;
  
  s = t->sequence;
  p = &s->pieces[SEQ_start];
  i = 0;
  g = 0;

  x = 0;
  y = t->font->baseline;
  
  while (true) {
    while (i < p->len && g < p->nglyphs) {
      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
	i++;
	continue;
      }

      /* Line Break. */
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

	p->glyphs[g].index = 0;
	p->glyphs[g].x = x;
	p->glyphs[g].y = y;

	x = 0;
	y += t->font->lineheight;
	i += a;
	g++;

	continue;
      }

      p->glyphs[g].index = FT_Get_Char_Index(t->font->face, code);
      if (p->glyphs[g].index == 0) {
	i += a;
	continue;
      }

      if (FT_Load_Glyph(t->font->face, p->glyphs[g].index,
			FT_LOAD_DEFAULT) != 0) {
	i += a;
	continue;
      }

      ww = t->font->face->glyph->advance.x >> 6;

      /* Wrap Line. */
      if (x + ww >= t->linewidth) {
	x = 0;
	y += t->font->lineheight;
      }

      p->glyphs[g].x = x;
      p->glyphs[g].y = y;

      x += ww;
      i += a;
      g++;
    }

    if (g != p->nglyphs) {
      p->nglyphs = g;
    }

    if (p->next != -1) {
      p = &s->pieces[p->next];
      i = 0;
      g = 0;
    } else {
      break;
    }
  }

  t->height = y - (t->font->face->size->metrics.descender >> 6);
}

