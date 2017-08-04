#include <string.h>

#include "mace.h"
#include "config.h"

#include "resources/tomlc99/toml.h"

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

static const char* help_message =
"Usage: %s [-hv] [-c config] [file ...]\n"
"\n"
"Options:\n"
"  -h, --help\n"
"        Show help options.\n"
"  -v, --version\n"
"        Show the mace version.\n"
"  -c config, --config config\n"
"        Override the default config.\n";

static struct tab *
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

static char *
findconfig(void)
{
	char *s;
	
	s = malloc(sizeof(char) * 512);
	if (s == NULL) {
		return NULL;
	}
	
	snprintf(s, 512, "mace.toml");
	return s;
}

static bool
applydefaultconfig(struct mace *m)
{
	struct tab *t;
	size_t i;
	
	t = maketutorialtab(m);
	if (t == NULL) {
		fprintf(stderr, "Error: failed to create tutorial tab!\n");
		return false;
	}
	
	paneaddtab(m->pane, t, -1);
	
	for (i = 0; 
	     i < sizeof(defaultkeybindings) / sizeof(defaultkeybindings[0]); 
	     i++) {
	     
		maceaddkeybinding(m, 
		                  defaultkeybindings[i].key, 
		                  defaultkeybindings[i].cmd);
	}
	
	return true;
}

static bool
applyconfigmace(struct mace *m, toml_table_t *conf) 
{
	const char *raw;
	struct tab *t;
	char *str;
	
	raw = toml_raw_in(conf, "actionbar");
	if (raw != NULL) {
		if (toml_rtos(raw, &str) != 0)  {
			fprintf(stderr, "Error in config, bad value '%s' for actionbar!\n",
			        raw);
			return false;
		}
		
		free(m->defaultaction);
		m->defaultaction = str;
	}
	
	raw = toml_raw_in(conf, "defaulttab");
	if (raw != NULL) {			
		if (m->pane->tabs == NULL) {
			if (strcmp(raw, "\"empty\"") == 0) {
				t = tabnewempty(m, "scratch");
				
			} else if (strcmp(raw, "\"tutorial\"") == 0) {
				t = maketutorialtab(m);
				
			} else {
				/* Default to tutorial. */
				t = maketutorialtab(m);
			}
				
			if (t == NULL) {
				fprintf(stderr, "defaulttab has invalid value '%s'\n", raw);
				fprintf(stderr, "should be one of: tutorial, empty\n");
				return false;
		  }
			  
		  paneaddtab(m->pane, t, -1);
	  }
	}
	
	return true;
}

static bool
applyconfigkeybindings(struct mace *m, toml_table_t *conf)
{
	const char *key, *raw;
	char *cmd;
	int i;
	
	i = 0;
	while ((key = toml_key_in(conf, i++)) != NULL) {
		raw = toml_raw_in(conf, key);
		if (raw == NULL) continue;
		if (toml_rtos(raw, &cmd) != 0) {
			fprintf(stderr, "Error in config, bad value '%s' for keybinding %s!\n",
			        raw, key);
			return false;
		}
		
		maceaddkeybinding(m, (uint8_t *) key, (uint8_t *) cmd);
		
		free(cmd);
	}
	
	return true;
}

static bool
applyconfig(struct mace *m, toml_table_t *conf) 
{
	toml_table_t *mace, *keybindings;
	size_t i;
	
	mace = toml_table_in(conf, "mace");
	if (mace != NULL && !applyconfigmace(m, mace)) {
		return false;
  }
  
	keybindings = toml_table_in(conf, "keybindings");
	if (keybindings != NULL) {
		if (!applyconfigkeybindings(m, keybindings)) {
			return false;
		}
  } else {
		for (i = 0; 
		     i < sizeof(defaultkeybindings) / sizeof(defaultkeybindings[0]); 
		     i++) {
			maceaddkeybinding(m, 
			                  defaultkeybindings[i].key, 
			                  defaultkeybindings[i].cmd);
		}
	}
	
  return true;
}

int
main(int argc, char **argv)
{
	char *configfile = NULL;
  struct optparse options;
	toml_table_t *conf;
	char errbuf[200];
	struct mace *m;
	struct tab *t;
  int option, r;
	char *arg;
	FILE *f;
	
	struct optparse_long longopts[] = {
		{"help",      'h',   OPTPARSE_NONE},
		{"version",   'v',   OPTPARSE_NONE},
		{"config",    'c',   OPTPARSE_REQUIRED},
		{0}
	};
	
	m = macenew();
  if (m == NULL) {
    fprintf(stderr, "Failed to initalize mace!");
    return EXIT_FAILURE;
  }
  
  optparse_init(&options, argv);

	while ((option = optparse_long(&options, longopts, NULL)) != -1) {
		switch(option) {
		case 'h':
			printf(help_message, argv[0]);
			macefree(m);
			return EXIT_SUCCESS;
			
		case 'v':
			printf("Mace %s\n", VERSION_STR);
			macefree(m);
			return EXIT_SUCCESS;
		
		case 'c':
			configfile = options.optarg;
			break;
			
		case '?':
			fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			macefree(m);
			return EXIT_FAILURE;
		}
	}

	/* Load remaining arguments as tabs. */
	
	while ((arg = optparse_arg(&options))) {
		t = tabnewfromfile(m, (uint8_t *) arg);
		if (t == NULL) {
			continue;
		}
		
		paneaddtab(m->pane, t, -1);
	}

	if (configfile == NULL) {
		configfile = findconfig();
	}
	
	if (configfile != NULL) {
		f = fopen(configfile, "r");
		if (f <= 0) {
			fprintf(stderr, "Failed to load config file!\n");
			macefree(m);
			return EXIT_FAILURE;
		}
	
		conf = toml_parse_file(f, errbuf, sizeof(errbuf));
		fclose(f);
		if (conf == NULL) {
			fprintf(stderr, "Error reading '%s': %s\n", configfile, errbuf);
			macefree(m);
			return EXIT_FAILURE;
		}
	
		if (!applyconfig(m, conf)) {
			macefree(m);
			toml_free(conf);
			return EXIT_FAILURE;
		}
		
	} else {
		if (!applydefaultconfig(m)) {
			macefree(m);
			return EXIT_FAILURE;
		}
	}
	
	m->pane->focus = m->pane->tabs;
	
	r = dodisplay(m);
	
	macefree(m);

  return r;
}
