#include "mace.h"

static struct colour nfg = { 0, 0, 0 };
/*
static struct colour sbg = { 0.5, 0.8, 0.7 };
*/

/*
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
*/

void
textboxpredraw(struct textbox *t)
{
  struct sequence *s;
  struct piece *p;
  size_t i;

  cairo_set_source_rgb(t->cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(t->cr);

  cairo_set_font_face(t->cr, t->font->cface);
  cairo_set_font_size(t->cr, t->font->size);

  s = t->sequence;
  p = &s->pieces[SEQ_start];
  i = 0;

  cairo_translate(t->cr, 0, -t->yoff);

  while (true) {
    /* TODO: selections, cursor. */
    cairo_set_source_rgb(t->cr, nfg.r, nfg.g, nfg.b);
    cairo_show_glyphs(t->cr, &p->glyphs[i], p->nglyphs - i);
    
    if (p->next == -1) {
      break;
    } else {
      p = &s->pieces[p->next];
      i = 0;
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

  ay = t->font->face->size->metrics.ascender >> 6;
  by = t->font->face->size->metrics.descender >> 6;
	 
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
      if (islinebreak(code,
		      s->data + p->off + i,
		      p->len - i, &a)) {

	if (p->glyphs[g].y - ay <= ly && ly <= p->glyphs[g].y - by) {
	  printf("pos found on new line %i around %f\n", ly, p->glyphs[g].y);
	  return p->pos + i;
	}
	
	i += a;
	continue;
      }

      if (FT_Load_Glyph(t->font->face, p->glyphs[g].index,
			FT_LOAD_DEFAULT) != 0) {
	i += a;
	continue;
      }

      ww = t->font->face->glyph->advance.x >> 6;

      if (p->glyphs[g].y - ay <= ly && ly <= p->glyphs[g].y - by) {
	if (p->glyphs[g].x <= lx && lx <= p->glyphs[g].x + ww) {
	  printf("pos found\n");
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

  printf("pos not found, probably at end\n");
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

	x = 0;
	y += t->font->lineheight;
	i += a;
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

