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
       struct sequence *mainseq)
{
  struct sequence *actionseq;
  struct tab *t;
  
  t = calloc(1, sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }

  t->name = malloc(nlen + 1);
  if (t->name == NULL) {
    tabfree(t);
    return NULL;
  }

  strncpy(t->name, name, nlen);
  t->name[nlen] = 0;

  actionseq = sequencenew(NULL, 0, 0);
  if (actionseq == NULL) {
    tabfree(t);
    return NULL;
  }

  if (!sequenceinsert(actionseq, 0, name, nlen)) {
    tabfree(t);
    return NULL;
  }
  
  if (!sequenceinsert(actionseq, nlen,
		      actionstart, strlen(actionstart))) {
    tabfree(t);
    return NULL;
  }
  
  t->action = textboxnew(t, &abg, actionseq);
  if (t->action == NULL) {
    sequencefree(actionseq);
    tabfree(t);
    return NULL;
  }

  t->action->cursor = nlen + strlen(actionstart);

  t->main = textboxnew(t, &bg, mainseq);
  if (t->main == NULL) {
    tabfree(t);
    return NULL;
  }

  return t;
} 

struct tab *
tabnewempty(struct mace *mace, const uint8_t *name, size_t nlen)
{
  struct sequence *seq;
  struct tab *t;

  seq = sequencenew(NULL, 0, 0);
  if (seq == NULL) {
    return NULL;
  }

  t = tabnew(mace, name, nlen, seq);
  if (t == NULL) {
    sequencefree(seq);
    return NULL;
  } else {
    return t;
  }
}

struct tab *
tabnewfromfile(struct mace *mace,
	       const uint8_t *name, size_t nlen,
	       const uint8_t *filename, size_t flen)
{
  struct sequence *seq;
  size_t dlen, dmax;
  struct stat st;
  uint8_t *data;
  struct tab *t;
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

  dmax = RDUP(dlen, sysconf(_SC_PAGESIZE));

  data = mmap(NULL, dmax, PROT_READ|PROT_WRITE,
	      MAP_PRIVATE|MAP_COPY,
	      fd, 0);

  close(fd);

  if (data == MAP_FAILED) {
    return NULL;
  }

  seq = sequencenew(data, dlen, dmax);
  if (seq == NULL) {
    return NULL;
  }

  t = tabnew(mace, name, nlen, seq);
  if (t == NULL) {
    sequencefree(seq);
    return NULL;
  } else {
    return t;
  }
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
  
  if (!textboxresize(t->action, w, h)) {
    return false;
  }

  if (!textboxresize(t->main, w - SCROLL_WIDTH, h)) {
    return false;
  }

  return true;
}

bool
tabscroll(struct tab *t, int x, int y, int dy)
{
  if (y < t->action->height) {
    return false;
  } else {
    return textboxscroll(t->main, x, y - t->action->height - 1, dy);
  }
}

bool
tabbuttonpress(struct tab *t, int x, int y,
	       unsigned int button)
{
  if (y < t->action->height) {
    mace->focus = t->action;
    return textboxbuttonpress(t->action, x, y, button);
  } else {
    mace->focus = t->main;
    return textboxbuttonpress(t->main, x, y - t->action->height - 1,
			      button);
  }
}

bool
tabbuttonrelease(struct tab *t, int x, int y,
	   unsigned int button)
{
  if (mace->focus == t->action) {
    return textboxbuttonrelease(t->action, x, y, button);
  } else {
    return textboxbuttonrelease(t->main, x, y - t->action->height - 1,
				button);
  }
}

bool
tabmotion(struct tab *t, int x, int y)
{
  if (mace->focus == t->action) {
    return textboxmotion(t->action, x, y);
  } else {
    return textboxmotion(t->main, x, y - t->action->height - 1);
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
			   t->y + y);

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
