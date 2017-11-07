#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

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

  flen = strlen((char *) filename);
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

  t->action->tab = t;
  t->main->tab = t;
  return t;
}

struct tab *
tabnewempty(struct mace *mace, const uint8_t *name)
{
  struct sequence *seq;
  struct tab *t;
  seq = sequencenew(NULL, 0);

  if (seq == NULL) {
    return NULL;
  }

  t = tabnew(mace, name, seq);

  if (t == NULL) {
    sequencefree(seq);
    return NULL;

  } else {
    return t;
  }
}

struct sequence *
sequencefromfile(struct stat *st, const uint8_t *path)
{
  struct sequence *seq;
  size_t dlen;
  uint8_t *data;
  int fd;
  
  dlen = st->st_size;

  if (dlen == 0) {
    return sequencenew(NULL, 0);
  }
  
  fd = open((const char *) path, O_RDONLY);

  if (fd < 0) {
    return sequencenew(NULL, 0);
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

  seq = sequencenew(data, dlen);

  if (seq == NULL) {
    free(data);
  	close(fd);
  	return NULL;
  } else {
    return seq;
   }
}

struct sequence *
sequencefromdir(struct stat *st, const uint8_t *path)
{
	struct sequence *seq;
	struct dirent *dir;
	size_t p, l;
	DIR *d;
	
	d = opendir((const char *) path);
	if (d == NULL) {
		return NULL;
	}
	
	seq = sequencenew(NULL, 0);
	if (seq == NULL) {
		closedir(d);
		return NULL;
	}
	
	p = 0;
	while ((dir = readdir(d)) != NULL) {
		if (strcmp(dir->d_name, ".") == 0) {
			continue;
		} else if (strcmp(dir->d_name, "..") == 0) {
			continue;
		}
		
		l = strlen(dir->d_name);
		
		if (!sequencereplace(seq, p, p, (const uint8_t *) dir->d_name, l)) {
			closedir(d);
			sequencefree(seq);
			return NULL;
		}
		
		p += l;		
		
		if (!sequencereplace(seq, p, p, (const uint8_t *) "\n", 1)) {
			closedir(d);
			sequencefree(seq);
			return NULL;
		}
		
		p += 1;
	}
	
	closedir(d);
	
	return seq;
}

struct tab *
tabnewfrompath(struct mace *mace,
               const uint8_t *path)
{
  struct sequence *seq;
  struct stat st;
  struct tab *t;
  
  if (stat((const char *) path, &st) != 0) {
    return NULL;
  }

	if (S_ISDIR(st.st_mode)) {
		seq = sequencefromdir(&st, path);
	} else {
		seq = sequencefromfile(&st, path);
	}
	
	if (seq == NULL) {
		return NULL;
	}
 
  t = tabnew(mace, path, seq);

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

  if (!textboxresize(t->action, w - SCROLL_WIDTH, h)) {
    return false;
  }

  return textboxresize(t->main, w - SCROLL_WIDTH, h);
}

static int
tabdrawaction(struct tab *t, cairo_t *cr, int x, int y, int w, int h)
{
  if (t->action->height < h) {
    h = t->action->height;
  }

  cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
  cairo_rectangle(cr, x, y,
                  SCROLL_WIDTH,
                  t->action->height);
  cairo_fill(cr);
  textboxdraw(t->action, cr, x + SCROLL_WIDTH, y, w - SCROLL_WIDTH, h);
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
