#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

struct piece *
piecenewgive(uint8_t *s, size_t rl, size_t pl)
{
  struct piece *p;

  p = malloc(sizeof(struct piece));
  if (p == NULL) {
    return NULL;
  }

  p->rl = rl;
  p->pl = pl;
  p->s = s;

  p->prev = NULL;
  p->next = NULL;
  
  return p;
}

struct piece *
piecenewcopy(uint8_t *s, size_t l)
{
  struct piece *p;
  
  p = malloc(sizeof(struct piece));
  if (p == NULL) {
    return NULL;
  }

  p->rl = max(PIECE_min, l);
  p->s = malloc(p->rl);
  if (p->s == NULL) {
    free(p);
    return NULL;
  }

  memmove(p->s, s, l);
  memset(p->s + l, 0, p->rl - l);

  p->pl = l;

  p->prev = NULL;
  p->next = NULL;
  
  return p;
}

struct piece *
piecenewtag(void)
{
  struct piece *p;
  
  p = malloc(sizeof(struct piece));
  if (p == NULL) {
    return NULL;
  }

  p->rl = 0;
  p->pl = 0;
  p->s = NULL;

  p->prev = NULL;
  p->next = NULL;
  
  return p;
}

void
piecefree(struct piece *p)
{
  free(p->s);
  free(p);
}

bool
piecesplit(struct piece *p, size_t pos,
	   struct piece **lr, struct piece **rr)
{
  *lr = piecenewcopy(p->s, pos);
  if (*lr == NULL) {
    return NULL;
  }

  *rr = piecenewcopy(p->s + pos, p->pl - pos);
  if (*rr == NULL) {
    piecefree(*lr);
    return false;
  }
 
  return true;
}

struct piece *
pieceinsert(struct piece *old, size_t pos,
	    uint8_t *s, size_t len)
{
  struct piece *ll, *rr, *l, *r, *n;

  ll = old->prev;
  rr = old->next;
  
  n = piecenewcopy(s, len);
  if (n == NULL) {
    return false;
  }

  if (pos == 0) {
    n->next = old;
    old->prev = n;
    l = n;
    r = old;
  } else if (pos == old->pl) {
    old->next = n;
    n->prev = old;
    l = old;
    r = n;
  } else {
    if (!piecesplit(old, pos, &l, &r)) {
      piecefree(n);
      return false;
    }

    l->next = n;
    n->prev = l;
    n->next = r;
    r->prev = n;
  }

  if (ll != NULL) {
    ll->next = l;
  }
  
  l->prev = ll;

  r->next = rr;
  if (rr != NULL) {
    rr->prev = r;
  }

  return n;
}

struct piece *
piecefind(struct piece *p, int pos, int *i)
{
  int pi;

  pi = 0;
  while (p != NULL) {
    if (pi + p->pl == pos) {
      *i = 0;
      return p->next;
    } else if (pi + p->pl > pos) {
      *i = pos - pi;
      return p;
    } else {
      pi += p->pl;
      p = p->next;
    }
  }

  return NULL;
}

bool
piecefindword(struct piece *p, int pos,
	      int *start, int *end)
{
  int32_t code, a, preva;
  int i, cpos;
  bool found;

  cpos = 0;
  preva = 0;
  *start = 0;
  found = false;

  while (p != NULL) {
    for (i = 0; i < p->pl; preva = a, cpos += a, i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      if (cpos == pos) {
	found = true;
      }
 
      /* Line Break. */
      if (iswordbreak(code)
	  || islinebreak(code, p->s + i, p->pl - i, &a)) {

	if (found) {
	  if (cpos == pos) {
	    return false;
	  } else {
	    *end = cpos - preva;
	    return true;
	  }
	} else {
	  *start = cpos + a;
	}
      }
    }

    p = p->next;
    if (p == NULL || p->s == NULL) {
      break;
    }
  }

  if (found) {
    *end = cpos - preva;
    return true;
  } else {
    return false;
  }
}

bool
pieceappend(struct piece *p, uint8_t *s, size_t l)
{
  uint8_t *ns;
  size_t nl;
  
  if (p->rl - p->pl < l) {
    nl = p->rl + l + PIECE_min;

    ns = realloc(p->s, nl * sizeof(uint8_t));
    if (ns == NULL) {
      return false;
    } else {
      p->s = ns;
      p->rl = nl;
    }
  }

  memmove(p->s + p->pl, s, l);
  p->pl += l;

  return true;
}
