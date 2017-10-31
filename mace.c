#include <string.h>
#include "mace.h"

static const uint8_t defaultaction[] =
  "save open close cut copy paste undo redo";

static const uint8_t defaultmain[] =
  "quit newcol";

struct colour bg   = { 1, 1, 1 };
struct colour abg  = { 0.86, 0.94, 1 };

struct mace *
macenew(void)
{
	struct sequence *seq;
  struct mace *m;
  size_t l;
  
  m = calloc(1, sizeof(struct mace));

  if (m == NULL) {
    return NULL;
  }

  m->font = fontnew();

  if (m->font == NULL) {
    macefree(m);
    return NULL;
  }
  
  seq = sequencenew(NULL, 0);
  if (seq == NULL) {
    macefree(m);
    return NULL;
  }
  
  if (!sequencereplace(seq, 0, 0, 
      defaultmain, strlen((const char *) defaultmain))) {
    sequencefree(seq);
    macefree(m);
  }

  m->textbox = textboxnew(m, &abg,
                          seq);
	if (m->textbox == NULL) {
  	sequencefree(seq);
  	macefree(m);
  	return NULL;
  }

  m->columns = columnnew(m);

  if (m->columns == NULL) {
    macefree(m);
    return NULL;
  }
  
  l = strlen((char *) defaultaction);
  m->defaultaction = malloc(sizeof(uint8_t) * (l + 1));

  if (m->defaultaction == NULL) {
    macefree(m);
    return NULL;
  }

  memmove(m->defaultaction, defaultaction, l + 1);
  
  m->scrollleft = SCROLL_up;
  m->scrollmiddle = SCROLL_immediate;
  m->scrollright = SCROLL_down;
  m->running = true;
  
  return m;
}

void
macequit(struct mace *m)
{
  m->running = false;
}

void
macefree(struct mace *m)
{
	struct column *c, *cn;
	
  curselremoveall(m);

	if (m->textbox != NULL) {
		textboxfree(m->textbox);
	}

	c = m->columns;
	while (c != NULL) {
		cn = c->next;
		columnfree(c);
		c = cn;
	}

  if (m->font != NULL) {
    fontfree(m->font);
  }

  if (m->defaultaction != NULL) {
    free(m->defaultaction);
  }

  free(m);
}

bool
maceaddkeybinding(struct mace *m, uint8_t *key,
                  uint8_t *cmd)
{
  struct keybinding *k;
  size_t klen, clen;
  k = malloc(sizeof(struct keybinding));

  if (k == NULL) {
    return false;
  }

  klen = strlen((char *) key) + 1;
  clen = strlen((char *) cmd) + 1;
  k->key = malloc(klen);

  if (k->key == NULL) {
    free(k);
    return false;
  }

  k->cmd = malloc(clen);

  if (k->cmd == NULL) {
    free(k->key);
    free(k);
    return false;
  }

  memmove(k->key, key, klen);
  memmove(k->cmd, cmd, clen);
  k->next = m->keybindings;
  m->keybindings = k;
  return true;
}

struct textbox *
findtextbox(struct mace *m, int x, int y)
{
	struct column *c;
	struct tab *tab;
	
	if (y < m->textbox->height) {
		return m->textbox;
	} else {
		y -= m->textbox->height + 1;
	
		for (c = m->columns; c != NULL; x -= c->width, c = c->next) {
			if (x <= c->width) {
				if (y < c->textbox->height) {
					return c->textbox;
				}
				
				y -= c->textbox->height + 1;
				for (tab = c->tabs; tab != NULL; tab = tab->next) {
					if (y < tab->action->height) {
						return tab->action;
					} else if (y < tab->height) {
						return tab->main;
					}
				}
			}
		}
	}
	
	return NULL;
}

bool
handlebuttonpress(struct mace *m, int x, int y, int button)
{
	struct textbox *t;
	
	t = findtextbox(m, x, y);
	if (t == NULL) {
		return false;
	}
	
	m->mousefocus = t;
	return textboxbuttonpress(t, x - t->x, y - t->y, button);
}

bool
handlebuttonrelease(struct mace *m, int x, int y,
                    int button)
{
  if (m->mousefocus == NULL) {
    return false;
  }

  x -= m->mousefocus->x;
  y -= m->mousefocus->y;

  if (m->immediatescrolling) {
    m->immediatescrolling = false;
    return false;
  } else {
    return textboxbuttonrelease(m->mousefocus, x, y, button);
  }
}

bool
handlemotion(struct mace *m, int x, int y)
{
  size_t pos;

  if (m->mousefocus == NULL) {
    return false;
  }

  x -= m->mousefocus->x;
  y -= m->mousefocus->y;

  if (m->immediatescrolling) {
    if (m->mousefocus->maxheight == 0) {
      return false;
    }

    if (y < 0) {
      y = 0;
    } else if (y > m->mousefocus->maxheight) {
      y = m->mousefocus->maxheight;
    }

    pos = sequencelen(m->mousefocus->sequence) * y
          / m->mousefocus->maxheight;
    pos = sequenceindexline(m->mousefocus->sequence, pos);

    if (m->mousefocus->start != pos) {
      m->mousefocus->start = pos;
      textboxplaceglyphs(m->mousefocus);
      return true;
    } else {
      return false;
    }
  } else {
    return textboxmotion(m->mousefocus, x, y);
  }
}

bool
handlescroll(struct mace *m, int x, int y, int dx, int dy)
{
	struct textbox *t;
  int lines;
  
  t = findtextbox(m, x, y);
  if (t == NULL) {
  	return false;
  }
  
  x -= t->x;
  y -= t->y;

  lines = dy > 0 ? 1 : -1;

  return textboxscroll(t, lines);
}

static bool
handletyping(struct mace *m, uint8_t *s, size_t n)
{
  struct cursel *c;
  size_t start;

  for (c = m->cursels; c != NULL; c = c->next) {
    if ((c->type & CURSEL_nrm) == 0) {
      continue;
    }

    sequencereplace(c->tb->sequence, c->start, c->end, s, n);
    textboxplaceglyphs(c->tb);
    start = c->start;
    shiftcursels(m, c->tb, start,
                 (int) n - (int) (c->end - start));
    c->start = start + n;
    c->end = start + n;
    c->cur = 0;
  }

  return true;
}

bool
handlekey(struct mace *m, uint8_t *s, size_t n,
          bool special)
{
  struct keybinding *k;

  for (k = m->keybindings; k != NULL; k = k->next) {
    if ((n == strlen((char *) k->key)) &&
        (strncmp((char *) s, (char *) k->key, n) == 0)) {
      command(m, k->cmd);
      return true;
    }
  }

  if (special) {
    return false;
  } else {
    /* Remove modifiers if we're putting it straight in. */
    while (n > 2 && s[1] == '-') {
      s += 2;
      n -= 2;
    }

    return handletyping(m, s, n);
  }
}

bool
maceresize(struct mace *m, int w, int h)
{
	struct column *c;
	
	m->width = w;
	m->height = h;
	
	if (!textboxresize(m->textbox, w, h)) {
		return false;
	}
	
	for (c = m->columns; c != NULL; c = c->next) {
		columnresize(c, 0, m->font->lineheight, w, h);
	}
	
	return true;
}

void
macedraw(struct mace *m, cairo_t *cr)
{
	struct column *c;
	int x;
	
	textboxdraw(m->textbox, cr, 0, 0, m->width, m->height);
	
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, 0, 1 + m->textbox->height);
  cairo_line_to(cr, m->width, 1 + m->textbox->height);
  cairo_stroke(cr);
  
  if (m->columns != NULL) {
  	for (x = 0, c = m->columns; c != NULL; x += c->width, c = c->next) {
  		columndraw(c, cr, x, m->textbox->height + 2);
  	}
  } else {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, m->textbox->height + 2,
                    m->width,
                    m->height);
    cairo_fill(cr);
  }
}
