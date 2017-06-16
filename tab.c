#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>

#include "mace.h"

#define SCROLL_WIDTH   10

static struct colour bg   = { 1, 1, 1 };
static struct colour abg  = { 0.86, 0.94, 1 };

static const uint8_t actionstart[] =
  ": save open close cut copy paste";

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

  t->mace = mace;
  t->next = NULL;

  if (!tabsetname(t, name, nlen)) {
    tabfree(t);
    return NULL;
  }

  actionseq = sequencenew(NULL, 0);
  if (actionseq == NULL) {
    tabfree(t);
    return NULL;
  }

  if (!sequenceinsert(actionseq, 0, name, nlen)) {
    tabfree(t);
    return NULL;
  }
  
  if (!sequenceinsert(actionseq, nlen,
		      actionstart,
		      strlen((const char *) actionstart))) {
    tabfree(t);
    return NULL;
  }
  
  t->action = textboxnew(t, &abg, actionseq);
  if (t->action == NULL) {
    sequencefree(actionseq);
    tabfree(t);
    return NULL;
  }

  t->action->cursor = sequencegetlen(actionseq);

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

  seq = sequencenew(NULL, 0);
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
               const uint8_t *filename, size_t flen)
{
  struct sequence *seq;
	const uint8_t *name;
  uint8_t *data;
  struct stat st;
  struct tab *t;
  size_t dlen;
  int fd;

	name = (const uint8_t *) basename((const char *) filename);

  fd = open((const char *) filename, O_RDONLY);
  if (fd < 0) {
    return NULL;
  }

  if (fstat(fd, &st) != 0) {
    close(fd);
    return NULL;
  }

  dlen = st.st_size;
  if (dlen == 0) {
    close(fd);
    return tabnewempty(mace, name, strlen((char *) name));
  }

  data = malloc(dlen);
  if (data == NULL) {
    close(fd);
    return NULL;
  }

  if (read(fd, data, dlen) != dlen) {
    free(data);
    close(fd);
    return NULL;
  }
  
  close(fd);

  seq = sequencenew(data, dlen);
  if (seq == NULL) {
    free(data);
    return NULL;
  }

  t = tabnew(mace, name, strlen((char *) name), seq);
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

  if (!textboxresize(t->main, w - SCROLL_WIDTH, h - t->action->height)) {
    return false;
  }

  return true;
}

bool
tabsetname(struct tab *t, const uint8_t *name, size_t len)
{
  uint8_t *new;
  
  new = malloc(len + 1);
  if (new == NULL) {
    return false;
  }
  
  memmove(new, name, len);
  new[len] = 0;
  
  if (t->name != NULL) {
    free(t->name);
  }

  t->name = new;
  t->nlen = len;
  
  return true;
}

bool
tabscroll(struct tab *t, int x, int y, int dx, int dy)
{
  if (y < t->action->height) {
    return false;
  } else {
    return textboxscroll(t->main, dx, t->main->yoff + dy);
  }
}

bool
tabbuttonpress(struct tab *t, int x, int y,
	       unsigned int button)
{
  int yoff;
  
  if (y < t->action->height) {
		t->mace->mousefocus = t->action;
    return textboxbuttonpress(t->action, x, y, button);

  } else if (x < t->width - SCROLL_WIDTH) {
		t->mace->mousefocus = t->main;
    return textboxbuttonpress(t->main, x, y - t->action->height - 1,
			      button);
  } else {
    yoff = (int) (t->main->height * (float) (y - t->action->height)
		  / (float) (t->height - t->action->height));

    return textboxscroll(t->main, 0, yoff);
  }
}

static int
tabdrawaction(struct tab *t, cairo_t *cr, int y)
{
  int h;
  
  h = t->height - y;
  if (t->action->height < h) {
    h = t->action->height;
  }
  
  cairo_set_source_surface(cr,
			   t->action->sfc, t->x, t->y + y);

  cairo_rectangle(cr, t->x, t->y + y, t->width, h);
  cairo_fill(cr);

  return y + h;
}

static int
tabdrawmain(struct tab *t, cairo_t *cr, int y)
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
			   t->y + y);

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

  cairo_line_to(cr,
		t->x + t->main->linewidth,
		t->y + t->height);

  cairo_set_line_width(cr, 1.0);
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
tabdraw(struct tab *t, cairo_t *cr)
{
  int y;

  y = 0;

  y = tabdrawaction(t, cr, y);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, t->x, t->y + y);
  cairo_line_to(cr, t->x + t->width, t->y + y);
  cairo_stroke(cr);

  y += 1;

  y = tabdrawmain(t, cr, y);
}
