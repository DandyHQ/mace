#include <string.h>
#include "mace.h"
#include "config.h"

static uint8_t message[] =
  "Welcome to the Mace text editor."
  "\n\n" 
  "We need to redo the guide."
  "\n\n"
;


struct mace *
macenew(void)
{
  const uint8_t name[] = "Mace";
  struct sequence *s;
  struct mace *m;
  struct tab *t;
  uint8_t *buf;
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

/* Should typing replace selections? Only if the cursos is in the selection? */

void
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

bool
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
handlekeypress(struct mace *m, keycode_t k)
{
	struct selection *sel;
	struct cursor *c;
	uint8_t s[16];
	size_t n, start;

	switch (k) {
	default:
		break;

	case KEY_tab:
		n = snprintf((char *) s, sizeof(s), "\t");
		return handletyping(m, s, n);

	case KEY_return:
		n = snprintf((char *) s, sizeof(s), "\n");
		return handletyping(m, s, n);
  
	case KEY_delete:
		if (m->selections == NULL) {
			for (c = m->cursors; c != NULL; c = c->next) {
				/* TODO: should delete one codepoint not one byte. 
				   sequence needs some functions for length of next and previous 
				   code point. */

				n = 1;
				start = c->pos;

				sequencedelete(c->tb->sequence, start, n);
				shiftselections(m, c->tb, start + n, -n);
				shiftcursors(m, c->tb, start + n, -n);
			}

			return true;
		}

		/* Fall through */
    
	case KEY_backspace:
		if (m->selections == NULL) {
			for (c = m->cursors; c != NULL; c = c->next) {
				/* TODO: should delete one codepoint not one byte. 
				   sequence needs some functions for length of next and previous 
				   code point. */

				n = 1;

				if (c->pos < n) {
					continue;
				}

				start = c->pos - n;

				sequencedelete(c->tb->sequence, start, n);
				shiftselections(m, c->tb, start, -n);
				shiftcursors(m, c->tb, start, -n);
			}

			return true;
		}

		for (sel = m->selections; sel != NULL; sel = sel->next) {
			start = sel->start;
			n = sel->end - sel->start;

			sequencedelete(sel->tb->sequence, start, n);

			shiftselections(m, sel->tb, start, -n);

			for (c = m->cursors; c != NULL; c = c->next) {
				if (c->tb != sel->tb) continue;
				if (sel->end < c->pos) {
					c->pos -= n;
				} else if (sel->start < c->pos && c->pos < sel->end) {
					c->pos = sel->end;
				}
			}
		}
			
		selectionremoveall(m);

		return true;
	}

	return false;
}

bool
handlekeyrelease(struct mace *m, keycode_t k)
{
	return false;
}
