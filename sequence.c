#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

struct sequence *
sequencenew(void)
{
  struct sequence *s;

  s = malloc(sizeof(struct sequence));
  if (s == NULL) {
    return NULL;
  }

  s->pmax = 10;
  s->pieces = malloc(sizeof(struct piece) * s->pmax);
  if (s->pieces == NULL) {
    return NULL;
  }

  s->plen = 1;
  s->pieces[SEQ_start].off = 0;
  s->pieces[SEQ_start].len = 0;
  s->pieces[SEQ_start].prev = 0;
  s->pieces[SEQ_start].next = SEQ_end;

  s->plen = 2;
  s->pieces[SEQ_end].off = 0;
  s->pieces[SEQ_end].len = 0;
  s->pieces[SEQ_end].prev = SEQ_start;
  s->pieces[SEQ_end].next = 0;

  s->data = NULL;
  s->dlen = 0;
  s->dmax = 0;

  return s;
}

void
sequencefree(struct sequence *s)
{
  free(s->pieces);
  free(s->data);
  free(s);
}

static ssize_t
piecefind(struct sequence *s, size_t pos, size_t *i)
{
  size_t p;
  size_t pi;

  pi = 0;
  p = SEQ_start;

  for (p = SEQ_start; p != SEQ_end; p = s->pieces[p].next) {
    if (pi + s->pieces[p].len >= pos) {
      *i = pos - pi;
      return p;
    } else {
      pi += s->pieces[p].len;
    }
  }

  return -1;
}

static ssize_t
pieceadd(struct sequence *s, size_t off, size_t len)
{
  if (s->plen + 1 >= s->pmax) {
    s->pieces = realloc(s->pieces,
			sizeof(struct piece) * (s->plen + 10));

    if (s->pieces == NULL) {
      s->pmax = 0;
      return -1;
    }
    
    s->pmax = s->plen + 10;
  }

  s->pieces[s->plen].off = off;
  s->pieces[s->plen].len = len;

  return s->plen++;
}

static ssize_t
appenddata(struct sequence *s, uint8_t *data, size_t len)
{
  size_t p;

  if (s->dlen + len >= s->dmax) {
    s->data = realloc(s->data, s->dlen + len);
    if (s->data == NULL) {
      return -1;
    }

    s->dmax = s->dlen + len;
  }

  memmove(s->data + s->dlen, data, len);

  p = pieceadd(s, s->dlen, len);
  if (p == -1) {
    return -1;
  }
  
  s->dlen += len;

  return p;
}

/* TODO: allow last added piece to grow */

bool
sequenceinsert(struct sequence *s, size_t pos,
	       uint8_t *data, size_t len)
{
  ssize_t p, pprev, pnext, n, l, r;
  size_t i;

  p = piecefind(s, pos, &i);
  if (p == -1) {
    return false;
  }

  pprev = s->pieces[p].prev;
  pnext = s->pieces[p].next;
  
  n = appenddata(s, data, len);
  if (n == -1) {
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

    l = pieceadd(s, s->pieces[p].off, i);
    if (l == -1) {
      /* Free n? */
      return false;
    }
    
    r = pieceadd(s, s->pieces[p].off + i, s->pieces[p].len - i);
    if (r == -1) {
      /* Free n and l? */
      return false;
    }

    s->pieces[pprev].next = l;
    s->pieces[l].prev = pprev;
    s->pieces[l].next = n;
    s->pieces[n].prev = l;
    s->pieces[n].next = r;
    s->pieces[r].prev = n;
    s->pieces[r].next = pnext;
  }

  return true;
}

bool
sequencedelete(struct sequence *s, size_t pos, size_t len)
{
  return false;
}

static size_t
findwordend(struct sequence *s, ssize_t p, size_t i)
{
  int32_t code, a;
  size_t end;
  
  end = 0;
  while (p != SEQ_end) {
    while (i < s->pieces[p].len) {
      a = utf8proc_iterate(s->data + s->pieces[p].off + i,
			   s->pieces[p].len - i, &code);
      if (a <= 0) {
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

  while (p != SEQ_start && in == -1) {
    i = 0;
    while (i < ii) {
      a = utf8proc_iterate(s->data + s->pieces[p].off + i,
			   s->pieces[p].len - i, &code);
      if (a <= 0) {
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
    ii = s->pieces[p].len;
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

  p = piecefind(s, pos, &i);
  if (p == -1) {
    return false;
  }

  start = findwordstart(s, p, i);
  end = findwordend(s, p, i);

  *begin = pos - start;
  *len = start + end;

  return true;
}

bool
sequencecopytobuf(struct sequence *s, size_t pos,
		  uint8_t *buf, size_t len)
{
  size_t i, l, b;
  ssize_t p;

  i = 0;
  for (p = SEQ_start; p != SEQ_end; p = s->pieces[p].next) {

    if (i > pos + len) {
      break;
    } else if (i + s->pieces[p].len < pos) {
      i += s->pieces[p].len;
      continue;
    }

    if (i < pos) {
      b = pos - i;
    } else {
      b = 0;
    }

    if (i + s->pieces[p].len > pos + len) {
      l = pos + len - i - b;
    } else {
      l = s->pieces[p].len - b;
    }

    memmove(buf + (i + b - pos),
	    s->data + s->pieces[p].off + b,
	    l);

    i += b + l;
  }
  
  return true;
}
