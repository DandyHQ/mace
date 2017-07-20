#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

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
  size_t l;

  for (l = 0; l + 1 < len; l++) {
		if (sequenceget(t->action->sequence, l, buf + l, sizeof(uint8_t)) 
		    != sizeof(uint8_t)) {
			break;
		} else if (buf[l] == ':') {
     	break;
    }
  }
  
	buf[l] = 0;
  
  return l;
}

static void
cmdsave(struct mace *m)
{
  uint8_t filename[1024], *buf;
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
openselection(struct mace *m, struct cursel *s)
{
  uint8_t name[1024];
  struct tab *t;
  size_t len;

  len = s->end - s->start;
	if (len + 1 >= sizeof(name)) {
		fprintf(stderr, "Selection is too long to be a name!\n");
		return;
	}

  len = sequenceget(s->tb->sequence, s->start, name, len);
	name[len] = 0;

  t = tabnewfromfile(m, name);
  if (t == NULL) {
    return;
  }

  paneaddtab(s->tb->tab->pane, t, -1);

  s->tb->tab->pane->focus = t;
}

static void
cmdopen(struct mace *m)
{
  struct cursel *s;

  for (s = m->cursels; s != NULL; s = s->next) {
		if ((s->type & CURSEL_nrm) != 0 && s->start != s->end) {
	    openselection(m, s);
		}
  }
}

static void
openfile(struct mace *m, const uint8_t *filename)
{
	struct tab *t;
	struct cursel *s = m->cursels;
	
	t = tabnewfromfile(m, filename);
	if (t == NULL) {
		return;
	}
	
	paneaddtab(s->tb->tab->pane, t, -1);
	
	s->tb->tab->pane->focus = t;
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
  struct cursel *c, *n;
	struct textbox *t;
	size_t start, len;

  for (c = m->cursels; c != NULL; c = n) {
    n = c->next;
    if ((c->type & CURSEL_cmd) != 0 || c->start == c->end) continue;
		t = c->tb;

		start = c->start;
		len = c->end - c->start;

		clipboard = realloc(clipboard, len);
		if (clipboard == NULL) {
			clipboardlen = 0;
			return;
		}

		clipboardlen = sequenceget(t->sequence, start, 
		                           clipboard, len);

		sequencereplace(t->sequence, start, c->end, NULL, 0);
		textboxplaceglyphs(t);
    
		shiftcursels(m, t, start, -clipboardlen);
		c->start = start;
		c->end = start;
		c->cur = 0;		
  }
}

static void
cmdcopy(struct mace *m)
{
  struct textbox *t;
  struct cursel *c;

  for (c = m->cursels; c != NULL; c = c->next) {
    if ((c->type & CURSEL_cmd) != 0 || c->start == c->end) continue;
		t = c->tb;
    
    clipboardlen = c->end - c->start;
    clipboard = realloc(clipboard, clipboardlen);
    if (clipboard == NULL) {
      clipboardlen = 0;
      return;
    }

    clipboardlen = sequenceget(t->sequence, c->start,
			                         clipboard, clipboardlen);
  }
}

static void
insertstring(struct mace *m, uint8_t *s, size_t n)
{
	struct cursel *c;
	size_t start;

	for (c = m->cursels; c != NULL; c = c->next) {
		start = c->start;
		
		sequencereplace(c->tb->sequence, start, c->end, s, n);
		textboxplaceglyphs(c->tb);

		shiftcursels(m, c->tb, start, n - (c->end - start));

		c->start = start + n;
		c->end = start + n;
		c->cur = 0;
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
	struct cursel *c;
	size_t start, n;
	int32_t code;

	for (c = m->cursels; c != NULL; c = c->next) {
		start = c->start;
		n = c->end - c->start;

		if (n > 0) {
			sequencereplace(c->tb->sequence, start, c->end, NULL, 0);
		} else {
			n = sequencecodepoint(c->tb->sequence, start, &code);
			sequencereplace(c->tb->sequence, start, start + n, NULL, 0);
		}

		textboxplaceglyphs(c->tb);

		shiftcursels(m, c->tb, start, -n);
		c->start = start;
		c->end = start;
		c->cur = 0;
	}
}

static void
cmdback(struct mace *m)
{
	struct cursel *c;
	size_t start, n;
	int32_t code;

	for (c = m->cursels; c != NULL; c = c->next) {
		start = c->start;
		n = c->end - c->start;

		if (n > 0) {
			sequencereplace(c->tb->sequence, start, c->end, NULL, 0);
		} else {
			n = sequenceprevcodepoint(c->tb->sequence, start, &code);
			start -= n;
			sequencereplace(c->tb->sequence, start, start + n, NULL, 0);
		}

		textboxplaceglyphs(c->tb);

		shiftcursels(m, c->tb, start, -n);
		c->start = start;
		c->end = start;
		c->cur = 0;
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
	uint8_t buf[64];
	struct cursel *c;
	size_t start, n;
	
	for (c = m->cursels; c != NULL; c = c->next) {
		start = c->start;

		n = textboxindentation(c->tb, start, buf, sizeof(buf));

		sequencereplace(c->tb->sequence, start, c->end, (uint8_t *) "\n", 1);
		sequencereplace(c->tb->sequence, start + 1, start + 1, buf, n);
		textboxplaceglyphs(c->tb);

		shiftcursels(m, c->tb, start, 1 + n - (c->end - start));

		c->start = start + 1 + n;
		c->end = start + 1 + n;
		c->cur = 0;
	}
}

static void
cmdleft(struct mace *m)
{
	struct cursel *c;
	int32_t code;
	size_t n;

	for (c = m->cursels; c != NULL; c = c->next) {
		n = c->end - c->start;
		if (n == 0) {
			n = sequenceprevcodepoint(c->tb->sequence, c->start, &code);
			c->start -= n;
			c->end -= n;
		} else {
			c->end = c->start;
			c->cur = 0;
		}
	}
}

static void
cmdright(struct mace *m)
{
	struct cursel *c;
	int32_t code;
	size_t n;

	for (c = m->cursels; c != NULL; c = c->next) {
		n = c->end - c->start;
		if (n == 0) {
			n = sequencecodepoint(c->tb->sequence, c->start, &code);
			c->start += n;
			c->end += n;
		} else {
			c->start = c->end;
			c->cur = 0;
		}
	}
}

static void
cmdup(struct mace *m)
{
	struct cursel *c;
	size_t n;

	for (c = m->cursels; c != NULL; c = c->next) {
		n = c->end - c->start;
		if (n == 0) {
			c->start = c->end = textboxindexabove(c->tb, c->start);
		} else {
			c->end = c->start;
			c->cur = 0;
		}
	}
}

static void
cmddown(struct mace *m)
{
	struct cursel *c;
	size_t n;

	for (c = m->cursels; c != NULL; c = c->next) {
		n = c->end - c->start;
		if (n == 0) {
			c->start = c->end = textboxindexbelow(c->tb, c->start);
		} else {
			c->start = c->end;
			c->cur = 0;
		}
	}
}

static void
cmdjump(struct mace *m)
{
	printf("Not yet implimented\n");
}

/* These should all do one action per text box if a text 
   box has more than one cursels in it. 
   They also need to update the cursels in ways I do not know.
*/
   
static void
cmdundo(struct mace *m)
{
	struct cursel *c;
	
	for (c = m->cursels; c != NULL; c = c->next) {
		sequencechangeup(c->tb->sequence);
		textboxplaceglyphs(c->tb);
	}
}

static void
cmdredo(struct mace *m)
{
	struct cursel *c;
	
	for (c = m->cursels; c != NULL; c = c->next) {
		sequencechangedown(c->tb->sequence);
		textboxplaceglyphs(c->tb);
	}
}

static void
cmdundocycle(struct mace *m)
{
	struct cursel *c;
	
	for (c = m->cursels; c != NULL; c = c->next) {
		sequencechangecycle(c->tb->sequence);
		textboxplaceglyphs(c->tb);
	}
}



static bool
fileexists(const uint8_t *filename)
{
	uint8_t *name;
	
	/* I am not sure if basename allocates name or if it is a slice of filename. */
	name = (uint8_t *) basename((char *) filename);
	
	return access((const char *)name, R_OK) != -1;
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
	{ "left",       cmdleft },
  { "right",      cmdright },
	{ "up",         cmdup },
	{ "down",       cmddown },
	{ "jump",       cmdjump },
	{ "undo",       cmdundo },
	{ "redo",       cmdredo },
	{ "undocycle",  cmdundocycle },
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
	
	// If it's a file. Load that instead.
	if (fileexists(s)) {
		openfile(mace, s);
		return true;
	}

	fprintf(stderr, "no command '%s' found\n", s);
	return false;
}
