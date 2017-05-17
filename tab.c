#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mace.h"

#define SCROLL_WIDTH   10

static struct colour bg   = { 1, 1, 1 };
static struct colour abg  = { 0.86, 0.94, 1 };

static const uint8_t actionstart[] = ": save cut copy paste search";

struct tab *
tabnew(struct mace *mace,
       const uint8_t *name, size_t nlen,
       uint8_t *data, size_t dlen, size_t dmax)
{
  struct tab *t;
  uint8_t *s;
  size_t al;
  
  al = nlen + sizeof(actionstart);
  s = malloc(al);
  if (s == NULL) {
    return NULL;
  }

  memmove(s, name, nlen);
  memmove(s + nlen, actionstart, sizeof(actionstart));
  
  t = calloc(1, sizeof(struct tab));
  if (t == NULL) {
    free(s);
    return NULL;
  }

  t->name = malloc(nlen + 1);
  if (t->name == NULL) {
    tabfree(t);
    free(s);
    return NULL;
  }

  strncpy(t->name, name, nlen);
  t->name[nlen] = 0;

  t->action = textboxnew(t, &abg, s, al, al);
  if (t->action == NULL) {
    tabfree(t);
    free(s);
    return NULL;
  }

  t->action->cursor = al;

  t->main = textboxnew(t, &bg, data, dlen, dmax);
  if (t->main == NULL) {
    tabfree(t);
    return NULL;
  }

  return t;
} 

struct tab *
tabnewempty(struct mace *mace, const uint8_t *name, size_t nlen)
{
  return tabnew(mace, name, nlen, NULL, 0, 0);
}

struct tab *
tabnewfromfile(struct mace *mace,
	       const uint8_t *name, size_t nlen,
	       const uint8_t *filename, size_t flen)
{
  uint8_t *data, *map;
  struct stat st;
  size_t dlen;
  int fd;

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    return NULL;
  }

  if (fstat(fd, &st) != 0) {
    close(fd);
    return NULL;
  }

  dlen = st.st_size;
  
  map = mmap(NULL, dlen, PROT_READ, MAP_PRIVATE, fd, 0);

  close(fd);

  if (map == MAP_FAILED) {
    return NULL;
  }
  
  data = malloc(dlen);
  if (data == NULL) {
    munmap(map, dlen);
    return NULL;
  }

  memmove(data, map, dlen);
  
  munmap(map, dlen);

  return tabnew(mace, name, nlen, data, dlen, dlen);
}

void
tabfree(struct tab *t)
{
  luaremove(t);

  if (t->action != NULL) {
    textboxfree(t->action);
  }

  if (t->main != NULL) {
    textboxfree(t->main);
  }

  if (t->name != NULL) {
    free(t->name);
  }
  
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
    mace->focus = t->action;
    textboxbuttonpress(t->action, x, y, button);
  } else {
    mace->focus = t->main;
    textboxbuttonpress(t->main, x, y - t->action->height - 1,
			      button);
  }
}

void
tabbuttonrelease(struct tab *t, int x, int y,
	   unsigned int button)
{
  if (mace->focus == t->action) {
    textboxbuttonrelease(t->action, x, y, button);
  } else {
    textboxbuttonrelease(t->main, x, y - t->action->height - 1,
				button);
  }
}

void
tabmotion(struct tab *t, int x, int y)
{
  if (mace->focus == t->action) {
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
  
  cairo_set_source_surface(mace->cr,
			   t->action->sfc, t->x, t->y + y);

  cairo_rectangle(mace->cr, t->x, t->y + y, t->width, h);
  cairo_fill(mace->cr);

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

  cairo_set_source_surface(mace->cr, t->main->sfc,
			   t->x,
			   t->y + y - t->main->yoff);

  cairo_rectangle(mace->cr,
		  t->x, t->y + y,
		  t->main->linewidth, h);

  cairo_fill(mace->cr);

  /* Fill rest of main block */
  
  cairo_set_source_rgb(mace->cr, 
		       t->main->bg.r,
		       t->main->bg.g,
		       t->main->bg.b);

  cairo_rectangle(mace->cr,
		  t->x, t->y + y + h,
		  t->main->linewidth, t->height - h - y);

  cairo_fill(mace->cr);

  /* Draw scroll bar */

  pos = t->main->yoff * (t->height - y) / t->main->height;
  size = h * (t->height - y) / t->main->height;

  cairo_set_source_rgb(mace->cr, 0, 0, 0);
  cairo_move_to(mace->cr, t->x + t->main->linewidth, t->y + y);

  cairo_line_to(mace->cr,
		t->x + t->main->linewidth,
		t->y + t->height);

  cairo_set_line_width(mace->cr, 1.0);
  cairo_stroke(mace->cr);
  
  cairo_set_source_rgb(mace->cr, 1, 1, 1);
  cairo_rectangle(mace->cr,
		  t->x + t->main->linewidth + 1,
		  t->y + y,
		  SCROLL_WIDTH - 1,
		  pos);

  cairo_fill(mace->cr);

  cairo_set_source_rgb(mace->cr, 0.3, 0.3, 0.3);
  cairo_rectangle(mace->cr,
		  t->x + t->main->linewidth + 1,
		  t->y + y + pos,
		  SCROLL_WIDTH - 1,
		  size);

  cairo_fill(mace->cr);

  cairo_set_source_rgb(mace->cr, 1, 1, 1);
  cairo_rectangle(mace->cr,
		  t->x + t->main->linewidth + 1,
		  t->y + y + pos + size,
		  SCROLL_WIDTH - 1,
		  t->height - y - pos - size);
  cairo_fill(mace->cr);


  return y + h;
}

void
tabdraw(struct tab *t)
{
  int y;

  y = 0;

  y = tabdrawaction(t, y);

  cairo_set_source_rgb(mace->cr, 0, 0, 0);
  cairo_move_to(mace->cr, t->x, t->y + y);
  cairo_line_to(mace->cr, t->x + t->width, t->y + y);
  cairo_stroke(mace->cr);

  y += 1;

  y = tabdrawmain(t, y);
}
