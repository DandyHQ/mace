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
  bool (*func)(struct mace *mace);
};


static bool
cmdquit(struct mace *m)
{
  m->running = false;
  return false;
}

static bool
cmdnewcol(struct mace *m)
{
  struct column *c, *p;
  int w;
  c = columnnew(m);

  if (c == NULL) {
    return false;
  }

  if (m->columns == NULL) {
    m->columns = c;
    return columnresize(c, m->width, m->height);

  } else {
    for (p = m->columns; p->next != NULL; p = p->next)
      ;

    p->next = c;
    w = p->width;

    if (!columnresize(p, w / 2, m->height)) {
      return false;

    } else if (!columnresize(c, w - p->width, m->height)) {
      return false;

    } else {
      return true;
    }
  }
}

static bool
cmddelcol(struct mace *m)
{
	struct column *c, *p;
	
	if (m->mousefocus == NULL) {
		return false;
	} else if (m->mousefocus->tab != NULL) {
		c = m->mousefocus->tab->column;
	} else if (m->mousefocus->column != NULL) {
		c = m->mousefocus->column;
	} else {
		return false;
	}
	
	if (m->columns == c) {
    if (c->next != NULL && 
        !columnresize(c->next, c->width + c->next->width, m->height)) {
    	return false;
    }
    
    m->columns = c->next;
    
  } else {
    for (p = m->columns; p->next != c; p = p->next)
      ;

    if (!columnresize(p, p->width + c->width, m->height)) {
      return false;
    }
    
    p->next = c->next;
  }
	
	columnfree(c);
	m->mousefocus = NULL;
	return true;
}

static bool
addtab(struct mace *m, struct tab *t)
{
  struct column *c;
  
	if (m->mousefocus != NULL) {
  	if (m->mousefocus->tab != NULL) {
   	 c = m->mousefocus->tab->column;
		} else if (m->mousefocus->column != NULL) {
   	 c = m->mousefocus->column;
		} else {
			c = NULL;
		}
			
  } else {
    for (c = m->columns; c != NULL && c->next != NULL; c = c->next)
      ;
  }

  if (c == NULL) {
  	if (!cmdnewcol(m)) {
    	return false;
    } else {
    	c = m->columns;
    	if (c == NULL) {
    		return false;
    	}
    }
  }
  
  return columnaddtab(c, t);
}

static bool
cmdscratch(struct mace *m)
{
  struct tab *t;

  t = tabnewempty(m, (uint8_t *) "*scratch*");

  if (t == NULL) {
    return false;
  } else if (!addtab(m, t)) {
  	tabfree(t);
  	return false;
  } else {
  	return true;
  } 
}

static size_t
getfilename(struct tab *t, uint8_t *buf, size_t len)
{
  size_t l;

  for (l = 0; l + 1 < len; l++) {
    if (sequenceget(t->action->sequence, l, buf + l,
                    sizeof(uint8_t))
        != sizeof(uint8_t)) {
      break;

    } else if (buf[l] == ':') {
      break;
    }
  }

  buf[l] = 0;
  return l;
}

static bool
cmdsave(struct mace *m)
{
  uint8_t filename[1024], *buf;
  struct sequence *s;
  size_t len;
  bool r;
  int fd;

  if (m->mousefocus == NULL || m->mousefocus->tab == NULL) {
    return false;
  }

  if (getfilename(m->mousefocus->tab, filename,
                  sizeof(filename)) == 0) {
    printf("filename not found\n");
    return false;
  }

  s = m->mousefocus->tab->main->sequence;
  len = sequencelen(s);
  buf = malloc(len);

  if (buf == NULL) {
    printf("Failed to allocate buffer for file!\n");
    return false;
  }

  len = sequenceget(s, 0, buf, len);
  fd = open((char *) filename, O_WRONLY | O_TRUNC | O_CREAT,
            0666);

  if (fd < 0) {
    printf("Failed to open %s\n", filename);
    free(buf);
    return false;
  }

  r = true;

  if (write(fd, buf, len) != len) {
    printf("Failed to write to %s\n", filename);
    r = false;
  }

  close(fd);
  free(buf);
  return r;
}

static bool
openpath(struct mace *m, const uint8_t *path)
{
  struct tab *t;
  t = tabnewfrompath(m, path);

  if (t == NULL) {
    return false;
  }

	if (!addtab(m, t)) {
		tabfree(t);
		return false;
	} else {
		return true;
	}
}

static bool
openselection(struct mace *m, struct cursel *s)
{
  uint8_t name[1024];
  size_t len;
  len = s->end - s->start;

  if (len + 1 >= sizeof(name)) {
    fprintf(stderr, "Selection is too long to be a name!\n");
    return false;
  }

  len = sequenceget(s->tb->sequence, s->start, name, len);
  name[len] = 0;
  return openpath(m, name);
}

static bool
cmdopen(struct mace *m)
{
  struct cursel *s;
  int o = 0, f = 0;

  for (s = m->cursels; s != NULL; s = s->next) {
    if ((s->type & CURSEL_nrm) != 0 && s->start != s->end) {
      if (openselection(m, s)) {
        o++;

      } else {
        f++;
      }
    }
  }

  return o > 0 && f == 0;
}

static bool
cmddel(struct mace *m)
{
  struct tab *t;

  if (m->mousefocus != NULL && m->mousefocus->tab != NULL) {
    t = m->mousefocus->tab;
    m->mousefocus = NULL;

  } else {
    return false;
  }

  columnremovetab(t->column, t);
  tabfree(t);
  return true;
}

static bool
cmdcut(struct mace *m)
{
  struct cursel *c;
  struct textbox *t;
  size_t start, len;
  uint8_t *data;
  c = m->cursels;

  if (c == NULL || (c->type & CURSEL_cmd) != 0
      || c->start == c->end) {
    return false;
  }

  t = c->tb;
  start = c->start;
  len = c->end - c->start;
  data = malloc(len);

  if (data == NULL) {
    return false;
  }

  len = sequenceget(t->sequence, start,
                    data, len);
  setclipboard(data, len);
  sequencereplace(t->sequence, start, c->end, NULL, 0);
  textboxplaceglyphs(t);
  shiftcursels(m, t, start, -len);
  c->start = start;
  c->end = start;
  c->cur = 0;
  return true;
}

static bool
cmdcopy(struct mace *m)
{
  struct textbox *t;
  struct cursel *c;
  uint8_t *data;
  size_t len;
  c = m->cursels;

  if (c == NULL || (c->type & CURSEL_cmd) != 0
      || c->start == c->end) {
    return false;
  }

  t = c->tb;
  len = c->end - c->start;
  data = malloc(len);

  if (data == NULL) {
    return false;
  }

  len = sequenceget(t->sequence, c->start,
                    data, len);
  setclipboard(data, len);
  return true;
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

static bool
cmdpaste(struct mace *m)
{
  uint8_t *data;
  size_t len;
  data = getclipboard(&len);

  if (data != NULL) {
    insertstring(m, data, len);
    return true;

  } else {
    return false;
  }
}

static bool
cmddelete(struct mace *m)
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

  return true;
}

static bool
cmdbackspace(struct mace *m)
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

  return true;
}

static bool
cmdtab(struct mace *m)
{
  struct cursel *c;
  size_t pos, count;

  for (c = m->cursels; c != NULL; c = c->next) {
    if (c->start == c->end) {
      pos = c->start;
      count = 1;
      sequencereplace(c->tb->sequence, c->start, c->start,
                      (uint8_t *) "\t", 1);

    } else {
      count = 0;

      for (pos = sequenceindexline(c->tb->sequence, c->start);
           pos <= c->end;
           pos = sequenceindexnextline(c->tb->sequence, pos)) {
        sequencereplace(c->tb->sequence, pos, pos,
                        (uint8_t *) "\t", 1);
        count++;
      }
    }

    c->start = c->start + 1;
    c->end = c->end + count;
    c->cur = c->end - c->start;
    shiftcursels(m, c->tb, c->start + 1, count);
    textboxplaceglyphs(c->tb);
  }

  return true;
}

static bool
cmdshifttab(struct mace *m)
{
  struct cursel *c;
  int count = 0;
  int32_t code;
  size_t pos;

  for (c = m->cursels; c != NULL; c = c->next) {
    pos = sequenceindexline(c->tb->sequence, c->start);

    while (pos <= c->end) {
      sequencecodepoint(c->tb->sequence, pos, &code);

      if (iswhitespace(code)) {
        sequencereplace(c->tb->sequence, pos, pos + 1, NULL, 0);
        count++;
        c->end = c->end - 1;
        c->cur = c->end - c->start;
      }

      pos = sequenceindexnextline(c->tb->sequence, pos);
    }

    shiftcursels(m, c->tb, c->start + 1, -count);
    textboxplaceglyphs(c->tb);
  }

  return true;
}

static bool
cmdreturn(struct mace *m)
{
  uint8_t buf[64];
  struct cursel *c;
  size_t start, n;

  for (c = m->cursels; c != NULL; c = c->next) {
    start = c->start;
    n = textboxindentation(c->tb, start, buf, sizeof(buf));
    sequencereplace(c->tb->sequence, start, c->end,
                    (uint8_t *) "\n", 1);
    sequencereplace(c->tb->sequence, start + 1, start + 1, buf,
                    n);
    textboxplaceglyphs(c->tb);
    shiftcursels(m, c->tb, start, 1 + n - (c->end - start));
    c->start = start + 1 + n;
    c->end = start + 1 + n;
    c->cur = 0;
  }

  return true;
}

static bool
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

  return true;
}

static bool
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

  return true;
}

static bool
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

  return true;
}

static bool
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

  return true;
}

static bool
cmdgoto(struct mace *m)
{
  struct textbox *t;
  struct cursel *s;
  uint8_t name[1024];
  size_t len;
  size_t lineno, colno;
  int scanned;
  size_t index;

  if (m->mousefocus == NULL || m->mousefocus->tab == NULL) {
    return false;
  }

  for (s = m->cursels; s != NULL; s = s->next) {
    if ((s->type & CURSEL_nrm) != 0 && s->start != s->end) {
      break;
    }
  }

  if (s == NULL) {
    return false;
  }

  len = s->end - s->start;

  if (len + 1 >= sizeof(name)) {
    fprintf(stderr, "Selection is too long to be a number!\n");
    return false;
  }

  len = sequenceget(s->tb->sequence, s->start, name, len);
  name[len] = 0;
  scanned = sscanf((const char *) name, "%zd:%zd", &lineno,
                   &colno);

  if (scanned < 1) {
    return false;

  } else if (scanned == 1) {
    colno = 0;
  }

  t = m->mousefocus->tab->main;
  index = sequenceindexpos(t->sequence, lineno, colno);
  t->start = sequenceindexline(t->sequence, index);
  textboxplaceglyphs(t);
  curselremoveall(m);
  curseladd(m, t, CURSEL_nrm, index);
  return true;
}

/* These should all do one action per text box if a text
   box has more than one cursels in it.
   They also need to update the cursels in ways I do not know.
*/

static bool
cmdundo(struct mace *m)
{
  struct cursel *c;

  for (c = m->cursels; c != NULL; c = c->next) {
    sequencechangeup(c->tb->sequence);
    textboxplaceglyphs(c->tb);
  }

  return true;
}

static bool
cmdredo(struct mace *m)
{
  struct cursel *c;

  for (c = m->cursels; c != NULL; c = c->next) {
    sequencechangedown(c->tb->sequence);
    textboxplaceglyphs(c->tb);
  }

  return true;
}

static bool
cmdundocycle(struct mace *m)
{
  struct cursel *c;

  for (c = m->cursels; c != NULL; c = c->next) {
    sequencechangecycle(c->tb->sequence);
    textboxplaceglyphs(c->tb);
  }

  return true;
}

static struct cmd cmds[] = {
  { "quit",       cmdquit },
  { "newcol",     cmdnewcol },
  { "delcol",     cmddelcol },
  { "scratch",    cmdscratch },

  { "save",       cmdsave },
  { "open",       cmdopen },
  { "del",        cmddel },

  { "cut",        cmdcut },
  { "copy",       cmdcopy },
  { "paste",      cmdpaste },

  { "backspace",  cmdbackspace },
  { "delete",     cmddelete },

  { "tab",        cmdtab },
  { "shifttab",   cmdshifttab },

  { "return",     cmdreturn },

  { "left",       cmdleft },
  { "right",      cmdright },
  { "up",         cmdup },
  { "down",       cmddown },

  { "goto",       cmdgoto },

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
      return cmds[i].func(mace);
    }
  }

  if (openpath(mace, s)) {
    return true;

  } else {
    fprintf(stderr,
            "Failed to run '%s' as a command or open as a file!\n", s);
    return false;
  }

  return false;
}
