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
piecenew(uint8_t *s, size_t rl, size_t pl)
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
  size_t len;
  uint8_t *s;

  len = min(PIECE_min, pos);
  s = malloc(len);
  if (s == NULL) {
    return false;
  }

  *lr = piecenew(s, len, pos);
  if (*lr == NULL) {
    free(s);
    return NULL;
  }

  len = min(PIECE_min, p->pl - pos);
  s = malloc(len);
  if (s == NULL) {
    return false;
  }

  *rr = piecenew(s, len, p->pl - pos);
  if (*rr == NULL) {
    free(s);
    piecefree(*lr);
    return false;
  }

  memmove((*lr)->s, p->s, pos);
  memset((*lr)->s + pos, 0, (*lr)->rl - pos);
 
  memmove((*rr)->s, p->s + pos, p->pl - pos);
  memset((*rr)->s + (p->pl - pos), 0, (*rr)->rl - (p->pl - pos));

  printf("split piece rl %i, pl %i, s = '%s'\n", p->rl, p->pl, p->s);
  printf("into right  rl %i, pl %i, s = '%s'\n", (*lr)->rl, (*lr)->pl, (*lr)->s);
  printf("and left    rl %i, pl %i, s = '%s'\n", (*rr)->rl, (*rr)->pl, (*rr)->s);
  
  return true;
}

struct piece *
pieceinsert(struct piece *old, size_t pos,
	    uint8_t *s, size_t rl, size_t pl)
{
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
    for (i = 0; i < p->pl && p->s[i] != 0; i += a) {
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
  
    p = p->next;
    *pos += i;
  }

  *pos = yy;
  return NULL;
}

