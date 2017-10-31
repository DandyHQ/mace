#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>

#include "mace.h"
#include "config.h"

struct tab *
tabnew(struct mace *mace,
       const uint8_t *filename,
       struct sequence *mainseq)
{
  struct sequence *actionseq;
  size_t flen;
  struct tab *t;
  t = calloc(1, sizeof(struct tab));

  if (t == NULL) {
    return NULL;
  }

  flen = strlen((char *)filename);
  t->mace = mace;
  t->next = NULL;

  actionseq = sequencenew(NULL, 0);

  if (actionseq == NULL) {
    tabfree(t);
    return NULL;
  }

  if (!sequencereplace(actionseq, 0, 0, filename, flen)) {
    tabfree(t);
    return NULL;
  }

  if (!sequencereplace(actionseq, flen, flen,
                       (uint8_t *) ": ", 2)) {
    tabfree(t);
    return NULL;
  }

  if (!sequencereplace(actionseq, flen + 2, flen + 2,
                       mace->defaultaction,
                       strlen((const char *) mace->defaultaction))) {
    tabfree(t);
    return NULL;
  }

  t->action = textboxnew(mace, &abg, actionseq);

  if (t->action == NULL) {
    sequencefree(actionseq);
    tabfree(t);
    return NULL;
  }

  t->main = textboxnew(mace, &bg, mainseq);

  if (t->main == NULL) {
    tabfree(t);
    return NULL;
  }

  return t;
}

struct tab *
tabnewemptyfile(struct mace *mace,
                const uint8_t *filename)
{
  struct sequence *seq;
  struct tab *t;
  seq = sequencenew(NULL, 0);

  if (seq == NULL) {
    return NULL;
  }

  t = tabnew(mace, filename, seq);

  if (t == NULL) {
    sequencefree(seq);
    return NULL;
  } else {
    return t;
  }
}

struct tab *
tabnewempty(struct mace *mace, const uint8_t *name)
{
  return tabnewemptyfile(mace, name);
}

struct tab *
tabnewfromfile(struct mace *mace,
               const uint8_t *filename)
{
  struct sequence *seq;
  size_t dlen;
  uint8_t *data;
  struct stat st;
  struct tab *t;
  int fd;
  
  fd = open((const char *) filename, O_RDONLY);

  if (fd < 0) {
    return tabnewemptyfile(mace, filename);
  }

  if (fstat(fd, &st) != 0) {
    close(fd);
    return NULL;
  }

  dlen = st.st_size;

  if (dlen == 0) {
    close(fd);
    return tabnewemptyfile(mace, filename);
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

  t = tabnew(mace, filename, seq);

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

  free(t);
}

bool
tabresize(struct tab *t, int w, int h)
{
  t->width = w;
  t->height = h;

  if (!textboxresize(t->action, w, h)) {
    return false;
  }

  return textboxresize(t->main, w - SCROLL_WIDTH, h);
}

bool
tabbuttonpress(struct tab *t, int x, int y,
               unsigned int button)
{
  int ah, lines, pos, ay, by;
  scroll_action_t action;
  
  ah = t->action->height;

  if (y < ah) {
    t->mace->mousefocus = t->action;
    return textboxbuttonpress(t->action, x, y, button);
    
  } else if (x >= SCROLL_WIDTH) {
    t->mace->mousefocus = t->main;
    return textboxbuttonpress(t->main, x - SCROLL_WIDTH,
                              y - ah - 1,
                              button);
  } else {
    ay = (t->mace->font->face->size->metrics.ascender >> 6);
    by = -(t->mace->font->face->size->metrics.descender >> 6);
    t->mace->mousefocus = t->main;

    switch (button) {
    case 1:
      action = t->mace->scrollleft;
      break;

    case 2:
      action = t->mace->scrollmiddle;
      break;

    case 3:
      action = t->mace->scrollright;
      break;

    default:
      action = SCROLL_none;
      break;
    }

    switch (action) {
    case SCROLL_up:
      lines = (y - ah - 1) / (ay + by) / 2;

      if (lines == 0) {
        lines = 1;
      }

      return textboxscroll(t->main, -lines);

    case SCROLL_down:
      lines = (y - ah - 1) / (ay + by) / 2;

      if (lines == 0) {
        lines = 1;
      }

      return textboxscroll(t->main, lines);

    case SCROLL_immediate:
      pos = sequencelen(t->main->sequence) * (y - ah - 1)
            / (t->height - ah - 1);
      pos = sequenceindexline(t->main->sequence, pos);
      t->mace->immediatescrolling = true;
      t->main->start = pos;
      textboxplaceglyphs(t->main);
      return true;

    default:
      return false;
    }
  }
}

static int
tabdrawaction(struct tab *t, cairo_t *cr, int x, int y, int w, int h)
{
  if (t->action->height < h) {
  	h = t->action->height;
  }
  
  textboxdraw(t->action, cr, x, y, w, h);
  return y + h;
}

#define SCROLL_MIN_HEIGHT 5

static int
tabdrawmain(struct tab *t, cairo_t *cr, int x, int y, int w, int h)
{
  int pos, size, len;

  textboxdraw(t->main, cr, x + SCROLL_WIDTH, y,
              w - SCROLL_WIDTH, h);

  /* Draw scroll bar */
  len = sequencelen(t->main->sequence);

  if (len > 0) {
    pos = t->main->start * h / len;
    size = t->main->drawablelen * h / len;
  } else {
    pos = 0;
    size = h;
  }

  if (size < SCROLL_MIN_HEIGHT) {
    size = SCROLL_MIN_HEIGHT;
  }

  cairo_set_source_rgb(cr, 0, 0, 0);

  cairo_move_to(cr, x + SCROLL_WIDTH - 1, y);
  cairo_line_to(cr,
                x + SCROLL_WIDTH - 1,
                y + h);
 
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr,
                  x,
                  y,
                  SCROLL_WIDTH - 1,
                  pos);
  cairo_fill(cr);
  cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
  cairo_rectangle(cr,
                  x,
                  y + pos,
                  SCROLL_WIDTH - 1,
                  size);
                  
  cairo_fill(cr);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr,
                  x,
                  y + pos + size,
                  SCROLL_WIDTH - 1,
                  h - pos - size);
  cairo_fill(cr);
  return y + h;
}

void
tabdraw(struct tab *t, cairo_t *cr, int x, int y, int w, int h)
{
  y = tabdrawaction(t, cr, x, y, w, h);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, x, y);
  cairo_line_to(cr, x + w, y);
  cairo_stroke(cr);
  y += 1;
  
  tabdrawmain(t, cr, x, y, w, h);
}
