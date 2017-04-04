#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

bool
handletyping(uint8_t *s, size_t n)
{
  struct textbox *tb;
  struct tab *f;

  f = focus->norm.focus;

  switch (focustype) {
  case FOCUS_action:
    tb = &f->action;
    break;
  case FOCUS_main:
    tb = &f->main;
    break;
  default:
    return false;
  }
  
  if (textboxtyping(tb, s, n)) {
    panedraw(focus);
    return true;
  } else {
    return false;
  }
}

bool
handlekeypress(keycode_t k)
{
  struct textbox *tb;
  struct tab *f;

  f = focus->norm.focus;

  switch (focustype) {
  case FOCUS_action:
    tb = &f->action;
    break;
  case FOCUS_main:
    tb = &f->main;
    break;
  default:
    return false;
  }
  
  if (textboxkeypress(tb, k)) {
    panedraw(focus);
    return true;
  } else {
    return false;
  }
}

bool
handlekeyrelease(keycode_t k)
{
  struct textbox *tb;
  struct tab *f;

  f = focus->norm.focus;

  switch (focustype) {
  case FOCUS_action:
    tb = &f->action;
    break;
  case FOCUS_main:
    tb = &f->main;
    break;
  default:
    return false;
  }
  
  if (textboxkeyrelease(tb, k)) {
    panedraw(focus);
    return true;
  } else {
    return false;
  }
}
