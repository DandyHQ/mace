#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>
#include <cairo-ft.h>

#include "utf8.h"
#include "sequence.h"

/* Just a helper structure, not really used in many places. */
struct colour {
  double r, g, b;
};

#define CURSEL_left   (0<<0)
#define CURSEL_right  (1<<0)
#define CURSEL_nrm    (1<<1)
#define CURSEL_cmd    (1<<2)

/* Should be in a ordered list in mace. Order by start and group by tb. */

struct cursel {
  int type; /* Some OR of the above options. */

  struct textbox *tb;
  size_t start,
         end;     /* Start and end of selection in tb. */
  size_t cur;              /* Offset of cursor from start. */

  struct cursel *next;
};

/* Padding used for textbox's. */
#define PAD 5

struct textbox {
  struct mace *mace;
  struct tab *tab;
  struct column *column;

  struct colour bg;

  struct sequence *sequence;

	int x, y;
	
  /* Maximum width a line can be. */
  int linewidth;

  /* Maximum height that will be displayed. */
  int maxheight;

  /* Index in sequence that drawing starts from. */
  size_t start;
  /* Number of bytes that are part of glyphs. ie: drawable. */
  size_t drawablelen;

  /* Height needed to display glyphs. Should be <= to maxheight. */
  int height;

  /* This is not done quite how I want, currently it is the placing
     all the glyphs in the sequence. */
  /* Number of glyphs to display. */
  size_t nglyphs;

  /* Number of glyphs glphys can currently fit. */
  size_t nglyphsmax;

  cairo_glyph_t *glyphs;

  /* Last added cursel in this textbox.
     Used for making selections. */
  struct cursel *curcs;
};


#define SCROLL_WIDTH   13

struct tab {
  struct mace *mace;

  struct column *column;

  int width, height;

  struct textbox *action, *main;

  struct tab *next;
};

struct column {
  struct mace *mace;

  int width, height;
  
  struct textbox *textbox;

  struct tab *tabs;
  
  struct column *next;
};

struct keybinding {
  uint8_t *key, *cmd;
  struct keybinding *next;
};

/* A wrapper around freetype with functions that use fontconfig to
   load fonts from patterns. */

struct font {
  char path[1024];
  double size;

  FT_Library library;

  FT_Face face;
  cairo_font_face_t *cface;

  int baseline, lineheight;
  size_t tabwidth;
  int tabwidthpixels;
};

/* A mace structure. Each structure represents a window and lua
   runtime. */

typedef enum {
  SCROLL_down,
  SCROLL_up,
  SCROLL_immediate,
  SCROLL_none
} scroll_action_t;

struct mace {
  bool running;

	int width, height;
	
  struct font *font;

  struct keybinding *keybindings;

  struct textbox *textbox;
  struct column *columns;
  
  int offx, offy, px, py;
  struct tab *movingtab;
  struct column *movingcolumn;
  
  struct cursel *cursels;

  uint8_t *defaultaction;
  uint8_t *defaultcol;
  uint8_t *defaultmain;

  struct textbox *mousefocus;
  bool immediatescrolling;

  scroll_action_t scrollleft, scrollmiddle, scrollright;
};


int
displayinit(struct mace *m);

int
displayloop(struct mace *m);

/* Claims data. */
void
setclipboard(uint8_t *data, size_t len);

/* Do not free. */
uint8_t *
getclipboard(size_t *len);


struct mace *
macenew(void);

bool
maceinit(struct mace *m);

void
macefree(struct mace *mace);

/* Sets a flag that informs whatever is drawing this can stop and free
   it. */
void
macequit(struct mace *mace);

bool
maceaddkeybinding(struct mace *m, uint8_t *key,
                  uint8_t *cmd);

bool
maceresize(struct mace *m, int w, int h);

void
macedraw(struct mace *m, cairo_t *cr);

bool
command(struct mace *mace, const uint8_t *cmd);



struct font *
fontnew(void);

void
fontfree(struct font *font);

bool
fontset(struct font *font, const char *pattern);

bool
fontsettabwidth(struct font *font, size_t spaces);

bool
loadglyph(FT_Face face, int32_t code);

void
drawglyph(struct font *f, cairo_t *cr, int x, int y,
          struct colour *fg, struct colour *bg);




struct cursel *
curseladd(struct mace *mace, struct textbox *t, int type,
          size_t pos);

void
curselremove(struct mace *mace, struct cursel *c);

void
curselremoveall(struct mace *mace);

bool
curselupdate(struct cursel *c, size_t pos);

struct cursel *
curselat(struct mace *mace, struct textbox *t,  size_t pos);

void
shiftcursels(struct mace *mace, struct textbox *t,
             size_t from, int dist);



struct column *
columnnew(struct mace *mace);

void
columnfree(struct column *c);

bool
columnaddtab(struct column *c, struct tab *t);

void
columnremovetab(struct column *c, struct tab *t);

bool
columnresize(struct column *c, int w, int h);

void
columndraw(struct column *c, cairo_t *cr, int x, int y);


/* New tabs are not added to any pane on creation, You must do that
   yourself with paneaddtab. paneaddtab will resize the tab to fit in
   the pane. Tabs can not be in multiple panes at once. */

struct tab *
tabnew(struct mace *mace,
       const uint8_t *filename,
       struct sequence *mainseq);

struct tab *
tabnewempty(struct mace *mace,
            const uint8_t *name);

struct tab *
tabnewfrompath(struct mace *mace,
               const uint8_t *path);

void
tabfree(struct tab *t);

bool
tabsetname(struct tab *t, const uint8_t *name, size_t len);

bool
tabresize(struct tab *t, int w, int h);

void
tabdraw(struct tab *t, cairo_t *cr, 
        int x, int y, int w, int h);

bool
tabbuttonpress(struct tab *t, int x, int y,
               unsigned int button);



struct textbox *
textboxnew(struct mace *mace,
           struct colour *bg,
           struct sequence *seq);

void
textboxfree(struct textbox *t);

bool
textboxresize(struct textbox *t, int width, int maxheight);

bool
textboxbuttonpress(struct textbox *t, int x, int y,
                   unsigned int button);

bool
textboxbuttonrelease(struct textbox *t, int x, int y,
                     unsigned int button);

bool
textboxmotion(struct textbox *t, int x, int y);

bool
textboxscroll(struct textbox *t, int lines);


/* Returns the index of the nearest glyph above the glyph at
   position i. */
size_t
textboxindexabove(struct textbox *t, size_t i);

/* Returns the index of the nearest glyph below the glyph at
   position i. */
size_t
textboxindexbelow(struct textbox *t, size_t i);

/* Fulls out buf with up to len bytes of 'indentation'
   characters in the line that contains position i. */
size_t
textboxindentation(struct textbox *t, size_t i,
                   uint8_t *buf, size_t len);



size_t
textboxfindpos(struct textbox *t, int x, int y);



void
textboxplaceglyphs(struct textbox *t);

/* Width should be t->linewidth otherwise it will not be drawn properly. */
void
textboxdraw(struct textbox *t, cairo_t *cr,
            int x, int y,
            int width, int height);



/* Handle UI events. Return true if mace needs to be redrawn */

bool
handlekey(struct mace *mace, uint8_t *s, size_t n,
          bool special);

bool
handlebuttonpress(struct mace *mace, int x, int y, int b);

bool
handlebuttonrelease(struct mace *mace, int x, int y, int b);

bool
handlemotion(struct mace *mace, int x, int y);

bool
handlescroll(struct mace *mace, int x, int y, int dx,
             int dy);

extern struct colour bg, abg;
