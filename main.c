#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

unsigned int width = 0;
unsigned int height = 0;
unsigned char *buf = NULL;

struct pane *root = NULL;

void
init(void)
{
  struct tab *t;
  
  fontinit();

  t = tabnew("Mace");
  if (t == NULL) {
    err(1, "Failed to allocate root tab!\n");
  }
  
  root = panenew(NULL, t);
  if (root == NULL) {
    err(1, "Failed to allocate root pane!\n");
  }
}

void
resize(char *nbuf, int w, int h)
{
  printf("resize to %ix%i\n", (int) w, (int) h);

  buf = nbuf;
  width = w;
  height = h;

  resizepane(root, 0, 0, w, h);
  
  drawpane(root);
}
