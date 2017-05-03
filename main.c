#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

int width, height;
cairo_t *cr;

FT_Face face;
int baseline, lineheight;

struct tab *tab;
struct textbox *focus;
struct selection *selections;

void
init(void)
{
  uint8_t name[] = "Mace";
  
  selections = NULL;
  
  tab = tabnew(name, strlen(name));
  if (tab == NULL) {
    errx(1, "Failed to create main tab!");
  }
  
  focus = tab->main;
}
