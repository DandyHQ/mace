#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>

#include "mace.h"
#include "config.h"

static struct colour bg   = { 1, 1, 1 };
static struct colour abg  = { 0.86, 0.94, 1 };

struct tab *
tabnew(struct mace *mace,
       const uint8_t *name,
       const uint8_t *filename,
       struct sequence *mainseq)
{
  struct sequence *actionseq;
  size_t nlen, flen;
  struct tab *t;
  
  t = calloc(1, sizeof(struct tab));
  if (t == NULL) {
    return NULL;
  }
  
  nlen = strlen((char *)name);
  flen = strlen((char *)filename);

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

  if (!sequencereplace(actionseq, 0, 0, filename, flen)) {
    tabfree(t);
    return NULL;
  }
  
  if (!sequencereplace(actionseq, flen, flen, (uint8_t *) ": ", 2)) {
  	tabfree(t);
  	return NULL;
  }
 
  if (!sequencereplace(actionseq, flen + 2, flen + 2,
		                   mace->defaultaction,
		                   strlen((const char *) mace->defaultaction))) {
    tabfree(t);
    return NULL;
  }
  
  t->action = textboxnew(mace, t, &abg, actionseq);
  if (t->action == NULL) {
    sequencefree(actionseq);
    tabfree(t);
    return NULL;
  }

  t->main = textboxnew(mace, t, &bg, mainseq);
  if (t->main == NULL) {
    tabfree(t);
    return NULL;
  }

  return t;
} 

struct tab *
tabnewemptyfile(struct mace *mace, 
                const uint8_t *name,
                const uint8_t *filename)
{
  struct sequence *seq;
  struct tab *t;

  seq = sequencenew(NULL, 0);
  if (seq == NULL) {
    return NULL;
  }

  t = tabnew(mace, name, filename, seq);
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
	return tabnewemptyfile(mace, name, name);
}

struct tab *
tabnewfromfile(struct mace *mace,
               const uint8_t *filename)
{
  struct sequence *seq;
  size_t dlen;
	uint8_t *name;
  uint8_t *data;
  struct stat st;
  struct tab *t;
  int fd;
	
	/* I am not sure if basename allocates name or if it is a slice of filename. */
	name = (uint8_t *) basename((char *) filename);
	
  fd = open((const char *) filename, O_RDONLY);
  if (fd < 0) {
    return tabnewemptyfile(mace, name, filename);
  }

  if (fstat(fd, &st) != 0) {
    close(fd);
    return NULL;
  }

  dlen = st.st_size;
  if (dlen == 0) {
    close(fd);

    return tabnewemptyfile(mace,name, filename);
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

  t = tabnew(mace, name, filename, seq);
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

  return textboxresize(t->main, w - SCROLL_WIDTH, h);
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
tabbuttonpress(struct tab *t, int x, int y, unsigned int button)
{
  int ah, lines, pos, ay, by;
  
	ah = t->action->height;

  if (y < ah) {
		t->mace->mousefocus = t->action;
    return textboxbuttonpress(t->action, x, y, button);
    
  } else if (t->mace->scrollbarleftside && x >= SCROLL_WIDTH) {
	    
		t->mace->mousefocus = t->main;
    return textboxbuttonpress(t->main, x - SCROLL_WIDTH, y - ah - 1, button);
  
  } else if (!t->mace->scrollbarleftside && x < t->width - SCROLL_WIDTH) {
	    
		t->mace->mousefocus = t->main;
    return textboxbuttonpress(t->main, x, y - ah - 1, button);
    
  } else {
  	ay = (t->mace->font->face->size->metrics.ascender >> 6);
  	by = -(t->mace->font->face->size->metrics.descender >> 6);
  
  	/* Relative line movement. */
    lines = (y - ah - 1) / (ay + by) / 2;
    if (lines == 0) {
    	lines = 1;
    }
    
    /* Immediate position. */
    pos = sequencelen(t->main->sequence) * (y - ah - 1) / (t->height - ah - 1);

		switch (button) {
		case 1:
			switch (t->mace->scrollleft) {
			case SCROLL_up:
				return textboxscroll(t->main, -lines);
				
			case SCROLL_down:
				return textboxscroll(t->main, lines);
			
			case SCROLL_immediate:
				t->mace->mousefocus = t->main;
				t->mace->immediatescrolling = true;
				t->main->start = pos;
				textboxplaceglyphs(t->main);
				return true;
				
			case SCROLL_none:
				return false;
			}

		case 2:
			switch (t->mace->scrollmiddle) {
			case SCROLL_up:
				return textboxscroll(t->main, -lines);
			case SCROLL_down:
				return textboxscroll(t->main, lines);
			
			case SCROLL_immediate:
				t->mace->mousefocus = t->main;
				t->mace->immediatescrolling = true;
				t->main->start = pos;
				textboxplaceglyphs(t->main);
				return true;
				
			case SCROLL_none:
				return false;
			}
			
		case 3:
			switch (t->mace->scrollright) {
			case SCROLL_up:
				return textboxscroll(t->main, -lines);
			case SCROLL_down:
				return textboxscroll(t->main, lines);
			
			case SCROLL_immediate:
				t->mace->mousefocus = t->main;
				t->mace->immediatescrolling = true;
				t->main->start = pos;
				textboxplaceglyphs(t->main);
				return true;
				
			case SCROLL_none:
				return false;
			}
			    
		default:
			return false;
		}
  }
}

static int
tabdrawaction(struct tab *t, cairo_t *cr, int y)
{
	int h, ah;
  
	ah = t->action->height;
	h = ah > t->height - y ? t->height - y : ah;

	textboxdraw(t->action, cr, t->x, t->y + y,
	            t->width, h + 1);
  
	return y + h + 1;
}

#define SCROLL_MIN_HEIGHT 5

static int
tabdrawmain(struct tab *t, cairo_t *cr, int y)
{
  int h, pos, size, len, x;
  
  h = t->height - y;
  
  if (t->mace->scrollbarleftside) {
		textboxdraw(t->main, cr, t->x + SCROLL_WIDTH, t->y + y, 
		            t->width - SCROLL_WIDTH, h);
	            
  } else {
		textboxdraw(t->main, cr, t->x, t->y + y, 
		            t->width - SCROLL_WIDTH, h);
  }

  
  /* Draw scroll bar */
  
  len = sequencelen(t->main->sequence);

	if (len > 0) {
		pos = t->main->start * h / sequencelen(t->main->sequence);
		size = t->main->drawablelen * h / sequencelen(t->main->sequence);
	} else {
		pos = 0;
		size = h;
	}
		
	if (size < SCROLL_MIN_HEIGHT) {
		size = SCROLL_MIN_HEIGHT;
	}
	
		
  cairo_set_source_rgb(cr, 0, 0, 0);
  
  if (t->mace->scrollbarleftside) {
  	x = 0;
  	
	  cairo_move_to(cr, t->x + SCROLL_WIDTH - 1, t->y + y);
	  cairo_line_to(cr,
			t->x + SCROLL_WIDTH - 1,
			t->y + t->height);  
			
  } else {
  	x = t->main->linewidth + 1;
  	
	  cairo_move_to(cr, t->x + t->main->linewidth, t->y + y);
	  cairo_line_to(cr,
			t->x + t->main->linewidth,
			t->y + t->height);
	}
	
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);
  
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr,
		  t->x + x,
		  t->y + y,
		  SCROLL_WIDTH - 1,
		  pos);

  cairo_fill(cr);

  cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
  cairo_rectangle(cr,
		  t->x + x,
		  t->y + y + pos,
		  SCROLL_WIDTH - 1,
		  size);

  cairo_fill(cr);

  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr,
		  t->x + x,
		  t->y + y + pos + size,
		  SCROLL_WIDTH - 1,
		  t->height - y - pos - size);
		  
  cairo_fill(cr);

  return y + h;
}

void
tabdraw(struct tab *t, cairo_t *cr)
{
  int y = 0;

  y = tabdrawaction(t, cr, y);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, t->x, t->y + y);
  cairo_line_to(cr, t->x + t->width, t->y + y);
  cairo_stroke(cr);

  y += 1;

  tabdrawmain(t, cr, y);
}
