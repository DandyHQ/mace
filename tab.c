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

#define SCROLL_WIDTH   10

static struct colour bg   = { 1, 1, 1 };
static struct colour abg  = { 0.86, 0.94, 1 };

struct tab *
tabnew(uint8_t *name, size_t len)
{
  uint8_t s[128] = ": save cut copy paste search";
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

  if (!sequenceinsert(t->action->sequence, 0, name, len)) {
    textboxfree(t->action);
    free(t->name);
    free(t);
    return NULL;
  }    

  if (!sequenceinsert(t->action->sequence, len, s, strlen(s))) {
    textboxfree(t->action);
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

  t->action->cursor = len + strlen(s);

  return t;
} 

void
tabfree(struct tab *t)
{
  luafree(t);
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

  if (!textboxresize(t->main, w - SCROLL_WIDTH)) {
    return false;
  }

  return true;
}

void
tabscroll(struct tab *t, int x, int y, int dy)
{
  if (y < t->action->height) {
    /* Do nothing */
  } else {
    textboxscroll(t->main, x, y - t->action->height - 1, dy);
  }
}

void
tabbuttonpress(struct tab *t, int x, int y,
	       unsigned int button)
{
  if (y < t->action->height) {
    focus = t->action;
    textboxbuttonpress(t->action, x, y, button);
  } else {
    focus = t->main;
    textboxbuttonpress(t->main, x, y - t->action->height - 1,
			      button);
  }
}

void
tabbuttonrelease(struct tab *t, int x, int y,
	   unsigned int button)
{
  if (focus == t->action) {
    textboxbuttonrelease(t->action, x, y, button);
  } else {
    textboxbuttonrelease(t->main, x, y - t->action->height - 1,
				button);
  }
}

void
tabmotion(struct tab *t, int x, int y)
{
  if (focus == t->action) {
    textboxmotion(t->action, x, y);
  } else {
    textboxmotion(t->main, x, y - t->action->height - 1);
  }
}

static int
tabdrawaction(struct tab *t, int y)
{
  int h;
  
  h = t->height - y;
  if (t->action->height < h) {
    h = t->action->height;
  }
  
  cairo_set_source_surface(cr, t->action->sfc, t->x, t->y + y);
  cairo_rectangle(cr, t->x, t->y + y, t->width, h);
  cairo_fill(cr);

  return y + h;
}

static int
tabdrawmain(struct tab *t, int y)
{
  int h, pos, size;
  
  h = t->height - y;
  if (h == 0) {
    return y;
  } else if (t->main->height - t->main->yoff < h) {
    h = t->main->height - t->main->yoff;
  }

  cairo_set_source_surface(cr, t->main->sfc,
			   t->x,
			   t->y + y - t->main->yoff);

  cairo_rectangle(cr,
		  t->x, t->y + y,
		  t->main->linewidth, h);
  cairo_fill(cr);

  /* Fill rest of main block */
  
  cairo_set_source_rgb(cr, 
		       t->main->bg.r,
		       t->main->bg.g,
		       t->main->bg.b);

  cairo_rectangle(cr,
		  t->x, t->y + y + h,
		  t->main->linewidth, t->height - h - y);
  cairo_fill(cr);

  /* Draw scroll bar */

  pos = t->main->yoff * (t->height - y) / t->main->height;
  size = h * (t->height - y) / t->main->height;

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, t->x + t->main->linewidth, t->y + y);
  cairo_line_to(cr, t->x + t->main->linewidth, t->y + t->height);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke(cr);
  
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr,
		  t->x + t->main->linewidth + 1,
		  t->y + y,
		  SCROLL_WIDTH - 1,
		  pos);
  cairo_fill(cr);

  cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
  cairo_rectangle(cr,
		  t->x + t->main->linewidth + 1,
		  t->y + y + pos,
		  SCROLL_WIDTH - 1,
		  size);
  cairo_fill(cr);

  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr,
		  t->x + t->main->linewidth + 1,
		  t->y + y + pos + size,
		  SCROLL_WIDTH - 1,
		  t->height - y - pos - size);
  cairo_fill(cr);


  return y + h;
}

void
tabdraw(struct tab *t)
{
  int y;
  
  y = 0;

  y = tabdrawaction(t, y);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, 0, y);
  cairo_line_to(cr, t->width, y);
  cairo_stroke(cr);

  y += 1;

  y = tabdrawmain(t, y);
}
