#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

struct tab *
tabnew(uint8_t *name)
{
  uint8_t s[128];
  struct tab *t;
  size_t n;

  t = malloc(sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }

  if (!textboxinit(&t->action, t, &abg, true)) {
    free(t);
    return NULL;
  }

  if (!textboxinit(&t->main, t, &bg, false)) {
    textboxfree(&t->action);
    free(t);
    return NULL;
  }
  
  n = snprintf((char *) s, sizeof(s),
	       "%s save cut copy paste search", name);

  if (pieceinsert(t->action.pieces->next, 0, s, n) == NULL) {
    textboxfree(&t->main);
    textboxfree(&t->action);
    free(t);
    return NULL;
  }

  t->action.cursor = n;
  
  t->next = NULL;

  strlcpy((char *) t->name, (char *) name, NAMEMAX);

  return t;
} 

void
tabfree(struct tab *t)
{
  textboxfree(&t->action);
  textboxfree(&t->main);
  free(t);
}

