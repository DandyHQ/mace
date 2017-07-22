#include <string.h>
#include "mace.h"

struct mace *
macenew(void)
{
  struct mace *m;

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
  
  m->clipboard = NULL;
  m->clipboardlen = 0;

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
	if (m->mousefocus != NULL) {
		x -= m->mousefocus->tab->x;
		y -= m->mousefocus->tab->y;

		if (m->mousefocus->tab->main == m->mousefocus) {
			y -= m->mousefocus->tab->action->height;
		}
		
  	return textboxbuttonrelease(m->mousefocus, x, y, button);
	} else {
		return false;
	}
}

bool
handlemotion(struct mace *m, int x, int y)
{
	if (m->mousefocus != NULL) {
  	x -= m->mousefocus->tab->x;
		y -= m->mousefocus->tab->y;

		if (m->mousefocus->tab->main == m->mousefocus) {
			y -= m->mousefocus->tab->action->height;
		}

		return textboxmotion(m->mousefocus, x, y);
	} else {
		return false;
	}
}

bool
handlescroll(struct mace *m, int x, int y, int dx, int dy)
{
  struct pane *p = findpane(m, x, y);

  x -= p->x;
  y -= p->y;
  
  if (y < m->font->lineheight) {
    return tablistscroll(p, x, y, dx, dy);
  } else {
    return tabscroll(p->focus, x, y - m->font->lineheight, dx, dy);
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
handlekey(struct mace *m, uint8_t *s, size_t n)
{
	struct keybinding *k;

	for (k = m->keybindings; k != NULL; k = k->next) {
		if ((n == strlen((char *) k->key)) && 
		    (strncmp((char *) s, (char *) k->key, n) == 0)) {

			command(m, k->cmd);
			return true;
		}
	}

	/* Remove modifiers if we're putting it straight in. */
	while (n > 2 && s[1] == '-') {
		s += 2;
		n -= 2;
	}

	return handletyping(m, s, n);
}
