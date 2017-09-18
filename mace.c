#include <string.h>
#include "mace.h"

static const uint8_t defaultaction[] = 
    "save open close cut copy paste undo redo";

struct mace *
macenew(void)
{
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

  m->pane = panenew(m);
  if (m->pane == NULL) {
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
	
	m->scrollbarleftside = true;
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
	curselremoveall(m);

  if (m->pane != NULL) {
    panefree(m->pane);
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
maceaddkeybinding(struct mace *m, uint8_t *key, uint8_t *cmd)
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

static struct pane *
findpane(struct mace *m, int x, int y)
{
  struct pane *p;

  p = m->pane;

  /* Once we had multiple panes in find the one that bounds x,y. */
  
  return p;
}

bool
handlebuttonpress(struct mace *m, int x, int y, int button)
{
  struct pane *p = findpane(m, x, y);

  x -= p->x;
  y -= p->y;
  
  if (y < m->font->lineheight) {
		m->mousefocus = NULL;
    return tablistbuttonpress(p, x, y, button);
  } else {
		return tabbuttonpress(p->focus, x, y - m->font->lineheight, button);
	}
}

bool
handlebuttonrelease(struct mace *m, int x, int y, int button)
{
	if (m->mousefocus == NULL) {
		return false;
	}
	
	
	x -= m->mousefocus->tab->x;
	y -= m->mousefocus->tab->y;

	if (m->mousefocus->tab->main == m->mousefocus) {
		y -= m->mousefocus->tab->action->height;
	}
		
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
		
	x -= m->mousefocus->tab->x;
	y -= m->mousefocus->tab->y;

	if (m->mousefocus->tab->main == m->mousefocus) {
		y -= m->mousefocus->tab->action->height;
		
		if (m->scrollbarleftside) {
			x -= SCROLL_WIDTH;
		}
	}

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
  struct pane *p = findpane(m, x, y);
  int lines;

  x -= p->x;
  y -= p->y;
  
  if (y < m->font->lineheight) {
    return tablistscroll(p, x, y, dx, dy);
  } else {
  	lines = dy > 0 ? 1 : -1;
  	
  	if (y < p->focus->action->height) {
  	  return false;
	  } else {
 	   return textboxscroll(p->focus->main, lines);
 	 }
  }
}

static bool
handletyping(struct mace *m, uint8_t *s, size_t n)
{
	struct cursel *c;
	size_t start;

	for (c = m->cursels; c != NULL; c = c->next) {
		if ((c->type & CURSEL_nrm) == 0) continue;

		sequencereplace(c->tb->sequence, c->start, c->end, s, n);
		textboxplaceglyphs(c->tb);
		
		start = c->start;

		shiftcursels(m, c->tb, start, (int) n - (int) (c->end - start));
		c->start = start + n;
		c->end = start + n;
		c->cur = 0;
	}

	return true;
}

bool
handlekey(struct mace *m, uint8_t *s, size_t n, bool special)
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
