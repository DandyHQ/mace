#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mace.h"

struct sequence *
sequencenew(struct mace *mace, uint8_t *data,
	    size_t len, size_t max)
{
  struct sequence *s;

  s = malloc(sizeof(struct sequence));
  if (s == NULL) {
    return NULL;
  }

  s->mace = mace;
  
  s->pmax = 10;
  s->pieces = malloc(sizeof(struct piece) * s->pmax);
  if (s->pieces == NULL) {
    return NULL;
  }

  s->plen = 3;

  s->pieces[SEQ_start].off = 0;
  s->pieces[SEQ_start].pos = 0;
  s->pieces[SEQ_start].len = 0;
  s->pieces[SEQ_start].prev = -1;
  s->pieces[SEQ_start].next = SEQ_end;

  s->pieces[SEQ_end].off = 0;
  s->pieces[SEQ_end].pos = 0;
  s->pieces[SEQ_end].len = 0;
  s->pieces[SEQ_end].prev = SEQ_start;
  s->pieces[SEQ_end].next = -1;
 
  if (data != NULL) {
    s->pieces[SEQ_first].off = 0;
    s->pieces[SEQ_first].pos = 0;
    s->pieces[SEQ_first].len = len;
    s->pieces[SEQ_first].prev = SEQ_start;
    s->pieces[SEQ_first].next = SEQ_end;

    s->pieces[SEQ_start].next = SEQ_first;
    s->pieces[SEQ_end].prev = SEQ_first;

    s->data = data;
    s->dlen = len;
    s->dmax = max;

  } else {
    s->dlen = 0;
    s->dmax = sysconf(_SC_PAGESIZE);

    s->data = mmap(NULL, s->dmax, PROT_READ|PROT_WRITE,
		   MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    if (s->data == MAP_FAILED) {
      sequencefree(s);
      return NULL;
    }
  }
 
  return s;
}

void
sequencefree(struct sequence *s)
{
  luaremove(s->mace->lua, s);
  
  free(s->pieces);

  if (s->data != NULL) {
    munmap(s->data, s->dmax);
  }

  free(s);
}

static void
shiftpieces(struct sequence *s, ssize_t p, size_t pos)
{
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
    if (s->pieces[p].pos + s->pieces[p].len >= pos) {
      *i = pos - s->pieces[p].pos;
      return p;
    }

    p = s->pieces[p].next;
  }

  return -1;
}

ssize_t
sequencefindpiece(struct sequence *s, size_t pos, size_t *i)
{
  return piecefind(s, SEQ_start, pos, i);
}

static ssize_t
pieceadd(struct sequence *s, size_t pos, size_t off, size_t len)
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

  s->pieces[s->plen].pos = pos;
  s->pieces[s->plen].off = off;
  s->pieces[s->plen].len = len;

  return s->plen++;
}

static bool
appenddata(struct sequence *s, const uint8_t *data, size_t len)
{
  uint8_t *ndata;
  size_t pg;

  pg = sysconf(_SC_PAGESIZE);

  while (s->dlen + len >= s->dmax) {
    ndata = mmap(s->data + s->dmax, pg, PROT_READ|PROT_WRITE,
		   MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);

    if (ndata == MAP_FAILED) {
      return false;
    } else {
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
  if (p != SEQ_start && p != SEQ_end
      && s->pieces[p].pos + s->pieces[p].len == pos
      && s->pieces[p].off + s->pieces[p].len == s->dlen) {

    if (!appenddata(s, data, len)) {
      return false;
    }

    s->pieces[p].len += len;

    shiftpieces(s, s->pieces[p].next, pos + len);
    
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

  n = pieceadd(s, pos, s->dlen, len);
  if (p == -1) {
    return false;
  }
  
  if (!appenddata(s, data, len)) {
    s->plen--; /* Free n */
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
		 s->pieces[p].off,
		 i);

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
  }

  shiftpieces(s, n, pos);

  return true;
}

/* TODO: this is broken. Can not delete first byte */

bool
sequencedelete(struct sequence *s, size_t pos, size_t len)
{
  ssize_t start, end, starti, endi, nstart, nend;
  ssize_t startprev, endnext;

  printf("sequence delete from %zu len %zu\n", pos, len);
  
  start = piecefind(s, SEQ_start, pos, &starti);
  if (start == -1) {
    return false;
  }

  end = piecefind(s, start, pos + len, &endi);
  if (end == -1) {
    return false;
  }

  printf("so start is in %zu,%zu and end in %zu,%zu\n", start, starti, end, endi);
  
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

  startprev = s->pieces[start].prev;
  endnext   = s->pieces[end].next;

  if (startprev != -1) {
    s->pieces[startprev].next = nstart;
  }
  
  s->pieces[nstart].prev = startprev;

  s->pieces[nstart].next = nend;
  s->pieces[nend].prev = nstart;

  s->pieces[nend].next = endnext;

  if (endnext != -1) {
    s->pieces[endnext].prev = nend;
  }

  shiftpieces(s, nend, pos);
  
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

  while (p != -1 && in == -1) {
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
  size_t p;

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
  size_t i, l, b, p, ret;

  i = 0;
  for (p = SEQ_start; p != -1; p = s->pieces[p].next) {

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

    if (i + s->pieces[p].len - pos >= len) {
      l = pos + len - i - b;
    } else {
      l = s->pieces[p].len - b;
    }

    memmove(buf + (i + b - pos),
	    s->data + s->pieces[p].off + b,
	    l);

    i += b + l;
  }

  if (i <= pos) {
    ret = 0;
    i = pos;
  } else {
    ret = i - pos;
  }

  memset(buf + (i - pos), 0, len + 1 - (i - pos));

  return ret;
}
