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

struct colour fg = { 0, 0, 0, 255 };
struct colour bg = { 255, 255, 255, 255 };

struct colour abg = { 220, 240, 255, 255 };

struct colour sfg = { 0, 0, 0, 255 };
struct colour sbg = { 200, 150, 100, 255 };

int lineheight;
int baseline;

int tabwidth = 80;

uint8_t *buf = NULL;
unsigned int width = 0;
unsigned int height = 0;

struct pane *root = NULL;

struct pane *focuspane = NULL;
struct textbox *focus = NULL;

struct selection *selections = NULL;
/* Current selection being made. */
struct selection *cselection = NULL;

void
init(void)
{
  struct tab *t;

  fontinit();

  t = tabnew((uint8_t *) "Mace");
  if (t == NULL) {
    errx(1, "Failed to allocate root tab");
  }

  focus = &t->main;

  focuspane = root = panenew(NULL, t);
  if (root == NULL) {
    errx(1, "Failed to allocate root pane");
  }

  luainit();
}
