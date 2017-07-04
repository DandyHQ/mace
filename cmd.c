#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "mace.h"
#include "config.h"

struct cmd {
	const char *name;
	void (*func)(struct mace *mace);
};

static uint8_t *clipboard = NULL;
static size_t clipboardlen = 0;

static size_t
getfilename(struct tab *t, uint8_t *buf, size_t len)
{
  char c;
  size_t l;

  l = 0;
  while (l < len && sequenceget(t->action->sequence, l,
		    (uint8_t *) &c, sizeof(c)) == sizeof(c)) {
    if (c == ':') {
      break;
    } else {
      l++;
    }
  }
  
  sequenceget(t->action->sequence, 0, buf, l);
  
  return l;
}

static void
cmdsave(struct mace *m)
{
  uint8_t filename[PATH_MAX], *buf;
  struct sequence *s;
  size_t len;
  int fd;

	printf("save\n");

  if (m->mousefocus == NULL) {
    return;
  }

  if (getfilename(m->mousefocus->tab, filename, sizeof(filename)) == 0) {
    printf("filename not found\n");
    return;
  }

	printf("filename: %s\n", filename);

  s = m->mousefocus->tab->main->sequence;
  len = sequencelen(s);

	printf("alloc %zu\n", len);

  buf = malloc(len);
  if (buf == NULL) {
    printf("Failed to allocate buffer for file!\n");
    return;
  }

	printf("fill buffer\n");
  len = sequenceget(s, 0, buf, len);

	printf("open file\n");
  
  fd = open((char *) filename, O_WRONLY|O_TRUNC|O_CREAT, 0666);
  if (fd < 0) {
    printf("Failed to open %s\n", filename);
    free(buf);
    return;
  }

	printf("write data\n");

  if (write(fd, buf, len) != len) {
    printf("Failed to write to %s\n", filename);
  }

	printf("close\n");
  close(fd);
	printf("free\n");
  free(buf);
	printf("file saved\n");
}

static void
openselection(struct mace *m, struct selection *s)
{
  uint8_t name[PATH_MAX];
  struct tab *t;
  size_t len;

  len = s->end - s->start;
  len = sequenceget(s->tb->sequence, s->start, name, len);

  t = tabnewfromfile(m, name, len);
  if (t == NULL) {
    return;
  }

  paneaddtab(s->tb->tab->pane, t, -1);

  s->tb->tab->pane->focus = t;
}

static void
cmdopen(struct mace *m)
{
  struct selection *s;

  for (s = m->selections; s != NULL; s = s->next) {
		if (s->type == SELECTION_normal) {
	    openselection(m, s);
		}
  }
}

static void
cmdclose(struct mace *m)
{
  struct pane *p;
  struct tab *t;

  if (m->mousefocus == NULL) {
    return;
  }

  t = m->mousefocus->tab;
  p = t->pane;

  paneremovetab(p, t);
  tabfree(t);

  if (p->tabs == NULL) {
    m->running = false;
  }
}

static void
cmdcut(struct mace *m)
{
  struct selection *s, *n;
	struct textbox *t;

  for (s = m->selections; s != NULL; s = n) {
    n = s->next;
    if (s->type != SELECTION_normal) continue;
		t = s->tb;

    clipboardlen = s->end - s->start;
    clipboard = realloc(clipboard, clipboardlen);
    if (clipboard == NULL) {
      clipboardlen = 0;
      return;
    }

    clipboardlen = sequenceget(t->sequence, s->start, 
		                                           clipboard, clipboardlen);

    sequencedelete(t->sequence, s->start, clipboardlen);
    
		/* TODO: shift cursors properly. This doesn't work if the cursor is
		    in the selection. It will shift it to before the selection unlike the
		    expected moving it to the start of the selection. */

		shiftcursors(m, t, s->start, -clipboardlen);

    selectionremove(m, s);
  }
}

static void
cmdcopy(struct mace *m)
{
  struct textbox *t;
  struct selection *s;

  for (s = m->selections; s != NULL; s = s->next) {
    if (s->type != SELECTION_normal) continue;
		t = s->tb;
    
    clipboardlen = s->end - s->start;
    clipboard = realloc(clipboard, clipboardlen);
    if (clipboard == NULL) {
      clipboardlen = 0;
      return;
    }

    clipboardlen = sequenceget(t->sequence, s->start,
			                         clipboard, clipboardlen);
  }
}

static void
deleteselections(struct mace *m)
{
	struct selection *sel;
	struct cursor *c;
	size_t start, n;

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
}

static void
insertstring(struct mace *m, uint8_t *s, size_t n)
{
	struct cursor *c;

	deleteselections(m);

	for (c = m->cursors; c != NULL; c = c->next) {
		sequenceinsert(c->tb->sequence, c->pos, s, n);
		shiftselections(m, c->tb, c->pos, n);
		shiftcursors(m, c->tb, c->pos, n);
	}
}

static void
cmdpaste(struct mace *m)
{
  if (clipboard != NULL) {
    insertstring(m, clipboard, clipboardlen);
  }

}

static void
cmddel(struct mace *m)
{
	struct cursor *c;
	size_t start, n;

	if (m->selections == NULL) {
		for (c = m->cursors; c != NULL; c = c->next) {
			n = sequencecodepointlen(c->tb->sequence, c->pos);

			start = c->pos;

			sequencedelete(c->tb->sequence, start, n);
			shiftselections(m, c->tb, start + n, -n);
			shiftcursors(m, c->tb, start + n, -n);
		}
	} else {
		deleteselections(m);
	}
}

static void
cmdback(struct mace *m)
{
	struct cursor *c;
	size_t start, n;

	if (m->selections == NULL) {
		for (c = m->cursors; c != NULL; c = c->next) {
			n = sequenceprevcodepointlen(c->tb->sequence, c->pos);

			start = c->pos - n;

			sequencedelete(c->tb->sequence, start, n);
			shiftselections(m, c->tb, start, -n);
			shiftcursors(m, c->tb, start, -n);
		}
	} else {
		deleteselections(m);
	}
}

static void
cmdtab(struct mace *m)
{
	insertstring(m, (uint8_t *) "\t", 1);
}

static void
cmdreturn(struct mace *m)
{
	insertstring(m, (uint8_t *) "\n", 1);
}

static struct cmd cmds[] = {
	{ "save",       cmdsave },
	{ "open",       cmdopen },
	{ "close",      cmdclose },
	{ "cut",        cmdcut },
	{ "copy",       cmdcopy },
	{ "paste",      cmdpaste },
	{ "back",       cmdback },
	{ "del",        cmddel },
	{ "tab",        cmdtab },
	{ "return",     cmdreturn },
};

bool
command(struct mace *mace, const uint8_t *s)
{
	int i;
	
	for (i = 0; i < sizeof(cmds) / sizeof(struct cmd); i++) {
		if (strcmp((const char *) s, cmds[i].name) == 0) {
 		  cmds[i].func(mace);
			return true;
		}
	}

	return false;
}
