#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sequence.h"
#include "utf8.h"

/* This file is ugly and needs to be redone properly. */

struct sequence *
sequencenew(uint8_t *data, size_t len)
{
  struct sequence *s;

  s = calloc(1, sizeof(struct sequence));
  if (s == NULL) {
    return NULL;
  }

  s->pmax = 10;
  s->pieces = malloc(sizeof(struct piece) * s->pmax);
  if (s->pieces == NULL) {
		free(s);
    return NULL;
  }

  s->plen = 3;

  /* This is pretty ugly. */
  
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

    s->pieces[SEQ_end].pos = len;

    s->data = data;
    s->dlen = len;
    s->dmax = len;

  } else {
    s->dlen = 0;
    s->dmax = 1024;

    s->data = malloc(s->dmax);
    if (s->data == NULL) {
    	free(s->pieces);
      free(s);
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

#define PIECE_inc 10

static ssize_t
pieceadd(struct sequence *s, size_t pos, size_t off, size_t len)
{
	if (s->plen + 1 >= s->pmax) {
		s->pieces = realloc(s->pieces,
		                    sizeof(struct piece) * 
		                    (s->plen + PIECE_inc));

		if (s->pieces == NULL) {
			/* What should happen here? It is fucked. */
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

#define DATA_inc 1024

static bool
appenddata(struct sequence *s, const uint8_t *data, size_t len)
{
  if (s->dlen + len >= s->dmax) {
  	if (s->data == NULL) {
  		s->data = malloc(len + DATA_inc);
  	} else {
	    s->data = realloc(s->data, s->dmax + len + DATA_inc);
		}
		
    if (s->data == NULL) {
    	/* If this happens we're kinda fucked, we just 
    	   lost all the data. Not sure how this should be
    	   handled. */
    	s->dmax = 0;
    	s->dlen = 0;
      return false;
      
    } else {
      s->dmax += len + DATA_inc;
    }
  }

  memmove(s->data + s->dlen, data, len);

  s->dlen += len;

  return true;
}

bool
sequencereplace(struct sequence *s, 
                size_t begin, size_t end,
	              const uint8_t *data, size_t len)
{
  size_t bi, ei, newoff;
  ssize_t b, e, n, p;
  
  newoff = s->dlen;  
  if (len > 0 && !appenddata(s, data, len)) {
  	return false;
  }
  
  b = piecefind(s, SEQ_start, begin, &bi);
  if (b == -1) {
  	return false;
  }
  
  /* Are we just inserting and is this the last piece added?
     If so then grow the piece and be done with it. */
 
  if (begin == end && len > 0 && 
	    b != SEQ_start && b != SEQ_end &&
	    s->pieces[b].pos + s->pieces[b].len == begin &&
      s->pieces[b].off + s->pieces[b].len == newoff) {
  	
  	s->pieces[b].len += len;
      
		shiftpieces(s, b, s->pieces[b].pos);
      
		return true;
  }
  
  /* Otherwise we have to do it this way. */
  
  e = piecefind(s, b, end, &ei);
  if (e == -1) {
  	return false;
  }
  
  while (ei == s->pieces[e].len && e != SEQ_end) {
  	e = s->pieces[e].next;
  	ei = 0;
  }
  
  /* Need a new begining piece. */
  if (bi < s->pieces[b].len) {
  	n = pieceadd(s, s->pieces[b].pos, 
  	             s->pieces[b].off, bi);
  	             
  	if (n == -1) {
  		return false;
  	}
  	
  	s->pieces[n].prev = s->pieces[b].prev;
  	s->pieces[s->pieces[n].prev].next = n;
  	b = n;
  }
  
  /* Need a middle piece */
  if (len > 0) {
		n = pieceadd(s, begin, newoff, len);
		if (n == -1) {
			return false;
		}
		
		s->pieces[b].next = n;
		s->pieces[n].prev = b;
		p = n;
		
	} else {
		p = b;
	}

	/* Need a new end piece. */
  if (ei > 0) {
  	n = pieceadd(s, s->pieces[e].pos + ei, 
  	             s->pieces[e].off + ei, 
  	             s->pieces[e].len - ei);
  	             
  	if (n == -1) {
  		return false;
  	}

		s->pieces[n].next = s->pieces[e].next;
  	e = n;
  }
  
	s->pieces[p].next = e;
	s->pieces[e].prev = p;
	
	shiftpieces(s, b, s->pieces[b].pos);
	
  return true;
}

size_t
sequencecodepoint(struct sequence *s, size_t pos, int32_t *code)
{
	ssize_t p;
	size_t i, l;

	p = piecefind(s, SEQ_start, pos, &i);
	if (p == -1) {
		return 0;
	}

	while (s->pieces[p].len == i) {
		p = s->pieces[p].next;
		if (p == -1) {
			return 0;
		}

		i = 0;
	}

	l = utf8iterate(s->data + s->pieces[p].off + i, 
	                s->pieces[p].len - i, code);
		
	return l;
}

size_t
sequenceprevcodepoint(struct sequence *s, size_t pos, int32_t *code)
{
	ssize_t p;
	size_t i;

	p = piecefind(s, SEQ_start, pos, &i);
	if (p == -1) {
		return 0;
	}

	while (i == 0) {
		p = s->pieces[p].prev;
		if (p == -1) {
			return 0;
		}

		i = s->pieces[p].len;
	}

	return utf8deiterate(s->data + s->pieces[p].off, 
	                     s->pieces[p].len, i, code);
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
  	l = s->pieces[p].len - i;
    if (ii + l >= len) {
      l = len - ii;
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

static size_t
findwordstart(struct sequence *s, size_t i)
{
	int32_t code;
	size_t a;
	
	while (i > 0 && (a = sequenceprevcodepoint(s, i, &code)) > 0) {
		if (iswordbreak(code) || islinebreak(code) || iswhitespace(code)) {
			return i;
		} else {
			i -= a;
		}
	}
	
	return i;
}

static size_t
findwordend(struct sequence *s, size_t i)
{
	int32_t code;
	size_t a;
	
	while ((a = sequencecodepoint(s, i, &code)) > 0) {
		if (iswordbreak(code) || islinebreak(code) || iswhitespace(code)) {
			return i;
		} else {
			i += a;
		}
	}
	
	return i;
}

bool
sequencefindword(struct sequence *s, size_t pos,
                 size_t *begin, size_t *len)
{
	if (pos >= sequencelen(s)) {
		return false;
	}
	
  *begin = findwordstart(s, pos);
  *len = findwordend(s, pos) - *begin;

  return *len > 0;
}