#include <string.h>
#include "mace.h"
#include "config.h"

static uint8_t message[] =
  "Welcome to the Mace text editor."
  "\n\n" 
  "We need to redo the guide."
  "\n\n"
;

struct defkeybinding {
	uint8_t *key, *cmd;
};

static struct defkeybinding defaultkeybindings[] = {
	{ (uint8_t *) "Tab",         (uint8_t *) "tab" },
	{ (uint8_t *) "Return",      (uint8_t *) "return" },
	{ (uint8_t *) "Delete",      (uint8_t *) "del" },
	{ (uint8_t *) "BackSpace",   (uint8_t *) "back" },
};

struct mace *
macenew(void)
{
  const uint8_t name[] = "Mace";
  struct sequence *s;
  struct mace *m;
  struct tab *t;
  uint8_t *buf;
  size_t l, i;

  m = calloc(1, sizeof(struct mace));
  if (m == NULL) {
    return NULL;
  }

  m->font = fontnew();
  if (m->font == NULL) {
    macefree(m);
    return NULL;
  }

  buf = malloc(sizeof(message));
  if (buf == NULL) {
    macefree(m);
    return NULL;
  }

  l = snprintf((char *) buf, sizeof(message), "%s", message);
  
  s = sequencenew(m->font, buf, l);
  if (s == NULL) {
    free(buf);
    macefree(m);
    return NULL;
  }

  m->pane = panenew(m);
  if (m->pane == NULL) {
    macefree(m);
    return NULL;
  }

  t = tabnew(m, name, strlen((const char *) name),
	     name, strlen((const char *) name), s);
  if (t == NULL) {
    macefree(m);
    return NULL;
  }

  paneaddtab(m->pane, t, 0);
  m->pane->focus = t;

	for (i = 0; i < sizeof(defaultkeybindings) / sizeof(defaultkeybindings[0]); i++) {
		maceaddkeybinding(m, defaultkeybindings[i].key, defaultkeybindings[i].cmd);
	}

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
			y -= sequenceheight(m->mousefocus->tab->action->sequence);
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
			y -= sequenceheight(m->mousefocus->tab->action->sequence);
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

static void
deleteselections(struct mace *m, struct cursor *c)
{
	struct selection *s, *sn;
	size_t start, len;

	s = m->selections; 
	while (s != NULL) {
		sn = s->next;
	
		if (s->tb == c->tb && s->start <= c->pos && c->pos <= s->end) {
			start = s->start;
			len = s->end - s->start;

			sequencedelete(s->tb->sequence, start, len);
			shiftselections(m, s->tb, start, -len);
			shiftcursors(m, s->tb, start, -len);
			c->pos = start;
			selectionremove(m, s);
		}

		s  = sn;
	}
}

static bool
handletyping(struct mace *m, uint8_t *s, size_t n)
{
	struct cursor *c;

	for (c = m->cursors; c != NULL; c = c->next) {
		deleteselections(m, c);

		sequenceinsert(c->tb->sequence, c->pos, s, n);
		shiftselections(m, c->tb, c->pos, n);
		shiftcursors(m, c->tb, c->pos, n);
	}

	return true;
}

bool
handlekey(struct mace *m, uint8_t *s, size_t n)
{
	struct keybinding *k;

	for (k = m->keybindings; k != NULL; k = k->next) {
		if (strncmp((char *) s, (char *) k->key, n) == 0) {
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
