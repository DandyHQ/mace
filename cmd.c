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

  if (m->mousefocus == NULL) {
    return;
  }

  if (getfilename(m->mousefocus->tab, filename, sizeof(filename)) == 0) {
    printf("filename not found\n");
    return;
  }

  s = m->mousefocus->tab->main->sequence;
  len = sequencegetlen(s);

  buf = malloc(len);
  if (buf == NULL) {
    printf("Failed to allocate buffer for file!\n");
    return;
  }

  len = sequenceget(s, 0, buf, len);
  
  fd = open((char *) filename, O_WRONLY|O_CREAT, 0666);
  if (fd < 0) {
    free(buf);
    return;
  }

  write(fd, buf, len);

  close(fd);
  free(buf);
}

static void
cmdopen(struct mace *m)
{

}

static void
cmdclose(struct mace *m)
{

}

static void
cmdcut(struct mace *m)
{

}

static void
cmdcopy(struct mace *m)
{

}

static void
cmdpaste(struct mace *m)
{

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
		  printf("command returned to cmd\n");
			return true;
		}
	}

	return false;
}
