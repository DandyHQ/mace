#include <string.h>
#include "mace.h"
#include "config.h"

static uint8_t message[] =
  "Welcome to the Mace text editor. Below is a basic guide on how to use this editor."
  "\n\n" 
  "When you type, text is added at the cursor position. The cursor can be moved with the arrow keys, "
  "or by clicking on the desired position with the mouse. Text can be selected by clicking and dragging "
  "with the mouse."	
  "\n\n"
  "You can scroll the window with a mouse scroll wheel, or by clicking on different positions on the "
  "scroll bar on the right side of the screen."
  "\n\n"
  "At the top of the screen is a bar that shows the different tabs that are open in Mace. You can "
  "navigate between these by clicking on the corresponding box. The current tab is represented with a "
  "white box, while the others are grey."
  "\n\n"
  "Just below this is another bar. On the left of this is the name of the current file, and then "
  "several commands. Each of these commands can be executed by right-clicking on them. For example, "
  "right-clicking on save will save this guide as a file in the Mace directory. To create a new file, "
  "or open an existing file, type and select the pathname and right-click the open command. This will "
  "open a new tab with that file."
  "\n\n" 
  "Each of the other commands work as you might expect. The close command closes the current tab. "
  "The cut and copy commands cut/copy the currently selected text, and the paste commands pastes " 
  "the most recently copied text at the cursor position. Each of these commands can be used from the "
  "command bar, or directly from the text. For example, if you click the word save in this sentence, "
  "save commands will be executed! "
  "\n\n"
  "We hope this guide was helpful. Have fun using Mace!"
;

struct defkeybinding {
	uint8_t *key, *cmd;
};

static struct defkeybinding defaultkeybindings[] = {
	{ (uint8_t *) "Tab",         (uint8_t *) "tab" },
	{ (uint8_t *) "Return",      (uint8_t *) "return" },
	{ (uint8_t *) "Delete",      (uint8_t *) "del" },
	{ (uint8_t *) "BackSpace",   (uint8_t *) "back" },
	{ (uint8_t *) "S-BackSpace", (uint8_t *) "back" },
	{ (uint8_t *) "Left",        (uint8_t *) "left" },
	{ (uint8_t *) "Right",       (uint8_t *) "right" },
	{ (uint8_t *) "Up",          (uint8_t *) "up" },
	{ (uint8_t *) "Down",        (uint8_t *) "down" },
	{ (uint8_t *) "C-z",         (uint8_t *) "undo" },
	{ (uint8_t *) "S-C-Z",       (uint8_t *) "redo" },
	{ (uint8_t *) "C-c",         (uint8_t *) "copy" },
	{ (uint8_t *) "C-x",         (uint8_t *) "cut" },
	{ (uint8_t *) "C-v",         (uint8_t *) "paste" },
	{ (uint8_t *) "C-s",         (uint8_t *) "save" },
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
  
  s = sequencenew(buf, l);
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
