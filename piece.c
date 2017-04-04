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
findpiece(struct piece *p, int pos, int *i)
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
