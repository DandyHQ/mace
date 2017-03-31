#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

FT_Face face;

int fontsize;

struct colour bg = { 255, 255, 255, 255 };
struct colour fg = { 0, 0, 0, 255 };
struct colour abg = { 220, 240, 255, 255 };

int lineheight;
int baseline;

int tabwidth = 80;

uint8_t *buf = NULL;
unsigned int width = 0;
unsigned int height = 0;

struct pane *root = NULL;

struct pane *focus = NULL;
focus_t focustype = FOCUS_main;

void
init(void)
{
  struct tab *t;

  fontinit();

  t = tabnew((uint8_t *) "Mace");
  if (t == NULL) {
    err(1, "Failed to allocate root tab!\n");
  }

  focus = root = panenew(NULL, t);
  if (root == NULL) {
    err(1, "Failed to allocate root pane!\n");
  }

  luainit();
}
