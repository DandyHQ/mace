#include <stdlib.h>
#include <string.h>

#include "mace.h"

static void
placeglyphs(struct sequence *s);

struct sequence *
sequencenew(struct font *font, uint8_t *data, size_t len)
{
  struct sequence *s;

  s = calloc(1, sizeof(struct sequence));
  if (s == NULL) {
    return NULL;
  }

	s->font = font;
	s->linewidth = -1;

  s->pmax = 10;
  s->pieces = malloc(sizeof(struct piece) * s->pmax);
  if (s->pieces == NULL) {
    return NULL;
  }

  s->plen = 3;

  /* This is pretty ugly. */
  
  s->pieces[SEQ_start].off = 0;
  s->pieces[SEQ_start].pos = 0;
  s->pieces[SEQ_start].len = 0;
  s->pieces[SEQ_start].nglyphs = 0;
  s->pieces[SEQ_start].glyphs = NULL;
  s->pieces[SEQ_start].prev = -1;
  s->pieces[SEQ_start].next = SEQ_end;

  s->pieces[SEQ_end].off = 0;
  s->pieces[SEQ_end].pos = 0;
  s->pieces[SEQ_end].len = 0;
  s->pieces[SEQ_end].nglyphs = 1;
  s->pieces[SEQ_end].glyphs = calloc(1, sizeof(cairo_glyph_t));
  if (s->pieces[SEQ_end].glyphs == NULL) {
    sequencefree(s);
    return NULL;
  }
  s->pieces[SEQ_end].prev = SEQ_start;
  s->pieces[SEQ_end].next = -1;

  if (data != NULL) {
    s->pieces[SEQ_first].off = 0;
    s->pieces[SEQ_first].pos = 0;
    s->pieces[SEQ_first].len = len;
    s->pieces[SEQ_first].prev = SEQ_start;
    s->pieces[SEQ_first].next = SEQ_end;

    s->pieces[SEQ_first].nglyphs = utf8codepoints(data, len);
    s->pieces[SEQ_first].glyphs =
      calloc(s->pieces[SEQ_first].nglyphs,
	     sizeof(cairo_glyph_t));

    if (s->pieces[SEQ_first].glyphs == NULL) {
      sequencefree(s);
      return NULL;
    }
 
    s->pieces[SEQ_start].next = SEQ_first;
    s->pieces[SEQ_end].prev = SEQ_first;

    s->pieces[SEQ_end].pos = len;

    s->data = data;
    s->dlen = len;
    s->dmax = len;

  } else {
    s->dlen = 0;
    s->dmax = 1024;

    s->data = malloc(s->dmax);
    if (s->data == NULL) {
      sequencefree(s);
      return NULL;
    }
  }

  return s;
}

void
sequencefree(struct sequence *s)
{
  free(s->pieces);

  free(s->data);

  free(s);
}

static void
shiftpieces(struct sequence *s, ssize_t p, size_t pos)
{
  p = SEQ_start;
  pos = 0;
  
  while (p != -1) {
    s->pieces[p].pos = pos;
    pos += s->pieces[p].len;
    p = s->pieces[p].next;
  }
}

static ssize_t
piecefind(struct sequence *s, ssize_t p, size_t pos, size_t *i)
{
  while (p != -1) {
    if (pos <= s->pieces[p].pos + s->pieces[p].len) {
      *i = pos - s->pieces[p].pos;
      return p;
    } else {
      p = s->pieces[p].next;
    }
  }

  return -1;
}

static ssize_t
pieceadd(struct sequence *s, size_t pos, size_t off, size_t len)
{
  cairo_glyph_t *glyphs;
  size_t nglyphs;

  nglyphs = utf8codepoints(s->data + off, len);
  glyphs = calloc(nglyphs, sizeof(cairo_glyph_t));
  if (glyphs == NULL) {
    return -1;
  }

  if (s->plen + 1 >= s->pmax) {
    s->pieces = realloc(s->pieces,
			sizeof(struct piece) * (s->plen + 10));

    if (s->pieces == NULL) {
      s->pmax = 0;
      return -1;
    }

    s->pmax = s->plen + 10;
  }

  s->pieces[s->plen].pos = pos;
  s->pieces[s->plen].off = off;
  s->pieces[s->plen].len = len;
  s->pieces[s->plen].nglyphs = nglyphs;
  s->pieces[s->plen].glyphs = glyphs;

  return s->plen++;
}

static bool
appenddata(struct sequence *s, const uint8_t *data, size_t len)
{
  uint8_t *ndata;
  size_t pg;

  pg = 1024;

  while (s->dlen + len >= s->dmax) {
    ndata = realloc(s->data, s->dmax + pg);

    if (ndata == NULL) {
      return false;
    } else {
      s->data = ndata;
      s->dmax += pg;
    }
  }

  memmove(s->data + s->dlen, data, len);

  s->dlen += len;

  return true;
}

static bool
sequenceappend(struct sequence *s, ssize_t p, size_t pos,
	       const uint8_t *data, size_t len)
{
  cairo_glyph_t *glyphs;
  size_t nglyphs;
  
  if (p != SEQ_start && p != SEQ_end
      && s->pieces[p].pos + s->pieces[p].len == pos
      && s->pieces[p].off + s->pieces[p].len == s->dlen) {

    nglyphs = s->pieces[p].nglyphs + utf8codepoints(data, len);
    glyphs = calloc(nglyphs, sizeof(cairo_glyph_t));
    if (glyphs == NULL) {
      return false;
    }
   
    if (!appenddata(s, data, len)) {
      free(glyphs);
      return false;
    }

    s->pieces[p].len += len;

    if (s->pieces[p].glyphs != NULL) {
      free(s->pieces[p].glyphs);
    }

    s->pieces[p].nglyphs = nglyphs;
    s->pieces[p].glyphs = glyphs;

    shiftpieces(s, p, s->pieces[p].pos);
		placeglyphs(s);

    return true;
  } else {
    return false;
  }
}

bool
sequenceinsert(struct sequence *s, size_t pos,
	       const uint8_t *data, size_t len)
{
  ssize_t p, pprev, pnext, n, l, r;
  size_t i;

  p = piecefind(s, SEQ_start, pos, &i);
  if (p == -1) {
    return false;
  }

  if (sequenceappend(s, p, pos, data, len)) {
    return true;
  }

  pprev = s->pieces[p].prev;
  pnext = s->pieces[p].next;

  if (!appenddata(s, data, len)) {
    return false;
  }

  n = pieceadd(s, pos, s->dlen - len, len);
  if (p == -1) {
    /* Should free appended data? */
    return false;
  }

  if (i == s->pieces[p].len) {
    /* New goes after p. */

    s->pieces[p].next = n;
    s->pieces[n].prev = p;
    s->pieces[n].next = pnext;
    s->pieces[pnext].prev = n;

  } else {
    /* Split p and put new in the middle. */

    l = pieceadd(s, s->pieces[p].pos,
		                    s->pieces[p].off, i);

    if (l == -1) {
      /* Should free n */
      return false;
    }

    r = pieceadd(s, pos + len,
		                     s->pieces[p].off + i,
		                     s->pieces[p].len - i);

    if (r == -1) {
      /* Should free n and l */
      return false;
    }

    s->pieces[pprev].next = l;
    s->pieces[l].prev = pprev;
    s->pieces[l].next = n;
    s->pieces[n].prev = l;
    s->pieces[n].next = r;
    s->pieces[r].prev = n;
    s->pieces[r].next = pnext;
    s->pieces[pnext].prev = r;

    if (s->pieces[p].glyphs != NULL) {
      free(s->pieces[p].glyphs);
      s->pieces[p].glyphs = NULL;
      s->pieces[p].nglyphs = 0;
    }
  }

  shiftpieces(s, n, pos);
	placeglyphs(s);

  return true;
}

/* TODO: This could go for some improvements. */

bool
sequencedelete(struct sequence *s, size_t pos, size_t len)
{
  ssize_t start, end, startprev, endnext, nstart, nend;
  size_t starti, endi;

  start = piecefind(s, SEQ_start, pos, &starti);
  if (start == -1) {
    return false;
  } else if (start == SEQ_start) {
    start = s->pieces[SEQ_start].next;
  }

  end = piecefind(s, start, pos + len, &endi);
  if (end == -1) {
    /* Use the very end. It is probably trying to delete one past the
       end. */

    end = s->pieces[SEQ_end].prev;
    endi = s->pieces[end].len;
  }

  startprev = s->pieces[start].prev;
  endnext   = s->pieces[end].next;

  nstart = pieceadd(s,
		    s->pieces[start].pos,
		    s->pieces[start].off,
		    starti);

  if (nstart == -1) {
    return false;
  }

  nend = pieceadd(s,
		  pos,
		  s->pieces[end].off + endi,
		  s->pieces[end].len - endi);

  if (nend == -1) {
    return false;
  }

  if (startprev != -1) {
    s->pieces[startprev].next = nstart;
  } else {
    /* We have a problem: Dangling start! */
  }

  s->pieces[nstart].prev = startprev;

  s->pieces[nstart].next = nend;
  s->pieces[nend].prev = nstart;

  s->pieces[nend].next = endnext;

  if (endnext != -1) {
    s->pieces[endnext].prev = nend;
  } else {
    /* We have a problem: Dangling end! */
    /* Can't give up now. */
  }

  if (s->pieces[start].glyphs != NULL) {
    free(s->pieces[start].glyphs);
    s->pieces[start].glyphs = NULL;
    s->pieces[start].nglyphs = 0;
  }
 
  if (s->pieces[end].glyphs != NULL) {
    free(s->pieces[end].glyphs);
    s->pieces[end].glyphs = NULL;
    s->pieces[end].nglyphs = 0;
  }

  shiftpieces(s, nend, pos);
	placeglyphs(s);

  return true;
}

static size_t
findwordend(struct sequence *s, ssize_t p, size_t i)
{
  int32_t code, a;
  size_t end;

  end = 0;
  while (p != -1) {
    while (i < s->pieces[p].len) {
      a = utf8iterate(s->data + s->pieces[p].off + i,
			                       s->pieces[p].len - i, &code);
      if (a == 0) {
				i += 1;
				continue;
      }

      if (iswordbreak(code)
			    || islinebreak(code, s->data + s->pieces[p].off + i,
			                        s->pieces[p].len - i, &a)) {

					return end;
      }

      i += a;
      end += a;
    }

    i = 0;
    p = s->pieces[p].next;
  }

  return end;
}

static size_t
findwordstart(struct sequence *s, ssize_t p, size_t ii)
{
  size_t piecedist, i;
  int32_t code, a;
  ssize_t in;

  piecedist = 0;
  in = -1;

  while (true) {
    i = 0;
    while (i < ii) {
      a = utf8iterate(s->data + s->pieces[p].off + i,
		      s->pieces[p].len - i, &code);
      if (a == 0) {
				i += 1;
				continue;
      }

      if (iswordbreak(code)
			    || islinebreak(code, s->data + s->pieces[p].off + i,
					                    s->pieces[p].len - i, &a)) {

				in = i + a;
      }

      i += a;
    }

    piecedist += ii;

    p = s->pieces[p].prev;
    if (p == -1 || in != -1) {
      break;
    } else {
      ii = s->pieces[p].len;
    }
  }

  if (in == -1) {
    return piecedist;
  } else {
    return piecedist - in;
  }
}

bool
sequencefindword(struct sequence *s, size_t pos,
		 size_t *begin, size_t *len)
{
  size_t start, end, i;
  ssize_t p;

  p = piecefind(s, SEQ_start, pos, &i);
  if (p == -1) {
    return false;
  }

  start = findwordstart(s, p, i);
  end = findwordend(s, p, i);

  *begin = pos - start;
  *len = start + end;

  return true;
}

size_t
sequenceget(struct sequence *s, size_t pos,
	    uint8_t *buf, size_t len)
{
  size_t i, ii, l;
  ssize_t p;

  p = piecefind(s, SEQ_start, pos, &i);
  if (p == -1) {
    return 0;
  }

  ii = 0;
  while (p != -1 && ii < len) {
    if (ii + s->pieces[p].len - i >= len) {
      l = len - ii;
    } else {
      l = s->pieces[p].len - i;
    }

    memmove(buf + ii,
	    s->data + s->pieces[p].off + i,
	    l);

    ii += l;
    p = s->pieces[p].next;
    i = 0;
  }

  return ii;
}

size_t
sequencelen(struct sequence *s)
{
  return s->pieces[SEQ_end].pos;
}

void
sequencesetlinewidth(struct sequence *s, int l)
{
	if (s->linewidth != l) {
		s->linewidth = l;
		placeglyphs(s);
	}
}

int
sequenceheight(struct sequence *s)
{
	return s->pieces[SEQ_end].glyphs[0].y 
	    - (s->font->face->size->metrics.descender >> 6);
}

static void
placesomeglyphs(struct sequence *s,
                           cairo_glyph_t *glyphs, size_t nglyphs,
                           int *x, int *y)
{
  size_t g, gg, linestart, lineend;
	int32_t index;
  int ww;

	linestart = 0;
	lineend = 0;

  for (g = 0; g < nglyphs; g++) {
		index = FT_Get_Char_Index(s->font->face, glyphs[g].index);
		if (FT_Load_Glyph(s->font->face, index, FT_LOAD_DEFAULT) != 0) {
			continue;
		}

    ww = s->font->face->glyph->advance.x >> 6;

    if (*x + ww >= s->linewidth) {
			*x = 0;
			*y += s->font->lineheight;

			if (linestart < lineend) {
				/* Only attempt word breaks if there are words to break. */

				for (gg = lineend; gg < g; gg++) {
					glyphs[gg].x = *x;
					glyphs[gg].y = *y;
				
					if (FT_Load_Glyph(s->font->face, 
					                           glyphs[gg].index, 
					                           FT_LOAD_DEFAULT) != 0) {
						continue;
					}

					*x += s->font->face->glyph->advance.x >> 6;
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

static void
placeglyphs(struct sequence *s)
{
  size_t i, g, startg;
  struct piece *p;
  int32_t code, a;
  int x, y;

  x = 0;
  y = s->font->baseline;
  
  for (p = &s->pieces[SEQ_start]; 
	          p->next != -1; 
	          p = &s->pieces[p->next]) {
	
	  startg = 0;
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

      if (islinebreak(code, s->data + p->off + i, p->len - i, &a)) {
        if (startg < g) {
					placesomeglyphs(s, 
				              &p->glyphs[startg],
				              g - startg,
				              &x, &y);
        }

				p->glyphs[g].index = 0;
				p->glyphs[g].x = x;
				p->glyphs[g].y = y;

				x = 0;
				y += s->font->lineheight;

        startg = g + 1;

      } else if (code == '\t') {
        if (startg < g) {
					placesomeglyphs(s, 
				                  &p->glyphs[startg],
				                  g - startg,
				                  &x, &y);
				}

				if (x + s->font->tabwidthpixels >= s->linewidth) {
					x = 0;
					y += s->font->lineheight;
				}

				p->glyphs[g].index = 0;
				p->glyphs[g].x = x; 
				p->glyphs[g].y = y;

				x += s->font->tabwidthpixels;

        startg = g + 1;

      } else {
				p->glyphs[g].index = code;
			}
    }

    if (startg < p->nglyphs) {
      placesomeglyphs(s,
			       &p->glyphs[startg],
			       p->nglyphs - startg,
             &x, &y);
    }
  }

	/* End which has one glyph. */
	p->glyphs[0].index = 0;
	p->glyphs[0].x = x;
	p->glyphs[0].y = y;
}

