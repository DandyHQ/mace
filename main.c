#include <string.h>

#include "mace.h"
#include "config.h"

#include "resources/tomlc99/toml.h"

static uint8_t message[] =
  "Welcome to the Mace text editor.\n"
  "\n"
  "Mace is a text editor built around the principles of simplicity, speed, and \n"
  "coherence. It is designed to be intuitive once you understand the basics.\n"
  "\n"
  "What you should see right now is two horizontal lines and a text box (containing this\n"
  "tutorial). The first line, at the top of the screen, is a list of tabs that Mace has\n"
  "opened for you. If you start Mace by giving it file names, Mace will start with those\n"
  "files open. Further on you will find out how to open new tabs. The next line is called\n"
  "the action bar. It is a text box the same as this one. It will wrap and grow as needed\n"
  "when you type into it. The purpose of the action bar is to have a place for commands that\n"
  "you are using with the current file and as a general scratch area. Commands will be\n"
  "explained later on. The main text box below is where files you have opened go.\n"
  "\n"
  "If you left click on some text, a cursor will be placed. Once you have a\n"
  "cursor you can type. If you middle click somewhere another cursor will be\n"
  "placed alongside any other cursors currently placed. Anything you now type\n"
  "or any commands you run will (generally) be applied to all the cursors you\n"
  "have placed. Left clicking will remove all additional cursors.\n"
  "\n"
  "You can scroll through the text either with a scroll wheel or by clicking\n"
  "the scroll bar. In Mace the scroll bar doesn't work how it does in many\n"
  "modern graphical user interfaces. In Mace a left click will scroll the\n"
  "text box up and a right will scroll down, while a middle click will take\n"
  "you to that immediate position, which you can then drag around. For left\n"
  "and right clicks, the position of the click in the scroll bar matters. A\n"
  "click near the top will result in a small movement while one near the bottom a\n"
  "larger one. So if you right click at the top of the scroll bar you will move\n"
  "down by a line while if you right click at the bottom you will move by a page.\n"
  "\n"
  "If you right click on text (either a word or a selection), one of two things\n"
  "will happen. It will be run as a command if it is one, and if not, then it will\n"
  "be opened as a file in a new tab. Commands generally work with cursors and\n"
  "selections but some work with where they were clicked. For example, 'save' saves\n"
  "the main text box of the tab that it was clicked in. Here is a list of commands\n"
  "currently in Mace:\n"
  "\n"
  "- save: Saves the current tab.\n"
  "- open: Opens new tabs of selected file names.\n"
  "- close: Closes the current tab or tabs with cursors in them.\n"
  "- cut: Cuts the first selection to the clip board.\n"
  "- copy: Copies the first selection to the clip board.\n"
  "- paste: Pastes the clipboard at all cursors.\n"
  "- back: Backspaces.\n"
  "- del: Deletes.\n"
  "- tab: Indents the selected regions or inserts a tab.\n"
  "- shifttab: Un-indents the selected regions.\n"
  "- return: Inserts a new line and indents.\n"
  "- left, right, up, down: Moves the cursors.\n"
  "- goto: Jumps to a line as specified by a selected lineno:colno.\n"
  "- undo: Moves back up the undo tree.\n"
  "- redo: Moves down the undo tree.\n"
  "- undocycle: Cycles through the branches of the undo tree.\n"
  "- scratch: Opens a new empty tab.\n"
  "\n"
  "So yes, Mace has unlimited undo in the form of a tree.\n"
  "\n"
  "Mace also has a configuration file that goes at ~/.config/mace/config.toml.\n"
  "There is an example in the source directory.\n"
  ;

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

static struct tab *
maketutorialtab(struct mace *m)
{
  const uint8_t name[] = "*tutorial*";
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

  t = tabnew(m, name, s);

  if (t == NULL) {
    sequencefree(s);
    return NULL;
  }

  return t;
}

static bool
applydefaultconfig(struct mace *m)
{
  size_t i;

/*
TODO
  if (m->pane->tabs == NULL) {
    t = maketutorialtab(m);

    if (t == NULL) {
      fprintf(stderr, "Error: failed to create tutorial tab!\n");
      return false;
    }

    paneaddtab(m->pane, t, -1);
  }
*/

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
  
  raw = toml_raw_in(conf, "defaulttab");

  if (raw != NULL) {
    if (toml_rtos(raw, &str) != 0)  {
      fprintf(stderr, "Bad value '%s' for defaulttab!\n", raw);
      return false;
    }

/* TODO
    if (m->pane->tabs == NULL) {
      if (strcmp(str, "empty") == 0) {
        t = tabnewempty(m, (uint8_t *) "*scratch*");
      } else if (strcmp(str, "tutorial") == 0) {
        t = maketutorialtab(m);
      } else {
        fprintf(stderr, "defaulttab has invalid value '%s'\n", raw);
        fprintf(stderr, "should be one of: tutorial, empty\n");
        free(str);
        return false;
      }

      if (t == NULL) {
        fprintf(stderr, "Failed to make default tab!\n");
        free(str);
        return false;
      }

      paneaddtab(m->pane, t, -1);
    }
    */

    free(str);
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
  m = macenew();

  if (m == NULL) {
    fprintf(stderr, "Failed to initalize mace!");
    return EXIT_FAILURE;
  }

  optparse_init(&options, argv);

  while ((option = optparse_long(&options, longopts,
                                 NULL)) != -1) {
    switch (option) {
    case 'h':
      printf(help_message, argv[0]);
      macefree(m);
      return EXIT_SUCCESS;

    case 'v':
      printf("Mace %s\n", VERSION_STR);
      macefree(m);
      return EXIT_SUCCESS;

    case 'c':
      snprintf(configpath, sizeof(configpath), "%s",
               options.optarg);
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

/* TODO
    paneaddtab(m->pane, t, -1);
    */
  }

  if (findconfig(&f, configpath, sizeof(configpath))) {
    conf = toml_parse_file(f, errbuf, sizeof(errbuf));
    fclose(f);

    if (conf == NULL) {
      fprintf(stderr, "Error parsing '%s': %s\n", configpath,
              errbuf);
      macefree(m);
      return EXIT_FAILURE;
    }

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

  r = dodisplay(m);
  macefree(m);
  return r;
}
