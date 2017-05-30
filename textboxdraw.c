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

  cairo_move_to(t->cr, x, y + 1);
  cairo_line_to(t->cr, x, y + h - 2);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y + 1);
  cairo_line_to(t->cr, x + 2, y + 1);
  cairo_stroke(t->cr);

  cairo_move_to(t->cr, x - 2, y + h - 2);
  cairo_line_to(t->cr, x + 2, y + h - 2);
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
  for (g = 1; g < len; g++) {
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

  ay = (t->font->face->size->metrics.ascender >> 6);
  by = -(t->font->face->size->metrics.descender >> 6);
	 
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
      if (islinebreak(code, s->data + p->off + i, p->len - i, &a)) {
	if (start < g) {
	  drawglyphs(t, &p->glyphs[start], g - start,
		     &nfg, bg, ay, by);
	}

	start = g + 1;

	if (sel != NULL) {
	  cairo_set_source_rgb(t->cr, bg->r, bg->g, bg->b);
	  cairo_rectangle(t->cr,
			  p->glyphs[g].x,
			  p->glyphs[g].y - ay,
			  t->linewidth - p->glyphs[g].x,
			  ay + by);
		  
	  cairo_fill(t->cr); 
	}
	
      } else if (code == '\t') {
	if (start < g) {
	  drawglyphs(t, &p->glyphs[start], g - start,
		     &nfg, bg, ay, by);
	}

	start = g + 1;

	if (sel != NULL) {
	  cairo_set_source_rgb(t->cr, bg->r, bg->g, bg->b);
	  cairo_rectangle(t->cr,
			  p->glyphs[g].x,
			  p->glyphs[g].y - ay,
			  t->font->tabwidthpixels,
			  ay + by);
		  
	  cairo_fill(t->cr); 
	}
      }

      if (p->pos + i == t->cursor) {
	if (start <= g) {
	  drawglyphs(t, &p->glyphs[start], g - start + 1,
		     &nfg, bg, ay, by);
	}

	start = g + 1;

	drawcursor(t,
		   p->glyphs[g].x,
		   p->glyphs[g].y - ay,
		   ay + by);
      }

      i += a;
      g++;
    }

    if (start < g) {
      drawglyphs(t, &p->glyphs[start], g - start,
		 &nfg, bg, ay, by);
    }

    if (g < p->nglyphs && p->glyphs[g].y >= t->yoff + t->maxheight) {
      break;
    } else if (p->next != -1) {
      p = &s->pieces[p->next];
      i = 0;
      start = 0;
      g = 0;
    } else {
      break;
    }
  }

  if (t->cursor == sequencegetlen(s)) {
    p = &s->pieces[SEQ_end];
    drawcursor(t,
	       p->glyphs[0].x,
	       p->glyphs[0].y - ay,
	       ay + by);
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
  
  ay = (t->font->face->size->metrics.ascender >> 6);
  by = -(t->font->face->size->metrics.descender >> 6);
	 
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

      if (islinebreak(code, s->data + p->off + i, p->len - i, &a)) {
	if (p->glyphs[g].y - ay <= ly &&
	    ly <= p->glyphs[g].y + by) {

	  return p->pos + i;
	}

      } else if (code == '\t') {
	if (p->glyphs[g].y - ay <= ly &&
	    ly <= p->glyphs[g].y + by &&
	    p->glyphs[g].x <= lx &&
	    lx <= p->glyphs[g].x + t->font->tabwidthpixels) {

	  return p->pos + i;
	}
      } else {
	if (FT_Load_Glyph(t->font->face, p->glyphs[g].index,
			  FT_LOAD_DEFAULT) != 0) {
	  i += a;
	  continue;
	}

	ww = t->font->face->glyph->advance.x >> 6;

	if (p->glyphs[g].y - ay <= ly &&
	    ly <= p->glyphs[g].y + by &&
	    p->glyphs[g].x <= lx &&
	    lx <= p->glyphs[g].x + ww) {

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

  return sequencegetlen(s);
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
      
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

	p->glyphs[g].index = 0;
	p->glyphs[g].x = x;
	p->glyphs[g].y = y;

	x = 0;
	y += t->font->lineheight;

      } else if (code == '\t') {
	if (x + t->font->tabwidthpixels >= t->linewidth) {
	  x = 0;
	  y += t->font->lineheight;
	}

	p->glyphs[g].index = 0;
	p->glyphs[g].x = x; 
	p->glyphs[g].y = y;

	x += t->font->tabwidthpixels;

      } else {

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
      }
      
      i += a;
      g++;
    }

    if (p->next != -1) {
      p->nglyphs = g;
      p = &s->pieces[p->next];
      i = 0;
      g = 0;

    } else {
      /* End which has one glyph. */
      p->glyphs[0].index = 0;
      p->glyphs[0].x = x;
      p->glyphs[0].y = y;
      
      break;
    }
  }

  t->height = y - (t->font->face->size->metrics.descender >> 6);
}

