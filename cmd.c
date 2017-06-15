#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pwd.h>

#include "mace.h"
#include "config.h"

struct cmd {
	const char *name;
	void (*func)(struct mace *mace);
};

static void
cmdsave(struct mace *m)
{

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
	{ "save",      &cmdsave },
	{ "open",      &cmdopen },
	{ "close",     &cmdclose },
	{ "cut",       &cmdcut },
	{ "copy",      &cmdcopy },
	{ "paste",     &cmdpaste },
};

void
command(struct mace *mace, const uint8_t *s)
{
	int i;

  printf("do command '%s'\n", s);

	for (i = 0; i < sizeof(cmds) / sizeof(struct cmd); i++) {
		if (strcmp((const char *) s, cmds[i].name) == 0) {
			cmds[i].func(mace);
			return;
		}
	}
}
