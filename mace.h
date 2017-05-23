#include <unistd.h>
#include <stdbool.h>
#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

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

typedef enum { SELECTION_left, SELECTION_right } selection_t;

/* Note: There is a memory leak if lua code changes the linked list
   structure to skip a selection. Should probably turn them into
   tables or have some more structured way of talking to them. This
   applies for all linked lists that lua can see (selections and
   tabs). */

struct selection {
  struct textbox *textbox;

  int32_t start, end;
  selection_t direction;

  struct selection *next;
};


/* This is done horribly. There can only be one instance for every
   instance of mace. I would like to be able to have each textbox have
   its own instance, if not of library then at least of face. */

struct font {
  FT_Library library;
  FT_Face face;

  int baseline, lineheight;
};

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

  int32_t cursor;
  int yoff;

  int linewidth, height;
  int maxheight;
};

struct tab {
  struct mace *mace;
  
  uint8_t *name;

  int x, y, width, height;
  
  struct textbox *action, *main;
  struct tab *next;
};

struct mace {
  bool running;

  struct font *font;
  lua_State *lua;
  
  struct tab *tabs;
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
luaruninit(lua_State *L);

void
command(lua_State *L, const uint8_t *s);



struct font *
fontnew(void);

void
fontfree(struct font *font);

int
fontset(struct font *font, const uint8_t *pattern);

bool
loadglyph(FT_Face face, int32_t code);

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
tabresize(struct tab *t, int x, int y, int w, int h);

void
tabdraw(struct tab *t, cairo_t *cr);

bool
tabscroll(struct tab *t, int x, int y, int dy);

bool
tabbuttonpress(struct tab *t, int x, int y,
	       unsigned int button);

bool
tabbuttonrelease(struct tab *t, int x, int y,
		 unsigned int button);

bool
tabmotion(struct tab *t, int x, int y);



/* Takes ownership of seq */

struct textbox *
textboxnew(struct tab *tab, struct colour *bg,
	   struct sequence *seq);

void
textboxfree(struct textbox *t);

bool
textboxresize(struct textbox *t, int width, int maxheight);

bool
textboxscroll(struct textbox *t, int x, int y, int dy);

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
handlescroll(struct mace *mace, int x, int y, int dy);
