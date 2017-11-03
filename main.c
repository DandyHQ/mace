#include <string.h>

#include "mace.h"
#include "config.h"

#include "resources/tomlc99/toml.h"

struct defkeybinding {
  uint8_t *key, *cmd;
};

static struct defkeybinding defaultkeybindings[] = {
  { (uint8_t *) "Tab",         (uint8_t *) "tab" },
  { (uint8_t *) "S-Tab",       (uint8_t *) "shifttab" },
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

static const char *help_message =
  "Usage: %s [-hv] [-c config] [file ...]\n"
  "\n"
  "Options:\n"
  "  -h, --help\n"
  "        Show help options.\n"
  "  -v, --version\n"
  "        Show the mace version.\n"
  "  -c config, --config config\n"
  "        Override the default config.\n";

static bool
applydefaultconfig(struct mace *m)
{
  size_t i;

  for (i = 0;
       i < sizeof(defaultkeybindings) / sizeof(
         defaultkeybindings[0]);
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
  char *str;
  int64_t i;
  
  raw = toml_raw_in(conf, "actionbar");

  if (raw != NULL) {
    if (toml_rtos(raw, &str) != 0)  {
      fprintf(stderr, "Bad value '%s' for actionbar!\n", raw);
      return false;
    }

    free(m->defaultaction);
    m->defaultaction = (uint8_t *) str;
  }

  raw = toml_raw_in(conf, "colbar");

  if (raw != NULL) {
    if (toml_rtos(raw, &str) != 0)  {
      fprintf(stderr, "Bad value '%s' for colbar!\n", raw);
      return false;
    }

    free(m->defaultcol);
    m->defaultcol = (uint8_t *) str;
  }
  
  raw = toml_raw_in(conf, "mainbar");

  if (raw != NULL) {
    if (toml_rtos(raw, &str) != 0)  {
      fprintf(stderr, "Bad value '%s' for mainbar!\n", raw);
      return false;
    }

    free(m->defaultmain);
    m->defaultmain = (uint8_t *) str;
  }
  
  raw = toml_raw_in(conf, "font");

  if (raw != NULL) {
    if (toml_rtos(raw, &str) != 0)  {
      fprintf(stderr, "Bad value '%s' for font!\n", raw);
      return false;
    }

    if (!fontset(m->font, str)) {
      fprintf(stderr, "Failed to find font '%s'\n", str);
      free(str);
      return false;
    }

    free(str);
  }

  raw = toml_raw_in(conf, "tabwidth");

  if (raw != NULL) {
    if (toml_rtoi(raw, &i) != 0)  {
      fprintf(stderr, "Bad value '%s' for tabwidth!\n", raw);
      return false;
    }

    if (!fontsettabwidth(m->font, (size_t) i)) {
      fprintf(stderr, "Failed to set tab width to %li\n", i);
      return false;
    }
  }

  return true;
}

static bool
applyconfigscroll(struct mace *m, toml_table_t *conf)
{
  const char *key, *raw;
  scroll_action_t *b;
  char *str;
  int i;
  i = 0;

  while ((key = toml_key_in(conf, i++)) != NULL) {
    raw = toml_raw_in(conf, key);

    if (strstr(key, "Button-") != NULL) {
      /* For now. Button presses need to be redone to support
       * modifiers and so on. */
      if (strcmp(key, "Button-1") == 0) {
        b = &m->scrollleft;
      } else if (strcmp(key, "Button-2") == 0) {
        b = &m->scrollmiddle;
      } else if (strcmp(key, "Button-3") == 0) {
        b = &m->scrollright;
      } else {
        b = &m->scrollleft;
      }

      if (toml_rtos(raw, &str) != 0)  {
        fprintf(stderr, "Bad value '%s' for scroll %s!\n",
                raw, key);
        return false;
      }

      if (strcmp(str, "up") == 0) {
        *b = SCROLL_up;
      } else if (strcmp(str, "down") == 0) {
        *b = SCROLL_down;
      } else if (strcmp(str, "immediate") == 0) {
        *b = SCROLL_immediate;
      } else {
        fprintf(stderr, "Bad value %s for scrollmiddle!\n", str);
        free(str);
        return false;
      }

      free(str);
    } else {
      fprintf(stderr,
              "Unknown configuration in scroll table '%s'\n", key);
      return false;
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

    if (raw == NULL) {
      continue;
    }

    if (toml_rtos(raw, &cmd) != 0) {
      fprintf(stderr, "Bad value '%s' for keybinding %s!\n",
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
  toml_table_t *mace, *scroll, *keybindings;
  size_t i;
  mace = toml_table_in(conf, "mace");

  if (mace != NULL && !applyconfigmace(m, mace)) {
    return false;
  }

  scroll = toml_table_in(conf, "scroll");

  if (scroll != NULL && !applyconfigscroll(m, scroll)) {
    return false;
  }

  keybindings = toml_table_in(conf, "keybindings");

  if (keybindings != NULL) {
    if (!applyconfigkeybindings(m, keybindings)) {
      return false;
    }
  } else {
    for (i = 0;
         i < sizeof(defaultkeybindings) / sizeof(
           defaultkeybindings[0]);
         i++) {
      maceaddkeybinding(m,
                        defaultkeybindings[i].key,
                        defaultkeybindings[i].cmd);
    }
  }

  return true;
}

static bool
findconfig(FILE **f, char *path, size_t l)
{
  char *home, *xdg;

  if (*path != 0) {
    *f = fopen(path, "r");

    if (*f != NULL) {
      return true;
    } else {
      fprintf(stderr, "Failed to load config file %s!\n", path);
      return false;
    }
  }

  home = getenv("HOME");
  xdg = getenv("XDG_CONFIG_HOME");

  if (xdg != NULL) {
    snprintf(path, l, "%s/mace/config.toml", xdg);
    *f = fopen(path, "r");

    if (*f != NULL) {
      return true;
    }
  }

  if (home != NULL) {
    snprintf(path, l, "%s/.config/mace/config.toml", home);
    *f = fopen(path, "r");

    if (*f != NULL) {
      return true;
    }

    snprintf(path, l, "%s/.mace.toml", home);
    *f = fopen(path, "r");

    if (*f != NULL) {
      return true;
    }
  }

  snprintf(path, l, "/etc/mace.toml");
  *f = fopen(path, "r");

  if (*f != NULL) {
    return true;
  }

  return false;
}

int
main(int argc, char **argv)
{
  char configpath[1024] = { '\0' };
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
  
  optparse_init(&options, argv);

  while ((option = optparse_long(&options, longopts,
                                 NULL)) != -1) {
    switch (option) {
    case 'h':
      printf(help_message, argv[0]);
      return EXIT_SUCCESS;

    case 'v':
      printf("Mace %s\n", VERSION_STR);
      return EXIT_SUCCESS;

    case 'c':
      snprintf(configpath, sizeof(configpath), "%s",
               options.optarg);
      break;

    case '?':
      fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
      return EXIT_FAILURE;
    }
  }

  if (findconfig(&f, configpath, sizeof(configpath))) {
    conf = toml_parse_file(f, errbuf, sizeof(errbuf));
    fclose(f);

    if (conf == NULL) {
      fprintf(stderr, "Error parsing '%s': %s\n", configpath,
              errbuf);
    }
	} else {
		conf = NULL;
	}
	
  m = macenew();

  if (m == NULL) {
    fprintf(stderr, "Failed to initalize mace!");
    return EXIT_FAILURE;
  }
  
  if (conf != NULL) {
  	if (!applyconfig(m, conf)) {
  		fprintf(stderr, "Error(s) in config '%s'.\n", configpath);
   	 toml_free(conf);
   	 macefree(m);
   	 return EXIT_FAILURE;
  	}
  } else {
    if (!applydefaultconfig(m)) {
      macefree(m);
      return EXIT_FAILURE;
    }
  }

	m->columns = columnnew(m);
	
	if (m->columns == NULL) {
		fprintf(stderr, "Failed to create column!\n");
    return EXIT_FAILURE;
	}
	
  /* Load remaining arguments as tabs. */
	while ((arg = optparse_arg(&options))) {
    t = tabnewfromfile(m, (uint8_t *) arg);

    if (t == NULL) {
			fprintf(stderr, "Error opening %s!\n", arg);
			macefree(m);
   	 return EXIT_FAILURE;
		}

    columnaddtab(m->columns, t);
  }

	if (conf != NULL) {
		toml_free(conf);
  }
  
  r = dodisplay(m);
  macefree(m);
  return r;
}
