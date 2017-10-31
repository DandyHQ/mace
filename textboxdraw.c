#include "mace.h"
#include FT_ADVANCES_H

/* TODO:
   I don't like how tabs or newlines are handled. It would possibly
   be better to have each piece have another array of extra
   information about the glyphs containing width, maybe height,
   maybe colour. This way all glyphs could be handled in a uniform
   way.

   This file is pretty hacky. Rendering is hard and so is line
   wrapping and everything that comes with that. I don't like
   any of it.
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

  if (len == 0) {
    return;
  }

  if (bg != NULL) {
    cairo_set_source_rgb(cr, bg->r, bg->g, bg->b);

    for (start = 0, g = 1; g < len; g++) {
      if (glyphs[g].x != 0.0) {
        continue;
      }

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
  bool drawcursornow;
  struct sequence *s;
  struct colour *bg;
  struct cursel *cs;
  struct piece *p;
  int32_t code;
  int ay, by;
  ssize_t pp;
  bg = NULL;
  drawcursornow = false;
  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);
  cairo_save(cr);
  cairo_rectangle(cr, x, y, width, height);
  cairo_clip(cr);
  cairo_translate(cr, x + PAD, y);
  cairo_set_source_rgb(cr, t->bg.r, t->bg.g, t->bg.b);
  cairo_paint(cr);
  cairo_set_font_face(cr, t->mace->font->cface);
  cairo_set_font_size(cr, t->mace->font->size);
  cs = t->mace->cursels;
  
  t->x = x;
  t->y = y;

  while (cs != NULL) {
    if (cs->tb == t) {
      break;
    } else {
      cs = cs->next;
    }
  }

  s = t->sequence;
  pp = sequencepiecefind(s, SEQ_start, t->start, &ii);

  if (pp == -1) {
    return;
  }

  p = &s->pieces[pp];

  for (startg = 0, g = 0; g < t->nglyphs; g++, ii += a) {
    while (ii >= p->len) {
      pp = p->next;

      if (pp == SEQ_end) {
        goto end;
      } else {
        p = &s->pieces[pp];
        ii = 0;
      }
    }

    /* This is horrible. textboxplaceglyphs may place too many. */
    if (t->glyphs[g].y > t->maxheight) {
      t->drawablelen = p->off + ii - t->start;
      t->nglyphs = g;
      break;
    }

    a = utf8iterate(s->data + p->off + ii, p->len - ii, &code);

    if (a == 0) {
      break;
    }

    if (cs != NULL) {
      if (cs->start + cs->cur == p->pos + ii
          && (cs->type & CURSEL_nrm) != 0) {
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
      /* This is g + 1 because it needs to draw the current glyph
         as well. Tab and linebreak don't. */
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

  if (startg < g) {
    drawglyphs(cr, &t->glyphs[startg], g - startg,
               &nfg, bg,
               t->linewidth - PAD * 2,
               ay, by);
  }

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

  if (ly <= 0) {
    ly = 0;
  }

  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);
  i = t->start;

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
    } else if (g + 1 < t->nglyphs
               && t->glyphs[g + 1].x == 0.0) {
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
    if (glyphs[g].index == '\t') {
      index = 0;
      ww = t->mace->font->tabwidthpixels;
    } else {
      index = FT_Get_Char_Index(t->mace->font->face,
                                glyphs[g].index);

      if (FT_Get_Advance(t->mace->font->face,
                         index, FT_LOAD_DEFAULT,
                         &advance) != 0) {
        continue;
      }

      ww = advance / 65536;
    }

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
  int x, y, ay, by;
  cairo_glyph_t *n;
  struct piece *p;
  int32_t code;
  ssize_t pp;
  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);
  x = 0;
  y = t->mace->font->baseline;
  g = 0;
  startg = 0;
  s = t->sequence;
  pp = sequencepiecefind(s, SEQ_start, t->start, &i);

  if (pp == -1) {
    return;
  }

  p = &s->pieces[pp];

  while (y + by < t->maxheight) {
    while (i >= p->len) {
      pp = p->next;

      if (pp == SEQ_end) {
        goto end;
      } else {
        p = &s->pieces[pp];
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
    } else {
      t->glyphs[g].index = code;
    }

    if (++g == t->nglyphsmax) {
      t->nglyphsmax += 100;
      n = realloc(t->glyphs,
                  t->nglyphsmax * sizeof(cairo_glyph_t));

      if (n == NULL) {
        t->drawablelen = p->pos + i - t->start;
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

  /* Do we still need to do this? */
  /* Last glyph stores the end. */
  t->nglyphs = g;
  t->glyphs[t->nglyphs].index = 0;
  t->glyphs[t->nglyphs].x = x;
  t->glyphs[t->nglyphs].y = y;
  t->height = y + by;
  t->drawablelen = p->pos + i - t->start;
}

/* Glyphs must be large enough to store end - start + 1
   glyphs. Returns the number of glyphs loaded. */

static size_t
loadandplaceglyphs(struct textbox *t,
                   cairo_glyph_t *glyphs,
                   size_t start, size_t end,
                   int *x, int *y,
                   size_t findpos, size_t *glyphpos)
{
  size_t i, g, a;
  int32_t code;
  int ay, by;
  ay = (t->mace->font->face->size->metrics.ascender >> 6);
  by = -(t->mace->font->face->size->metrics.descender >> 6);

  for (i = start, g = 0; i < end; g++, i += a) {
    a = sequencecodepoint(t->sequence, i, &code);

    if (a == 0) {
      break;
    } else {
      glyphs[g].index = code;
    }

    if (i == findpos) {
      *glyphpos = g;
    }
  }

  placesomeglyphs(t, glyphs, g,
                  x, y,
                  ay, by);
  return g;
}

static size_t
findglyphabove(struct sequence *s,
               size_t start, size_t end,
               cairo_glyph_t *glyphs, size_t nglyphs,
               size_t startg,
               float wantx, float wanty)
{
  size_t g, i, a;
  int32_t code;

  for (g = startg, i = end; i > start; g--) {
    a = sequenceprevcodepoint(s, i, &code);

    if (a == 0) {
      break;
    } else {
      i -= a;
    }

    if (glyphs[g].y < wanty) {
      if (glyphs[g].x <= wantx) {
        break;
      }
    }
  }

  return i;
}

size_t
textboxindexabove(struct textbox *t, size_t pos)
{
  size_t start, end, nglyphs, wantg, g, i;
  cairo_glyph_t *glyphs;
  float wantx, wanty;
  int x, y;
  start = sequenceindexline(t->sequence, pos);
  end = sequenceindexnextline(t->sequence, pos);
  nglyphs = end - start;

  if (nglyphs != 0) {
    glyphs = malloc(sizeof(cairo_glyph_t) * nglyphs);

    if (glyphs == NULL) {
      return pos;
    }

    x = y = 0;
    wantg = 0;
    g = loadandplaceglyphs(t, glyphs,
                           start, end,
                           &x, &y,
                           pos, &wantg);
    wantx = glyphs[wantg].x;
    wanty = glyphs[wantg].y;
    i = findglyphabove(t->sequence, start, pos,
                       glyphs, g, wantg,
                       wantx, wanty);
    free(glyphs);

    /* Did we find a glyph below? */
    if (i > start || (wantx == 0.0 && wanty > 0.0)) {
      return i;
    }
  } else {
    wantx = 0.0;
  }

  /* Any y value will do. */
  wanty = 1000000; /* TODO: this should be infinite. */
  end = start;
  start = sequenceindexprevline(t->sequence, end);

  if (start == end) {
    return start;
  }

  nglyphs = end - start;
  glyphs = malloc(sizeof(cairo_glyph_t) * nglyphs);

  if (glyphs == NULL) {
    return start;
  }

  x = y = 0;
  g = loadandplaceglyphs(t, glyphs,
                         start, end,
                         &x, &y,
                         pos, &wantg);

  if (g == 0) {
    free(glyphs);
    return start;
  }

  i = findglyphabove(t->sequence, start, end,
                     glyphs, g, g - 1,
                     wantx, wanty);
  free(glyphs);
  return i;
}

static size_t
findglyphbelow(struct sequence *s,
               size_t start, size_t end,
               cairo_glyph_t *glyphs, size_t nglyphs,
               size_t startg,
               float wantx, float wanty)
{
  size_t g, i, a;
  int32_t code;

  for (g = startg, i = start; i < end
       && g < nglyphs; g++, i += a) {
    a = sequencecodepoint(s, i, &code);

    if (a == 0) {
      break;
    } else if (glyphs[g].y > wanty) {
      if (g + 1 == nglyphs || glyphs[g + 1].y > glyphs[g].y) {
        break;
      } else if (glyphs[g].x <= wantx) {
        if (g + 1 == nglyphs || glyphs[g + 1].x > wantx) {
          break;
        }
      }
    }
  }

  return i;
}

size_t
textboxindexbelow(struct textbox *t, size_t pos)
{
  size_t start, end, nglyphs, wantg, g, i;
  cairo_glyph_t *glyphs;
  float wantx, wanty;
  int x, y;
  start = sequenceindexline(t->sequence, pos);
  end = sequenceindexnextline(t->sequence, pos);
  nglyphs = end - start;

  if (nglyphs != 0) {
    glyphs = calloc(nglyphs, sizeof(cairo_glyph_t));

    if (glyphs == NULL) {
      return pos;
    }

    x = y = 0;
    wantg = 0;
    g = loadandplaceglyphs(t, glyphs,
                           start, end,
                           &x, &y,
                           pos, &wantg);
    wantx = glyphs[wantg].x;
    wanty = glyphs[wantg].y;
    i = findglyphbelow(t->sequence, pos, end,
                       glyphs, g, wantg,
                       wantx, wanty);
    free(glyphs);

    /* Did we find a glyph below? */
    if (i < end) {
      return i;
    }
  } else {
    wantx = 0.0;
  }

  /* Any y value will do. */
  wanty = -1.0;
  start = end;
  end = sequenceindexnextline(t->sequence, start);

  if (start == end) {
    return start;
  }

  nglyphs = end - start;
  glyphs = calloc(nglyphs, sizeof(cairo_glyph_t));

  if (glyphs == NULL) {
    return start;
  }

  x = y = 0;
  g = loadandplaceglyphs(t, glyphs,
                         start, end,
                         &x, &y,
                         pos, &wantg);
  i = findglyphbelow(t->sequence, start, end,
                     glyphs, g, 0,
                     wantx, wanty);
  free(glyphs);
  return i;
}

size_t
textboxindentation(struct textbox *t, size_t start,
                   uint8_t *buf, size_t len)
{
  size_t a, i, ii;
  int32_t code;
  /* TODO: This is broken. It should only find whitespace from
     the start of the line to i. Otherwise if you have whitespace
     after your cursor it moves your cursor across. */
  i = start;

  while ((a = sequenceprevcodepoint(t->sequence, i,
                                    &code)) != 0) {
    if (islinebreak(code)) {
      break;
    } else {
      i -= a;
    }
  }

  for (ii = 0; ii < len && i + ii < start; ii += a) {
    a = sequencecodepoint(t->sequence, i + ii, &code);

    if (a == 0) {
      break;
    } else if (!iswhitespace(code)) {
      break;
    }
  }

  return sequenceget(t->sequence, i, buf, ii);
}


