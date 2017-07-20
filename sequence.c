#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sequence.h"
#include "utf8.h"

#define PIECE_inc 10
#define CHANGE_inc 10
#define DATA_inc 1024

/* Much of this file is horrifically ugly. */

struct sequence *
sequencenew(uint8_t *data, size_t len)
{
  struct sequence *s;

  s = calloc(1, sizeof(struct sequence));
  if (s == NULL) {
    return NULL;
  }

	s->cmax = CHANGE_inc;
	s->changes = malloc(sizeof(struct change) * s->cmax);
	if (s->changes == NULL) {
		free(s);
		return NULL;
	}
	
	s->changehead = CHANGE_root;
	s->clen = 1;
	
	s->changes[CHANGE_root].prev = SEQ_start;
	s->changes[CHANGE_root].next = SEQ_end;
	s->changes[CHANGE_root].children = -1;
	s->changes[CHANGE_root].sibling = -1;
	s->changes[CHANGE_root].parent = -1;
	s->changes[CHANGE_root].napieces = 0;
	s->changes[CHANGE_root].nrpieces = 0;
	
  s->pmax = PIECE_inc;
  s->pieces = malloc(sizeof(struct piece) * s->pmax);
  if (s->pieces == NULL) {
  	free(s->changes);
		free(s);
    return NULL;
  }
  
  s->plen = 2;

  s->pieces[SEQ_start].off = 0;
  s->pieces[SEQ_start].pos = 0;
  s->pieces[SEQ_start].len = 0;
  s->pieces[SEQ_start].prev = SEQ_start;
  s->pieces[SEQ_start].next = SEQ_end;

  s->pieces[SEQ_end].off = 0;
  s->pieces[SEQ_end].pos = 0;
  s->pieces[SEQ_end].len = 0;
  s->pieces[SEQ_end].prev = SEQ_start;
  s->pieces[SEQ_end].next = SEQ_end;

  if (data != NULL) {
    s->pieces[s->plen].off = 0;
    s->pieces[s->plen].pos = 0;
    s->pieces[s->plen].len = len;
    s->pieces[s->plen].prev = SEQ_start;
    s->pieces[s->plen].next = SEQ_end;

    s->pieces[SEQ_start].next = s->plen;
    s->pieces[SEQ_end].prev = s->plen;

    s->pieces[SEQ_end].pos = len;

    s->data = data;
    s->dlen = len;
    s->dmax = len;
    
  	s->changes[CHANGE_root].apieces[s->changes[CHANGE_root].napieces++]
  	  = s->plen;
  	
  	s->plen++;
  	
  } else {
    s->dlen = 0;
    s->dmax = 1024;

    s->data = malloc(s->dmax);
    if (s->data == NULL) {
    	free(s->changes);
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
  
  while (p != SEQ_end) {
    s->pieces[p].pos = pos;
    pos += s->pieces[p].len;
    p = s->pieces[p].next;
  }
  
  s->pieces[SEQ_end].pos = pos;
}

static ssize_t
piecefind(struct sequence *s, ssize_t p, size_t pos, size_t *i)
{
  while (p != SEQ_end) {
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
piecenew(struct sequence *s, size_t pos, size_t off, size_t len)
{
	if (s->plen + 1 >= s->pmax) {
		s->pieces = realloc(s->pieces,
		                    sizeof(struct piece) * 
		                    (s->pmax + PIECE_inc));

		if (s->pieces == NULL) {
			/* What should happen here? It is fucked. */
			s->pmax = 0;
			s->plen = 0;
			return -1;
		}

		s->pmax += PIECE_inc;
	}

	s->pieces[s->plen].pos = pos;
	s->pieces[s->plen].off = off;
	s->pieces[s->plen].len = len;

	return s->plen++;
}

static ssize_t
changenew(struct sequence *s)
{
	if (s->clen + 1 >= s->cmax) {
		s->changes = realloc(s->changes,
		                     sizeof(struct change) * 
		                     (s->cmax + CHANGE_inc));

		if (s->changes == NULL) {
			/* What should happen here? It is fucked. */
			s->cmax = 0;
			s->clen = 0;
			return -1;
		}

		s->cmax += CHANGE_inc;
	}

	memset(&s->changes[s->clen], 0, sizeof(struct change));
	return s->clen++;
}

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

/*
static void
printchangetree(struct sequence *s, char *h, ssize_t c)
{
	char buf[512];
	size_t i;
	
	if (c == -1)
		return;
	
	if (s->changehead == c) {
		printf("%schange HEAD ", h);
	} else {
		printf("%schange ", h);
	}
	
	printf("%zi has added %zu pieces: ", c, s->changes[c].napieces);
	for (i = 0; i < s->changes[c].napieces; i++)
		printf("%zi ", s->changes[c].apieces[i]);
	
	printf("and removed %zu pieces: ", s->changes[c].nrpieces);
	for (i = 0; i < s->changes[c].nrpieces; i++)
		printf("%zi ", s->changes[c].rpieces[i]);
		
	printf("\n");
	
	snprintf(buf, sizeof(buf), "%s    ", h);
	printchangetree(s, buf, s->changes[c].children);
	
	printchangetree(s, h, s->changes[c].sibling);
}

static void
printsequence(struct sequence *s)
{
	ssize_t p;
	
	printf("[START]");
	
	for (p = s->pieces[SEQ_start].next; p != SEQ_end; p = s->pieces[p].next) {
		printf("[%zi len %zu at %zu]", p, s->pieces[p].len, s->pieces[p].pos);
	}
	
	printf("[END at %zu]\n", s->pieces[SEQ_end].pos);
}
*/

bool
sequencereplace(struct sequence *s, 
                size_t begin, size_t end,
	              const uint8_t *data, size_t len)
{
  size_t bi, ei, newoff;
  ssize_t b, e, n, p, c;
  
  if (len == 0 && begin == end) {
  	/* What are you trying to do? */
  	return false;
  }
  
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
 
 /*
  if (begin == end && len > 0 && 
	    b != SEQ_start && b != SEQ_end &&
	    s->pieces[b].pos + s->pieces[b].len == begin &&
      s->pieces[b].off + s->pieces[b].len == newoff) {
  	
  	s->pieces[b].len += len;
      
		shiftpieces(s, b, s->pieces[b].pos);
      
		return true;
  }
  */
  
  /* Otherwise we have to do it this way. */
  
  c = changenew(s);
  if (c == -1) {
  	return false;
  }
  
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
  	n = piecenew(s, s->pieces[b].pos, 
  	             s->pieces[b].off, bi);
  	             
  	if (n == -1) {
  		return false;
  	}
  	
  	s->pieces[n].prev = s->pieces[b].prev;
  	s->pieces[s->pieces[n].prev].next = n;
  	
  	s->changes[c].rpieces[s->changes[c].nrpieces++] = b;
  	s->changes[c].apieces[s->changes[c].napieces++] = n;
  	s->changes[c].prev = s->pieces[b].prev;
  	p = n;
  	
  } else {
  	s->changes[c].prev = b;
  	p = b;
  }
  
  /* Need a middle piece */
  if (len > 0) {
		n = piecenew(s, begin, newoff, len);
		if (n == -1) {
			return false;
		}
		
		s->pieces[p].next = n;
		s->pieces[n].prev = p;
		p = n;
		
  	s->changes[c].apieces[s->changes[c].napieces++] = n;

	} /* Else leave p as is. */
	
	/* Need a new end piece. */
  if (ei > 0) {
  	n = piecenew(s, s->pieces[e].pos + ei, 
  	             s->pieces[e].off + ei, 
  	             s->pieces[e].len - ei);
  	             
  	if (n == -1) {
  		return false;
  	}

		s->pieces[n].next = s->pieces[e].next;
		s->pieces[s->pieces[n].next].prev = n;
  	
  	/* Do not add it as removed twice! */
  	if (e != b) {
  		s->changes[c].rpieces[s->changes[c].nrpieces++] = e;
  	}
  	
  	s->changes[c].apieces[s->changes[c].napieces++] = n;
  	s->changes[c].next = s->pieces[e].next;
  	e = n;
  	
  } else {
  	s->changes[c].next = e;
  }
  
	s->pieces[p].next = e;
	s->pieces[e].prev = p;
	
	shiftpieces(s, SEQ_start, 0);
	
	if (s->changes[c].napieces == 0) {
		/* Umm? Don't add the change to the tree. */
		printf("somehow the change added no pieces! begin = %zu, end = %zu, len = %zu\n",
		       begin, end, len);
		return false;
	}
	
	s->changes[c].children = -1;
	s->changes[c].parent = s->changehead;
	
	s->changes[c].sibling = s->changes[s->changehead].children;
	s->changes[s->changehead].children = c;
	
	s->changehead = c;
	
	/*
	printchangetree(s, "", CHANGE_root);
	printsequence(s);
	*/
	
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
		if (p == SEQ_end) {
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
		if (p == SEQ_start) {
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
  while (p != SEQ_end && ii < len) {
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

static void
undochange(struct sequence *s, ssize_t c)
{
	ssize_t p, pp, prev;
	
	/* Add removed pieces back in. */
	
	prev = s->changes[c].prev;
	for (p = 0; p < s->changes[c].nrpieces; p++) {
		pp = s->changes[c].rpieces[p];
		s->pieces[pp].prev = prev;
		s->pieces[prev].next = pp;
		prev = pp;
	}
	
	s->pieces[prev].next = s->changes[c].next;
	s->pieces[s->changes[c].next].prev = prev;
	
	s->changehead = s->changes[c].parent;
	
	shiftpieces(s, SEQ_start, 0);
}

static void
redochange(struct sequence *s, ssize_t c)
{
	ssize_t p, pp, prev;
	
	/* Add removed pieces back in. */
	
	prev = s->changes[c].prev;
	for (p = 0; p < s->changes[c].napieces; p++) {
		pp = s->changes[c].apieces[p];
		s->pieces[pp].prev = prev;
		s->pieces[prev].next = pp;
		prev = pp;
	}
	
	s->pieces[prev].next = s->changes[c].next;
	s->pieces[s->changes[c].next].prev = prev;
	
	s->changehead = c;
	
	shiftpieces(s, SEQ_start, 0);
}

bool
sequencechangeup(struct sequence *s)
{
	ssize_t c;
	
	c = s->changehead;
	if (c == CHANGE_root) {
		return false;
	}
	
	undochange(s, c);
	
	return true;
}

bool
sequencechangedown(struct sequence *s)
{
	ssize_t c;
	
	c = s->changes[s->changehead].children;
	if (c == -1) {
		return false;
	}
	
	redochange(s, c);
	
	return true;
}

bool
sequencechangecycle(struct sequence *s)
{
	ssize_t c, n, p;
	
	c = s->changehead;
	if (c == CHANGE_root) {
		return false;
	}
	
	n = s->changes[c].sibling;
	if (n == -1) {
		return false;
	}
	
	undochange(s, c);
	redochange(s, n);
	
	s->changes[s->changes[n].parent].children = n;
	
	/* Move c to the end of the sibling list. */
	
	while (n != -1) {
		p = n;
		n = s->changes[p].sibling;
	}
	
	s->changes[p].sibling = c;
	s->changes[c].sibling = -1;
	
	return true;
}

