#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
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
  
  printf("insert '%s' into '%s' at pos %i\n", s, old->s, pos);
  
  n = piecenewcopy(s, len);
  if (n == NULL) {
    return false;
  }

  printf("copied piece '%s'\n", n->s);
  
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

    printf("split        %i : '%s'\n", old->pl, old->s);
    printf("into left    %i : '%s'\n", l->pl, l->s);
    printf("into middle  %i : '%s'\n", n->pl, n->s);
    printf("into right   %i : '%s'\n", r->pl, r->s);
    
    l->next = n;
    n->prev = l;
    n->next = r;
    r->prev = n;
  }

  if (ll != NULL) {
    printf("update left\n");
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

  printf("find pos %i in string of pieces\n", pos);
  
  pi = 0;
  while (p != NULL) {
    if (pi + p->pl == pos) {
      *i = 0;
      return p->next;
    } else if (pi + p->pl > pos) {
      printf("will be inside '%s' which starts at %i and has lenght %i at index %i\n", p->s, pi, p->pl, pos - pi);
      *i = pos - pi;
      return p;
    } else {
      pi += p->pl;
      p = p->next;
    }
  }

  return NULL;
}

struct piece *
findpos(struct piece *p,
	int x, int y, int linewidth,
	int *pos)
{
  const utf8proc_property_t *props;
  int i, xx, yy, ww;
  int32_t code;
  ssize_t a;

  xx = 0;
  yy = 0;

  *pos = 0;
  
  while (p != NULL) {
    if (p->s == NULL) {
      p = p->next;
      continue;
    }
    
    for (i = 0; i < p->pl; i += a) {
      a = utf8proc_iterate(p->s + i, p->pl - i, &code);
      if (a <= 0) {
	a = 1;
	continue;
      }

      props = utf8proc_get_property(code);
      if (props->boundclass == UTF8PROC_BOUNDCLASS_LF) {
	printf("line feed?\n");
	xx = 0;
	yy += lineheight;
	continue;
      }

      if (props->boundclass == UTF8PROC_BOUNDCLASS_CR) {
	printf("carrage return?\n");
 	xx = 0;
	yy += lineheight;
	continue;
      }
      
      if (!loadglyph(code)) {
	continue;
      }

      ww = face->glyph->advance.x >> 6;

      if (xx + ww >= linewidth) {
	if (yy < y && y <= yy + lineheight) {
	  *pos += i;
	  return p;
	} else {
	  yy += lineheight;
	  xx = 0;
	}
      }

      if (yy < y && y <= yy + lineheight && xx < x && x <= xx + ww) {
	*pos += i;
	return p;
      } else {
	xx += ww;
      }
    }
  
    *pos += i;
    if ((p->next == NULL || p->next->s == NULL)
	&& yy < y && y <= yy + lineheight) {
      return p;
    } else {
      p = p->next;
    }
  }

  *pos = yy;
  return NULL;
}

void
piecelistprint(struct piece *p)
{
  printf("%s", p->s);
  if (p->next != NULL) {
    piecelistprint(p->next);
  }
}
