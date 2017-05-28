#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifndef PATH_MAX
/* Linux has PATH_MAX defined under <linux/limits.h>, OpenBSD has it
   defined under <limits.h>, PATH_MAX is a horrible thing anyway so
   lets just make it up. */
#define PATH_MAX 512
#endif

#include "lib.h"
#include "sequence.h"

/* Most keys are just text and are given to functions as a utf8 
   encoded string. But some are not. And these are those special keys.
*/

typedef enum {
  KEY_none,

  KEY_shift,
  KEY_alt,
  KEY_super,
  KEY_control,

  KEY_left,
  KEY_right,
  KEY_up,
  KEY_down,
  KEY_pageup,
  KEY_pagedown,
  KEY_home,
  KEY_end,

  KEY_return,
  KEY_tab,
  KEY_backspace,
  KEY_delete,

  KEY_escape
} keycode_t;

struct colour {
  double r, g, b;
};

/* Used by textbox's to store a linked list of current
   selections. Currently you can only have one selection so it doesn't
   need to be a list, but if future I think it would be cool to have a
   multiple selections. */

typedef enum { SELECTION_left, SELECTION_right } selection_t;

struct selection {
  struct textbox *textbox;

  int32_t start, end;
  selection_t direction;

  struct selection *next;
};

/* A wrapper around freetype with functions that use fontconfig to
   load fonts from patterns. Each textbox has it's own font
   structure. This is probably inefficent but I will keep it for now. */

struct font {
  char path[PATH_MAX];
  double size;
  
  FT_Library library;
  FT_Face face;

  int baseline, lineheight;
};

#define TABMAX 32

struct textbox {
  struct tab *tab;
  
  struct font *font;

  struct selection *csel, *selections;
  bool cselisvalid;
  
  struct colour bg;

  cairo_t *cr;
  cairo_surface_t *sfc;

  struct sequence *sequence;

  ssize_t startpiece;
  size_t startindex;
  int startx, starty;

  uint8_t tabstring[TABMAX];
 
  int32_t cursor;
  int yoff;

  int linewidth, height;
  int maxheight;
};

struct tab {
  struct mace *mace;
  
  uint8_t *name;
  size_t nlen;

  int x, y, width, height;
  
  struct textbox *action, *main;

  struct pane *pane;
  struct tab *next;
};

/* There is currently only one pane but in future you will be able to
   split them and have multiple set up however you like. Each pane has
   a linked list of tabs. */

struct pane {
  struct mace *mace;

  int x, y, width, height;

  struct tab *tabs;
  struct tab *focus;
};

/* A mace structure. Each structure represents a window and lua
   runtime. */

struct mace {
  bool running;

  struct font *font;
  lua_State *lua;
  
  struct pane *pane;
  struct textbox *focus;
};

struct mace *
macenew(void);

void
macefree(struct mace *mace);

void
macequit(struct mace *mace);


lua_State *
luanew(struct mace *mace);

void
luafree(lua_State *L);

void
lualoadrc(lua_State *L);

/* Structs that lua uses must call this before freeing themselves. */
/* Currently pane, tabs, and textboxs. */
void
luaremove(lua_State *L, void *addr);


void
command(lua_State *L, const uint8_t *s);



struct font *
fontnew(void);

struct font *
fontcopy(struct font *old);

void
fontfree(struct font *font);

int
fontset(struct font *font, const char *pattern);

bool
loadglyph(FT_Face face, int32_t code);

void 
drawglyph(struct font *f, cairo_t *cr, int x, int y,
	  struct colour *fg, struct colour *bg);



struct pane *
panenew(struct mace *mace);

void
panefree(struct pane *p);

bool
paneresize(struct pane *p, int x, int y, int w, int h);

void
panedraw(struct pane *p, cairo_t *cr);

bool
tablistbuttonpress(struct pane *p, int x, int y, int button);

bool
tablistscroll(struct pane *p, int x, int y, int dx, int dy);

/* Pos = -1  puts the tab at the end. Otherwise it puts it in the list
   so that it is the pos'th tab */
void
paneaddtab(struct pane *p, struct tab *t, int pos);

void
paneremovetab(struct pane *p, struct tab *t);


/* New tabs are not added to any pane on creation, You must do that
   yourself with paneaddtab. paneaddtab will resize the tab to fit in
   the pane. Tabs can not be in multiple panes at once. */

struct tab *
tabnew(struct mace *mace,
       const uint8_t *name, size_t nlen,
       struct sequence *mainseq);

struct tab *
tabnewempty(struct mace *mace,
	    const uint8_t *name, size_t nlen);

struct tab *
tabnewfromfile(struct mace *mace,
	       const uint8_t *name, size_t nlen,
	       const uint8_t *filename, size_t flen);

void
tabfree(struct tab *t);

bool
tabsetname(struct tab *t, const uint8_t *name, size_t len);

bool
tabresize(struct tab *t, int x, int y, int w, int h);

void
tabdraw(struct tab *t, cairo_t *cr);

bool
tabscroll(struct tab *t, int x, int y, int dx, int dy);

bool
tabbuttonpress(struct tab *t, int x, int y,
	       unsigned int button);

bool
tabbuttonrelease(struct tab *t, int x, int y,
		 unsigned int button);

bool
tabmotion(struct tab *t, int x, int y);



struct textbox *
textboxnew(struct tab *tab, struct colour *bg,
	   struct sequence *seq);

void
textboxfree(struct textbox *t);

bool
textboxresize(struct textbox *t, int width, int maxheight);

bool
textboxscroll(struct textbox *t, int xoff, int yoff);

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button);

bool
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button);

bool
textboxmotion(struct textbox *t, int x, int y);

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l);

bool
textboxkeypress(struct textbox *t, keycode_t k);

bool
textboxkeyrelease(struct textbox *t, keycode_t k);

size_t
textboxfindpos(struct textbox *t, int x, int y);

void
textboxcalcpositions(struct textbox *t, size_t pos);

void
textboxfindstart(struct textbox *t);

void
textboxpredraw(struct textbox *t);




struct selection *
selectionnew(struct textbox *t, int32_t pos);

void
selectionfree(struct selection *s);

/* Updates the selection to include pos, returns true if something
   changed, false otherwise. */

bool
selectionupdate(struct selection *s, int32_t pos);

/* Checks if the position pos in the textbox t is in a selection,
   returns the selection if it is. */

struct selection *
inselections(struct textbox *t, int32_t pos);



/* Handle UI events. Return true if mace needs to be redrawn */

bool
handletyping(struct mace *mace, uint8_t *s, size_t n);

bool
handlekeypress(struct mace *mace, keycode_t k);

bool
handlekeyrelease(struct mace *mace, keycode_t k);

bool
handlebuttonpress(struct mace *mace, int x, int y, int b);

bool
handlebuttonrelease(struct mace *mace, int x, int y, int b);

bool
handlemotion(struct mace *mace, int x, int y);

bool
handlescroll(struct mace *mace, int x, int y, int dx, int dy);
