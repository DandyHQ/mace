#include "mace.h"
#include FT_ADVANCES_H

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
drawcursor(cairo_t *cr, int x, int y, int h)
{
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, 1.3);

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
drawglyphs(cairo_t *cr,
           cairo_glyph_t *glyphs, size_t len,
           struct colour *fg, struct colour *bg,
           int lw, int ay, int by)
{
	size_t g, start;

	if (len == 0) return;

	if (bg != NULL) {		
		cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);

		for (start = 0, g = 1; g < len; g++) {
			if (glyphs[g].x != 0.0) continue;

			cairo_rectangle(cr,
			                glyphs[start].x,
	                    glyphs[start].y - ay + 1,
			                lw - glyphs[start].x,
	                    ay + by);

			cairo_fill(cr);
			start = g;
		}

		if (start < g) {
			cairo_rectangle(cr,
			                glyphs[start].x,
	                    glyphs[start].y - ay + 1,
			                glyphs[g].x - glyphs[start].x,
	                    ay + by);

			cairo_fill(cr);
		}
	}

  cairo_set_source_rgb(cr, fg->r, fg->g, fg->b);
  cairo_show_glyphs(cr, glyphs, len);
}

void
textboxdraw(struct textbox *t, cairo_t *cr, 
            int x, int y, int width, int height)
{
	size_t g, startg, ii, a;
	struct colour *bg;
	struct sequence *s;
	struct cursel *cs;
	struct piece *p;
	int32_t code;
	int ay, by;
	bool drawcursornow;
	
	bg = NULL;

  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);

	cairo_save(cr);
	cairo_rectangle(cr, x, y, width, height);
	cairo_clip(cr);

	cairo_translate(cr, x + PAD, y - t->yoff);

  cairo_set_source_rgb(cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(cr);

  cairo_set_font_face(cr, t->mace->font->cface);
  cairo_set_font_size(cr, t->mace->font->size);

	cs = t->mace->cursels;
	while (cs != NULL) {
		if (cs->tb == t) {
			break;
		} else {
			cs = cs->next;
		}
	}

	s = t->sequence;
	p = &s->pieces[SEQ_start];
	ii = 0;
	drawcursornow = false;

	for (startg = 0, g = 0; g < t->nglyphs; g++, ii += a) {
		while (ii >= p->len) {
			if (p->next == SEQ_end) {
				goto end;
			} else {
				p = &s->pieces[p->next];
				ii = 0;
			}
		}

		a = utf8iterate(s->data + p->off + ii, p->len - ii, &code);
		if (a == 0) {
			break;
		}

		if (cs != NULL) {
			if (cs->start + cs->cur == p->pos + ii && (cs->type & CURSEL_nrm) != 0) {
				drawcursornow = true;
			} else {
				drawcursornow = false;
			}

			if (cs->end == p->pos + ii) {
				if (startg < g) {
					drawglyphs(cr, &t->glyphs[startg], g - startg,
					           &nfg, bg,
					           t->linewidth - PAD * 2,
					           ay, by);
				}
				
				startg = g;

				bg = NULL;
				cs = cs->next;
				if (cs != NULL && cs->tb != t) {
					cs = NULL;
				}
			}
		}

		if (cs != NULL && cs->start == p->pos + ii) {
			if (startg < g) {
				drawglyphs(cr, &t->glyphs[startg], g - startg,
				           &nfg, bg,
				           t->linewidth - PAD * 2,
				           ay, by);
			}
			
			startg = g;

			if (cs->start == cs->end) {
				bg = NULL;
				drawcursornow = true;
			} else if ((cs->type & CURSEL_cmd) != 0) {
				bg = &cbg;
			} else {
				bg = &sbg;
			}
		}

		if (islinebreak(code)) {
			if (startg < g) {
				drawglyphs(cr, &t->glyphs[startg], g - startg,
				           &nfg, bg,
				           t->linewidth - PAD * 2,
				           ay, by);
			}
			
			startg = g + 1;
			
			if (bg != NULL) {
				cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);
				cairo_rectangle(cr,
			  	              t->glyphs[g].x,
 	   	                t->glyphs[g].y - ay + 1,
				                t->linewidth - PAD * 2 - t->glyphs[g].x,
				                ay + by);

				cairo_fill(cr);
			}
		} else if (code == '\t') {
			if (startg < g) {
				drawglyphs(cr, &t->glyphs[startg], g - startg,
				           &nfg, bg,
				           t->linewidth - PAD * 2,
				           ay, by);
			}
			
			startg = g + 1;
			
			if (bg != NULL) {
				cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);
				cairo_rectangle(cr,
			  	              t->glyphs[g].x,
	    	                t->glyphs[g].y - ay + 1,
				                t->mace->font->tabwidthpixels,
				                ay + by);

				cairo_fill(cr);
			}
		}

		if (drawcursornow) {
			if (startg < g + 1) {
				drawglyphs(cr, &t->glyphs[startg], g - startg + 1,
				           &nfg, bg,
				           t->linewidth - PAD * 2,
				           ay, by);
			}

			startg = g + 1;

			drawcursor(cr, t->glyphs[g].x, t->glyphs[g].y - ay + 1,
			           ay + by);

			drawcursornow = false;
		}
	}

end:

	drawglyphs(cr, &t->glyphs[startg], g - startg,
	           &nfg, bg,
	           t->linewidth - PAD * 2,
	           ay, by);

	if (cs != NULL && (cs->type & CURSEL_nrm) != 0 &&
	    cs->start + cs->cur == sequencelen(t->sequence)) {

			drawcursor(cr, 
			           t->glyphs[t->nglyphs].x, 
			           t->glyphs[t->nglyphs].y - ay + 1, 
			           ay + by);
	}

	cairo_restore(cr);
} 

size_t
textboxfindpos(struct textbox *t, int lx, int ly)
{
  int32_t code;
  int ww, ay, by;
  size_t i, g, a;

	lx -= PAD;
  ly += t->yoff;

	if (ly <= 0) {
		ly = 0;
	}
  
  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);
	 
	i = 0;

	for (g = 0; g < t->nglyphs; g++, i += a) {
		a = sequencecodepoint(t->sequence, i, &code);
		if (a == 0) {
			break;
		}
		
		if (ly < t->glyphs[g].y - ay || t->glyphs[g].y + by <= ly) {
			continue;
		}

		if (islinebreak(code)) {
			return i;

		} else if (g + 1 < t->nglyphs && t->glyphs[g + 1].x == 0.0) {
			/* Word wrapping. */
			return i;

		} else if (code == '\t') {
			ww = t->mace->font->tabwidthpixels;

		} else {
			if (FT_Load_Glyph(t->mace->font->face, t->glyphs[g].index,
			    FT_LOAD_DEFAULT) != 0) {
				continue;
			}

			ww = t->mace->font->face->glyph->advance.x >> 6;
		}

		if (lx <= t->glyphs[g].x + ww * 0.75f) {
			/* Left three quarters of the glyph so go to the left side. */
			return i;

		} else if (lx <= t->glyphs[g].x + ww) {
			/* Right quarters of the glyph so go to the right side. */
			return i + a;
		}
  }

	/* Probably off the screen. */
  return i;
}

static void
placesomeglyphs(struct textbox *t,
                cairo_glyph_t *glyphs, size_t nglyphs,
                int *x, int *y,
                int ay, int by)
{
  size_t g, gg, linestart, lineend;
  FT_Fixed advance;
	int32_t index;
  int ww;

	linestart = 0;
	lineend = 0;

  for (g = 0; g < nglyphs; g++) {
		index = FT_Get_Char_Index(t->mace->font->face, glyphs[g].index);
		
		if (FT_Get_Advance(t->mace->font->face, 
		                   index, FT_LOAD_DEFAULT, 
		                   &advance) != 0) {
			continue;
		}

    ww = advance / 65536;

    if (*x + ww >= t->linewidth) {
			*x = 0;
			*y += ay + by;

			if (linestart < lineend) {
				/* Only attempt word breaks if there are words to break. */

				for (gg = lineend; gg < g; gg++) {
					glyphs[gg].x = *x;
					glyphs[gg].y = *y;
				
					if (FT_Get_Advance(t->mace->font->face, 
					                   glyphs[gg].index, FT_LOAD_DEFAULT,
					                   &advance) != 0) {
						continue;
					}

					*x += advance / 65536;
				}
			}

			linestart = lineend;

    } else if (iswordbreak(glyphs[g].index)) {
			lineend = g + 1;
		}

    glyphs[g].index = index;
    glyphs[g].x = *x;
    glyphs[g].y = *y;

    *x += ww;
  }
}

void
textboxplaceglyphs(struct textbox *t)
{
  size_t i, g, a, startg;
	struct sequence *s;
  cairo_glyph_t *n;
  int x, y, ay, by;
	struct piece *p;
  int32_t code;

	s = t->sequence;
	p = &s->pieces[SEQ_start];
	
  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);
  
  x = 0;
  y = t->mace->font->baseline;
	g = 0;
	startg = 0;
  i = 0;

	while (true) {
		while (i >= p->len) {
			if (p->next == SEQ_end) {
				goto end;
			} else {
				p = &s->pieces[p->next];
				i = 0;
			}
		}

		a = utf8iterate(s->data + p->off + i, p->len - i, &code);
		if (a == 0) {
			goto end;
		} else {
			i += a;
		}

		if (islinebreak(code)) {
			if (startg < g) {
				placesomeglyphs(t, 
			                  &t->glyphs[startg],
			                  g - startg,
			                  &x, &y,
			                  ay, by);
      }

			t->glyphs[g].index = 0;
			t->glyphs[g].x = x;
			t->glyphs[g].y = y;

			x = 0;
			y += ay + by;

      startg = g + 1;

    } else if (code == '\t') {
      if (startg < g) {
				placesomeglyphs(t, 
			                  &t->glyphs[startg],
			                  g - startg,
			                  &x, &y,
			                  ay, by);
			}

			if (x + t->mace->font->tabwidthpixels >= t->linewidth) {
				x = 0;
				y += ay + by;
			}

			t->glyphs[g].index = 0;
			t->glyphs[g].x = x; 
			t->glyphs[g].y = y;

			x += t->mace->font->tabwidthpixels;

      startg = g + 1;

    } else {
			t->glyphs[g].index = code;
		}

		if (++g == t->nglyphsmax) {
			t->nglyphsmax += 100;
			n = realloc(t->glyphs, t->nglyphsmax * sizeof(cairo_glyph_t));
			if (n == NULL) {
				return;
			}
			t->glyphs = n;
    }
	}

end:

  if (startg < g) {
    placesomeglyphs(t,
		                &t->glyphs[startg],
		                g - startg,
                    &x, &y,
                    ay, by);
  }

	/* Last glyph stores the end. */

	t->nglyphs = g;
	t->glyphs[t->nglyphs].index = 0;
	t->glyphs[t->nglyphs].x = x;
	t->glyphs[t->nglyphs].y = y;

	t->height = y + by;
}

size_t
textboxindexabove(struct textbox *t, size_t pos)
{
  int32_t code;
  size_t i, g, a;
	int x, y;
	 
	i = 0;

	for (g = 0; g < t->nglyphs && i < pos; g++, i += a) {
		a = sequencecodepoint(t->sequence, i, &code);
		if (a == 0) {
			break;
		}
  }

	if (i != pos) {
		return 0;
	} else {
		x = t->glyphs[g].x;
		y = t->glyphs[g].y;
	}

	while (--g > 0) {
		a = sequenceprevcodepoint(t->sequence, i, &code);
		if (a == 0) {
			break;
		} else {
			i -= a;
		}

	 if (t->glyphs[g].y < y && t->glyphs[g].x <= x) {
			return i;
		}
	}

  return 0;
}

size_t
textboxindexbelow(struct textbox *t, size_t pos)
{
  int32_t code;
  size_t i, g, a;
	int x, y;
	 
	i = 0;

	for (g = 0; g < t->nglyphs && i < pos; g++, i += a) {
		a = sequencecodepoint(t->sequence, i, &code);
		if (a == 0) {
			break;
		}
  }

	if (i != pos) {
		return 0;
	} else {
		x = t->glyphs[g].x;
		y = t->glyphs[g].y;
	}

	while (++g < t->nglyphs) {
		a = sequencecodepoint(t->sequence, i, &code);
		if (a == 0) {
			break;
		} else {
			i += a;
		}

		if (t->glyphs[g].y > y) {
			if (g+1 < t->nglyphs && t->glyphs[g+1].y > t->glyphs[g].y) {
				return i;
			} else if (t->glyphs[g].x >= x) {
				return i;
			}
		}
	}

  return sequencelen(t->sequence);
}

size_t
textboxindentation(struct textbox *t, size_t i,
                   uint8_t *buf, size_t len)
{
	size_t a, ii;
	int32_t code;

	while ((a = sequenceprevcodepoint(t->sequence, i, &code)) != 0) {
		if (islinebreak(code)) {
			break;
		} else {
			i -= a;
		}
	}

	for (ii = 0; ii < len; ii += a) {
		a = sequencecodepoint(t->sequence, i + ii, &code);
		if (a == 0) {
			break;
		} else if (!iswhitespace(code)) {
			break;
		}
	}

	return sequenceget(t->sequence, i, buf, ii);
}


