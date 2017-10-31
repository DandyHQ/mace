#include "mace.h"

static const uint8_t defaultcol[] =
  "ummm";

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
  
  seq = sequencenew(NULL, 0);
  if (seq == NULL) {
    columnfree(c);
    return NULL;
  }
  
  if (!sequencereplace(seq, 0, 0, 
      defaultcol, strlen((const char *) defaultcol))) {
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
	if (!tabresize(t, c->width, c->height)) {
  	return false;
  }
  	
	t->next = c->tabs;
	c->tabs = t;
	
	return true;
}

void
columnremovetab(struct column *c, struct tab *t)
{

}

static bool
placetabs(struct column *c)
{
	struct tab *t;
	  
  for (t = c->tabs; t != NULL; t = t->next) {
  	if (!tabresize(t, c->width, c->height - c->textbox->height)) {
  		return false;
  	}
  }
   
  return true;
}

bool
columnresize(struct column *c, int x, int y, int w, int h)
{
  c->width = w;
  c->height = h;
  
  if (!textboxresize(c->textbox, w, h)) {
  	return false;
  }
  
  return placetabs(c);
}

void
columndraw(struct column *c, cairo_t *cr, int x, int y)
{
	struct tab *t;
	
	textboxdraw(c->textbox, cr, 0, y, c->width, c->height);
	
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, 0, y + 1 + c->textbox->height);
  cairo_line_to(cr, c->width, y + 1 + c->textbox->height);
  cairo_stroke(cr);
  
  if (c->tabs != NULL) {
  	for (t = c->tabs; t != NULL; t = t->next) {
  		tabdraw(t, cr, x, y + c->textbox->height + 2, 
  		        c->width, c->mace->height - y - c->textbox->height - 2);
  	}
  } else {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, y + c->textbox->height + 2,
                    c->width,
                    c->height);
    cairo_fill(cr);
  }
}
