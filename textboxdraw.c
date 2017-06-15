#include "mace.h"
#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>

/* TODO: 

   Improve speed. Stop textboxcalcpositions from calculating 
   things it doesnt need to. eg: just push pieces down rather than 
   recalculating their width. 

   Reimpliment something like textboxfindstart to stop drawing
   unnessesary glyphs.

   I don't like how tabs or newlines are handled. It would possibly 
   be better to have each piece have another array of extra 
   information about the glyphs containing width, maybe height, 
   maybe colour. This way all glyphs could be handled in a uniform
   way.
*/

#define PAD 5

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
    if (glyphs[g].x != PAD) {
      continue;
    }

    cairo_rectangle(t->cr,
		    glyphs[start].x,
		    glyphs[start].y - ay,
		    t->linewidth - PAD - glyphs[start].x,
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
          /* Draw the undrawn glyphs with the previous selection. */
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
          /* Draw selection on rest of line. */

					cairo_set_source_rgb(t->cr, bg->r, bg->g, bg->b);
					cairo_rectangle(t->cr,
					                p->glyphs[g].x,
					                p->glyphs[g].y - ay,
					                t->linewidth - PAD - p->glyphs[g].x,
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
          /* Draw selection on tab. */
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
      } else if (p->glyphs[g].y - ay <= ly &&
				         ly <= p->glyphs[g].y + by &&
				         p->glyphs[g].x <= lx) {

				if (FT_Load_Glyph(t->font->face, p->glyphs[g].index,
			      FT_LOAD_DEFAULT) != 0) {
					i += a;
					continue;
				}

				ww = t->font->face->glyph->advance.x >> 6;

				if (lx <= p->glyphs[g].x + ww) {
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

static size_t
findlinebreak(hb_glyph_info_t *info, uint8_t *text, 
              size_t max, size_t min)
{
	size_t i;
	
	if (max == 0) {
		return max;
	}

	for (i = max - 1; i > min; i--) {
		if (iswordbreak(text[info[i].cluster])) {
			return i + 1;
		}
	}

	return max;
}

static size_t
placeglyphs(struct textbox *t, hb_buffer_t *hbbuf,
            uint8_t *text, size_t ntext,
            cairo_glyph_t *glyphs, size_t nglyphs,
            int *x, int *y)
{
  hb_glyph_position_t *pos;
  hb_glyph_info_t *info;
  unsigned int c;
  size_t g, linestart;
  int ww;

  hb_buffer_add_utf8(hbbuf, (const char *) text, ntext, 0, ntext);
  hb_buffer_guess_segment_properties(hbbuf);
  hb_shape(t->font->hbfont, hbbuf, NULL, 0);

  info = hb_buffer_get_glyph_infos(hbbuf, &c);
  pos = hb_buffer_get_glyph_positions(hbbuf, &c);

  /* If c is greater then we have a problem. 
     So just ignore the extra glyphs. */
  if (c < nglyphs) {
    nglyphs = c;
  }

	linestart = 0;
  for (g = 0; g < nglyphs; g++) {
    ww = pos[g].x_advance >> 6;

    if (*x + ww >= t->linewidth - PAD) {
      *x = PAD;
      *y += t->font->lineheight;

			g = findlinebreak(info, text, g, linestart);

			linestart = g;
			ww = pos[g].x_advance >> 6;
    }

    glyphs[g].index = info[g].codepoint;
    glyphs[g].x = *x + (pos[g].x_offset >> 6);
    glyphs[g].y = *y - (pos[g].y_offset >> 6);

    *x += ww;
    *y -= pos[g].y_advance >> 6;
  }

  hb_buffer_clear_contents(hbbuf);

  return nglyphs;
}

void
textboxcalcpositions(struct textbox *t, size_t pos)
{
  struct sequence *s;
  hb_buffer_t *hbbuf;
  struct piece *p;
  int32_t code, a;
  int x, y;
  size_t i, g, startg, starti;
  
  hbbuf = hb_buffer_create();
  hb_buffer_set_unicode_funcs(hbbuf,
			      hb_icu_get_unicode_funcs());

  s = t->sequence;
  p = &s->pieces[SEQ_start];
  i = 0;
  g = 0;
  startg = 0;
  starti = 0;

  x = PAD;
  y = t->font->baseline;
  
  while (true) {

    if (p->next == -1) {
      /* End which has one glyph. */
      p->glyphs[0].index = 0;
      p->glyphs[0].x = x;
      p->glyphs[0].y = y;
      
      break;
    }

    while (i < p->len && g < p->nglyphs) {
      a = utf8iterate(s->data + p->off + i, p->len - i, &code);
      if (a == 0) {
				i++;
				continue;
      }
      
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

        if (startg < g) {
					g = startg + 
					    placeglyphs(t, hbbuf,
					                s->data + p->off + starti,
				                  i - starti,
				                  &p->glyphs[startg],
				                  g - startg,
				                  &x, &y);
        }

				p->glyphs[g].index = 0;
				p->glyphs[g].x = x;
				p->glyphs[g].y = y;

				x = PAD;
				y += t->font->lineheight;

        startg = ++g;
        starti = (i += a);

      } else if (code == '\t') {

        if (startg < g) {
        	g = startg + 
					    placeglyphs(t, hbbuf,
					                s->data + p->off + starti,
				                  i - starti,
				                  &p->glyphs[startg],
				                  g - startg,
				                  &x, &y);
				}

				if (x + t->font->tabwidthpixels >= t->linewidth - PAD) {
					x = PAD;
					y += t->font->lineheight;
				}

				p->glyphs[g].index = 0;
				p->glyphs[g].x = x; 
				p->glyphs[g].y = y;

				x += t->font->tabwidthpixels;

        startg = ++g;
        starti = (i += a);

      } else {
        i += a;
        g++;
      }
    }

    if (startg < p->nglyphs && starti < p->len) {
      g = startg + 
			    placeglyphs(t, hbbuf,
			       s->data + p->off + starti,
			       p->len - starti,
			       &p->glyphs[startg],
			       p->nglyphs - startg,
             &x, &y);
    }

    p = &s->pieces[p->next];
    i = 0;
    g = 0;
    startg = 0;
    starti = 0;
  }

  t->height = y - (t->font->face->size->metrics.descender >> 6);

  hb_buffer_destroy(hbbuf);
}

