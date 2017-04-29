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

static struct colour bg   = { 1, 0, 1 };
static struct colour abg  = { 0.5, 0.5, 0.8 };

struct tab *
tabnew(uint8_t *name, size_t len)
{
  uint8_t s[128] = ": save cut copy paste search";
  struct piece *p;
  struct tab *t;

  t = malloc(sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }

  t->name = malloc(len + 1);
  if (t->name == NULL) {
    free(t);
    return NULL;
  }

  strncpy(t->name, name, len);
  t->name[len] = 0;
  
  t->action = textboxnew(t, &abg);
  if (t->action == NULL) {
    free(t->name);
    free(t);
    return NULL;
  }

  t->main = textboxnew(t, &bg);
  if (t->main == NULL) {
    textboxfree(t->action);
    free(t->name);
    free(t);
    return NULL;
  }

  p = pieceinsert(t->action->pieces->next, 0, name, len);
  if (p == NULL) {
    textboxfree(t->main);
    textboxfree(t->action);
    free(t->name);
    free(t);
    return NULL;
  }    
  
  if (pieceinsert(p, len, s, strlen(s)) == NULL) {
    textboxfree(t->main);
    textboxfree(t->action);
    free(t->name);
    free(t);
    return NULL;
  }

  t->action->cursor = len + strlen(s);

  t->next = NULL;

  return t;
} 

void
tabfree(struct tab *t)
{
  textboxfree(t->action);
  textboxfree(t->main);
  free(t->name);
  free(t);
}

bool
tabresize(struct tab *t, int x, int y, int w, int h)
{
  t->x = x;
  t->y = y;
  t->width = w;
  t->height = h;
  
  if (!textboxresize(t->action, w)) {
    return false;
  }

  if (!textboxresize(t->main, w)) {
    return false;
  }

  return true;
}

void
tabscroll(struct tab *t, int x, int y, int dx, int dy)
{

}

void
tabbuttonpress(struct tab *t, int x, int y,
	 unsigned int button)
{
  if (y < t->action->height) {
    focus = t->action;
    return textboxbuttonpress(t->action, x, y, button);
  } else {
    focus = t->main;
    return textboxbuttonpress(t->main, x, y - t->action->height,
			      button);
  }
}

void
tabbuttonrelease(struct tab *t, int x, int y,
	   unsigned int button)
{
  if (focus == t->action) {
    return textboxbuttonrelease(t->action, x, y, button);
  } else {
    return textboxbuttonrelease(t->main, x, y - t->action->height,
				button);
  }
}

void
tabmotion(struct tab *t, int x, int y)
{
  if (y < t->action->height) {
    return textboxmotion(t->action, x, y);
  } else {
    return textboxmotion(t->main, x, y - t->action->height);
  }
}

void
tabdraw(struct tab *t)
{
  int y, h;
  
  y = 0;

  h = t->height - y;
  if (t->action->height < h) {
    h = t->action->height;
  }
  
  cairo_set_source_surface(cr, t->action->sfc, t->x, t->y + y);
  cairo_rectangle(cr, t->x, t->y + y, t->width, h);
  cairo_fill(cr);

  y += h;

  h = t->height - y;
  if (h == 0) {
    return;
  } else if (t->main->height < h) {
    h = t->main->height;
  }

  cairo_set_source_surface(cr, t->main->sfc, t->x, t->y + y);
  cairo_rectangle(cr, t->x, t->y + y, t->width, h);
  cairo_fill(cr);

  y += h;
  h = t->height - y;
  if (h == 0) {
    return;
  } 

  cairo_set_source_rgb(cr, 
		       t->main->bg.r,
		       t->main->bg.g,
		       t->main->bg.b);

  cairo_rectangle(cr, t->x, t->y + y, t->width, h);
  cairo_fill(cr);
}
