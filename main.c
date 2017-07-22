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

#define OPTPARSE_IMPLEMENTATION
#include "resources/optparse/optparse.h"

static const char* help_message = "\
Usage:\n\
  mace [OPTIONS...] [FILES...]\n\
\n\
Options:\n\
  -h, --help                        Show help options\n\
  -v, --version                     Show the mace version\n\
";

struct tab *
maketutorialtab(struct mace *m)
{
  const uint8_t name[] = "Mace";
  struct sequence *s;
  struct tab *t;
  uint8_t *buf;
  size_t l;

  buf = malloc(sizeof(message));
  if (buf == NULL) {
    return NULL;
  }

  l = snprintf((char *) buf, sizeof(message), "%s", message);
  
  s = sequencenew(buf, l);
  if (s == NULL) {
    free(buf);
    return NULL;
  }

  t = tabnew(m, name, name, s);
  if (t == NULL) {
  	sequencefree(s);
    return NULL;
  }
  
  return t;
}

int
main(int argc, char **argv)
{
	bool help = false, version = false;
  struct optparse options;
  int option, r, i;
	struct mace *m;
	struct tab *t;

	struct optparse_long longopts[] = {
		{"help", 'h', OPTPARSE_NONE},
		{"version", 'v', OPTPARSE_NONE},
		{0}
	};
  
  optparse_init(&options, argv);

	while ((option = optparse_long(&options, longopts, NULL)) != -1) {
		switch(option) {
		case 'h':
			help = true;
			break;
		case 'v':
			version = true;
			break;
		case '?':
			fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			return EXIT_FAILURE;
		}
	}

	if (help) {
		puts(help_message);
		return EXIT_SUCCESS;

	} else if (version) {
		printf("Mace %s\n", VERSION_STR);
		return EXIT_SUCCESS;
	}
	
	m = macenew();
  if (m == NULL) {
    fprintf(stderr, "Failed to initalize mace!");
    return EXIT_FAILURE;
  }

	for (i = 0; i < sizeof(defaultkeybindings) / sizeof(defaultkeybindings[0]); i++) {
		maceaddkeybinding(m, defaultkeybindings[i].key, defaultkeybindings[i].cmd);
	}

	t = NULL;
	for (i = 1; i < argc; i++) {
		t = tabnewfromfile(m, (uint8_t *) argv[i]);
		if (t == NULL) {
			continue;
		}
		
		paneaddtab(m->pane, t, -1);
	}

	if (t == NULL) {
		t = maketutorialtab(m);
		if (t == NULL) {
			fprintf(stderr, "%s: Failed to open tutorial tab!\n", argv[0]);
			macefree(m);
			return EXIT_FAILURE;
		}
		
		paneaddtab(m->pane, t, -1);
	}
	
	m->pane->focus = t;
	
	r = dodisplay(m);
	
	macefree(m);

  return r;
}
