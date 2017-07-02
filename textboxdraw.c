#include "mace.h"

/* TODO: 
   I don't like how tabs or newlines are handled. It would possibly 
   be better to have each piece have another array of extra 
   information about the glyphs containing width, maybe height, 
   maybe colour. This way all glyphs could be handled in a uniform
   way.
*/

static struct colour nfg = { 0, 0, 0 };
static struct colour sbg = { 0.5, 0.8, 0.7 };
static struct colour cbg = { 0.9, 0.5, 0.2 };

static void
drawcursor(struct textbox *t, cairo_t *cr, int x, int y, int h)
{
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width (cr, 1.3);

  cairo_move_to(cr, x, y + 1);
  cairo_line_to(cr, x, y + h - 2);
  cairo_stroke(cr);

  cairo_move_to(cr, x - 2, y + 1);
  cairo_line_to(cr, x + 2, y + 1);
  cairo_stroke(cr);

  cairo_move_to(cr, x - 2, y + h - 2);
  cairo_line_to(cr, x + 2, y + h - 2);
  cairo_stroke(cr);
}

static void
drawglyphs(struct textbox *t, cairo_t *cr, 
     cairo_glyph_t *glyphs, size_t len,
	   struct colour *fg, struct colour *bg,
	   int ay, int by)
{
  cairo_text_extents_t extents;
  size_t g, start;

  cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);

  /* Go through glyphs and find lines, draw background. */
  start = 0;
  for (g = 1; g < len; g++) {
    if (glyphs[g].x != 0) {
      continue;
    }

    cairo_rectangle(cr,
		    glyphs[start].x,
		    glyphs[start].y - ay,
		    t->linewidth - PAD * 2 - glyphs[start].x,
		    ay + by);
		  
    cairo_fill(cr);

    start = g;
  }

  /* Draw background of remaining glyphs. */

  if (start < g) {
    cairo_glyph_extents(cr, &glyphs[start], g - start, &extents);

    cairo_rectangle(cr,
		    glyphs[start].x,
		    glyphs[start].y - ay,
		    extents.x_advance,
		    ay + by);
		  
    cairo_fill(cr);
  }

  /* Draw glyphs */
  cairo_set_source_rgb(cr, fg->r, fg->g, fg->b);
  cairo_show_glyphs(cr, glyphs, len);
}

/* This is pretty horrible. */

/* TODO: cursors and selections could be ordered to make search faster.
    I doubt this will improve it much but maybe in future. */

void
textboxdraw(struct textbox *t, cairo_t *cr, 
            int x, int y, int width, int height)
{
  struct selection *sel, *nsel;
  struct sequence *s;
  size_t i, g, start;
  struct colour *bg;
  struct piece *p;
  int32_t code, a;
  int ay, by;

	cairo_save(cr);
	cairo_rectangle(cr, x, y, width, height);
	cairo_clip(cr);

	cairo_translate(cr, x + PAD, y - t->yoff);

  cairo_set_source_rgb(cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(cr);

  cairo_set_font_face(cr, t->mace->font->cface);
  cairo_set_font_size(cr, t->mace->font->size);

  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);
	 
  s = t->sequence;
  p = &s->pieces[SEQ_start];
  i = 0;
  g = 0;
  start = 0;

  sel = NULL;
  bg = &t->bg;

  while (true) {
		while (i < p->len && g < p->nglyphs
	    	     && p->glyphs[g].y < t->yoff + height) {

      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
				i++;
				continue;
      }

      nsel = inselections(t->mace, t, p->pos + i);
      if (nsel != sel) {
				if (start < g) {
          /* Draw the undrawn glyphs with the previous selection. */
					drawglyphs(t, cr, 
					           &p->glyphs[start], g - start,
					           &nfg, bg, ay, by);
				}

				start = g;

				sel = nsel;
				if (sel != NULL) {
					switch (sel->type) {
					case SELECTION_normal:
						bg = &sbg;
						break;
					case SELECTION_command:
						bg = &cbg;
						break;
					}
				} else {
					bg = &t->bg;
				}
      }

      /* Line Break. */
      if (islinebreak(code, s->data + p->off + i, p->len - i, &a)) {
				if (start < g) {
					drawglyphs(t, cr, 
					                  &p->glyphs[start], g - start,
					                  &nfg, bg, ay, by);
				}

				start = g + 1;

				if (sel != NULL) {
          /* Draw selection on rest of line. */

					cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);
					cairo_rectangle(cr,
					                p->glyphs[g].x,
					                p->glyphs[g].y - ay,
					                t->linewidth - PAD * 2 - p->glyphs[g].x,
					                ay + by);
		  
					cairo_fill(cr); 
				}
	
      } else if (code == '\t') {
				if (start < g) {
					drawglyphs(t, cr, 
					           &p->glyphs[start], g - start,
					           &nfg, bg, ay, by);
				}

				start = g + 1;

				if (sel != NULL) {
          /* Draw selection on tab. */
					cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);
					cairo_rectangle(cr,
					                p->glyphs[g].x,
									        p->glyphs[g].y - ay,
										      t->mace->font->tabwidthpixels,
					                ay + by);
		  
					cairo_fill(cr); 
				}
      }

      if (cursorat(t->mace, t, p->pos + i) != NULL) {
				if (start <= g) {
					drawglyphs(t, cr, 
					           &p->glyphs[start], g - start + 1,
					           &nfg, bg, ay, by);
				}

				start = g + 1;

				drawcursor(t, cr,
						   p->glyphs[g].x,
						   p->glyphs[g].y - ay,
				       ay + by);
      }

      i += a;
      g++;
    }

    if (start < g) {
      drawglyphs(t, cr,
			                  &p->glyphs[start], g - start,
			                  &nfg, bg, ay, by);
    }

    if (g < p->nglyphs && p->glyphs[g].y >= t->yoff + height) {
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

  if (cursorat(t->mace, t, sequencelen(s)) != NULL) {
    p = &s->pieces[SEQ_end];
    drawcursor(t, cr,
	       p->glyphs[0].x,
	       p->glyphs[0].y - ay,
	       ay + by);
  }
  
	cairo_restore(cr);
} 

size_t
textboxfindpos(struct textbox *t, int lx, int ly)
{
  struct sequence *s;
  struct piece *p;
  int32_t code, a;
  int ww, ay, by;
  size_t i, g;

	lx -= PAD;
  ly += t->yoff;

	if (ly <= 0) {
		ly = 0;
	}
  
  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);
	 
  s = t->sequence;
  
  for (p = &s->pieces[SEQ_start]; p->next != -1; p = &s->pieces[p->next]) {
    for (i = 0, g = 0; i < p->len && g < p->nglyphs; i += a, g++) {
			while (i < p->len) {
   	   a = utf8iterate(s->data + p->off + i, p->len - i, &code);
				if (a != 0) {
					break;
				} else {
					i++;
				}
			}

			if (i >= p->len) {
				break;
			}

			if (ly < p->glyphs[g].y - ay || p->glyphs[g].y + by <= ly) {
				continue;
			}

      if (islinebreak(code, s->data + p->off + i, p->len - i, &a)) {
				return p->pos + i;

			} else if (g + 1 < p->nglyphs && p->glyphs[g + 1].x == 0.0) {
				/* Word wrapping. */
				return p->pos + i;

      } else if (code == '\t') {
				ww = t->mace->font->tabwidthpixels;

      } else {
				if (FT_Load_Glyph(t->mace->font->face, p->glyphs[g].index,
			      FT_LOAD_DEFAULT) != 0) {
					continue;
				}

				ww = t->mace->font->face->glyph->advance.x >> 6;
			}

			if (lx <= p->glyphs[g].x + ww * 0.75f) {
				/* Left three quarters of the glyph so go to the left side. */
				return p->pos + i;

			} else if (lx <= p->glyphs[g].x + ww) {
				/* Right quarters of the glyph so go to the right side. */
				return p->pos + i + a;
			}
    }
  }

  return sequencelen(s);
}
