#include "mace.h"

struct column *
columnnew(struct mace *mace)
{
	struct sequence *seq;
  struct column *c;
  c = calloc(1, sizeof(struct column));

  if (c == NULL) {
    return NULL;
  }

  c->mace = mace;
  c->width = 1;
  c->height = 1;
  
  seq = sequencenew(NULL, 0);
  if (seq == NULL) {
    columnfree(c);
    return NULL;
  }
  
  if (!sequencereplace(seq, 0, 0, 
      mace->defaultcol, strlen((const char *) mace->defaultcol))) {
    sequencefree(seq);
    columnfree(c);
    return NULL;
  }

  c->textbox = textboxnew(c->mace, &abg,
                          seq);
  if (c->textbox == NULL) {
  	sequencefree(seq);
  	columnfree(c);
  	return NULL;
  }
  
  c->textbox->column = c;
  
  return c;
}

void
columnfree(struct column *c)
{
  struct tab *t, *n;
  
  textboxfree(c->textbox);

  t = c->tabs;
  while (t != NULL) {
    n = t->next;
    tabfree(t);
    t = n;
  }

  free(c);
}

bool
columnaddtab(struct column *c, struct tab *t)
{
	struct tab *p;
	int ha, hb;
	
	/* TODO: needs improvement to collapse and push tabs
	   up if there are too many. */
	
	if (c->tabs == NULL) {
		c->tabs = t;

		if (!tabresize(t, c->width, c->height)) {
			fprintf(stderr, "Failed to resize tab!\n");
			exit(EXIT_FAILURE);
  	}
  	
	} else {
		for (p = c->tabs; p->next != NULL; p = p->next)
			;
		
		p->next = t;
		
		hb = p->height / 2;
		ha = p->height - hb;
		if (hb == 0) hb = 1;
		if (ha == 0) ha = 1;
		
		if (!tabresize(t, c->width, ha)) {
			fprintf(stderr, "Failed to resize tab!\n");
			exit(EXIT_FAILURE);
		}
		
		if (!tabresize(p, c->width, hb)) {
			fprintf(stderr, "Failed to resize tab!\n");
			exit(EXIT_FAILURE);
		}				
	}
	
	t->next = NULL;
	t->column = c;
	
	return true;
}

void
columnremovetab(struct column *c, struct tab *t)
{
	struct tab *p;
	
	if (c->tabs == t) {
		c->tabs = t->next;
		if (c->tabs != NULL) {
			tabresize(c->tabs, c->width, c->tabs->height + t->height);
		}
	} else {
		for (p = c->tabs; p->next != t; p = p->next)
			;
	
		p->next = t->next;
		tabresize(p, c->width, p->height + t->height);
	}
}

bool
columnresize(struct column *c, int w, int h)
{
	struct tab *t;
  
  if (!textboxresize(c->textbox, w - SCROLL_WIDTH, h)) {
  	return false;
  }
  	  
  for (t = c->tabs; t != NULL; t = t->next) {
  	if (!tabresize(t, w, t->height * h / c->height)) {
  		return false;
  	}
  }
	
  c->width = w;
  c->height = h;
  
  return true;
}

void
columndraw(struct column *c, cairo_t *cr, int x, int y)
{
	struct tab *t;
  
	textboxdraw(c->textbox, cr, x + SCROLL_WIDTH, y, c->width - SCROLL_WIDTH, c->height);
	
  cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
  cairo_rectangle(cr, x, y,
                  SCROLL_WIDTH,
                  c->textbox->height);
  cairo_fill(cr);
  
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, x, y + c->textbox->height);
  cairo_line_to(cr, x + c->width, y + c->textbox->height);
  cairo_stroke(cr);
  
  cairo_move_to(cr, x, y);
  cairo_line_to(cr, x, c->mace->height);
  cairo_stroke(cr);
  
  y += c->textbox->height + 1;
  
  if (c->tabs != NULL) {
  	for (t = c->tabs; t != NULL; t = t->next) {
  		tabdraw(t, cr, x, y, 
  		        c->width, t->height);
  		y += t->height;
  	}
  } else {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, x + 1, 
                    y,
                    c->width - 1,
                    c->mace->height - y);
    cairo_fill(cr);
  }
}
