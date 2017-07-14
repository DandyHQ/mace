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
  
	if (l >= len) {
		return 0;
	}

  sequenceget(t->action->sequence, 0, buf, l);
  buf[l] = 0;

  return l;
}

static void
cmdsave(struct mace *m)
{
  uint8_t filename[PATH_MAX], *buf;
  struct sequence *s;
  size_t len;
  int fd;

  if (m->mousefocus == NULL) {
    return;
  }

  if (getfilename(m->mousefocus->tab, filename, sizeof(filename)) == 0) {
    printf("filename not found\n");
    return;
  }

  s = m->mousefocus->tab->main->sequence;
  len = sequencelen(s);

  buf = malloc(len);
  if (buf == NULL) {
    printf("Failed to allocate buffer for file!\n");
    return;
  }

  len = sequenceget(s, 0, buf, len);
  
  fd = open((char *) filename, O_WRONLY|O_TRUNC|O_CREAT, 0666);
  if (fd < 0) {
    printf("Failed to open %s\n", filename);
    free(buf);
    return;
  }

  if (write(fd, buf, len) != len) {
    printf("Failed to write to %s\n", filename);
  }

  close(fd);
  free(buf);
}

static void
openselection(struct mace *m, struct selection *s)
{
  uint8_t name[PATH_MAX];
  struct tab *t;
  size_t len;

  len = s->end - s->start;
	if (len + 1 >= sizeof(name)) {
		fprintf(stderr, "Selection is too long to be a name!\n");
		return;
	}

  len = sequenceget(s->textbox->sequence, s->start, name, len);
	name[len] = 0;

  t = tabnewfromfile(m, name, len);
  if (t == NULL) {
    return;
  }

  paneaddtab(s->textbox->tab->pane, t, -1);

  s->textbox->tab->pane->focus = t;
  m->keyfocus = t->main;
  m->mousefocus = t->main;
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
		t = s->textbox;

    clipboardlen = s->end - s->start;
    clipboard = realloc(clipboard, clipboardlen);
    if (clipboard == NULL) {
      clipboardlen = 0;
      return;
    }

    clipboardlen = sequenceget(t->sequence, s->start, 
		                                           clipboard, clipboardlen);

    sequencedelete(t->sequence, s->start, clipboardlen);
    
    if (s->start <= t->cursor || t->cursor <= s->end) {
      t->cursor = s->start;
    }

    selectionremove(s);
  }
}

static void
cmdcopy(struct mace *m)
{
  struct textbox *t;
  struct selection *s;

  for (s = m->selections; s != NULL; s = s->next) {
    if (s->type != SELECTION_normal) continue;
		t = s->textbox;
    
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
cmdpaste(struct mace *m)
{
  struct textbox *t;
  
  if (m->keyfocus == NULL || clipboard == NULL) {
    return;
  }

  t = m->keyfocus;
  sequenceinsert(t->sequence, t->cursor, clipboard, clipboardlen);
  t->cursor += clipboardlen;
}

static struct cmd cmds[] = {
	{ "save",      cmdsave },
	{ "open",      cmdopen },
	{ "close",     cmdclose },
	{ "cut",       cmdcut },
	{ "copy",      cmdcopy },
	{ "paste",     cmdpaste },
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
