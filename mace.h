#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>
#include <cairo-ft.h>

#if defined(__linux)

#define PATH_MAX 1024

static size_t
strlcpy(char *dst, const char *src, size_t dstsize)
{
	size_t i;

	for (i = 0; i + 1 < dstsize && src[i] != 0; i++) {
		dst[i] = src[i];
	}

	dst[i] = 0;
	return i;
}

#elif defined(__OpenBSD__)

#include <limits.h>

#endif

#include "utf8.h"

/* Just a helper structure, not really used in many places. */
struct colour {
  double r, g, b;
};

/* A wrapper around freetype with functions that use fontconfig to
   load fonts from patterns. */

struct font {
  char path[PATH_MAX];
  double size;
  
  FT_Library library;

  FT_Face face;
  cairo_font_face_t *cface;

  int baseline, lineheight;
  size_t tabwidth;
  int tabwidthpixels;
};

/* Cursors and selections could possibly be merged. Every selection has
    a cursor except commands. So selections may end up with a cursor 
    position field at some point. */

struct cursor {
	struct textbox *tb;
	size_t pos;

	struct cursor *next;
};

typedef enum { SELECTION_left, SELECTION_right } selection_direction_t;
typedef enum { SELECTION_normal, SELECTION_command } selection_t;

struct selection {
	struct textbox *tb;
	selection_t type;

	size_t start, end;
	selection_direction_t direction;
	
	struct selection *next;
};

#define SEQ_start  0
#define SEQ_end   1
#define SEQ_first   2

struct piece {
	ssize_t prev, next;
	size_t len;
	size_t pos; /* In sequence */
	size_t off; /* In data buffer */

	cairo_glyph_t *glyphs;
	size_t nglyphs;
};

struct sequence {
  struct piece *pieces;
  size_t plen, pmax;

  uint8_t *data;
  size_t dlen, dmax;

	int linewidth;
	struct font *font;
};

/* Padding used for textbox's. */
#define PAD 5

struct textbox {
	struct mace *mace;
	struct tab *tab;

  /* Maximum width a line can be. */
  int linewidth;

	/* How far from the top have we scrolled */
  int yoff;

  struct colour bg;

  struct sequence *sequence;

	/* For adding a selection. */
  struct selection *csel;
	struct cursor *ccur;
	size_t newselpos;
};

struct tab {
  /* Mace structure that created this. */
  struct mace *mace;

  /* Parent pane. */
  struct pane *pane;
  
  uint8_t *name;
  size_t nlen;

  int x, y, width, height;
  
  struct textbox *action, *main;

  /* Next in linked list. */
  struct tab *next;
};

/* There is currently only one pane but in future you will be able to
   split them and have multiple set up however you like. Each pane has
   a linked list of tabs. */

struct pane {
  struct mace *mace;

  int x, y, width, height;

  /* Linked list of tabs in this pane. */
  struct tab *tabs;

  /* Currently focused tab. */
  struct tab *focus;
};

struct keybinding {
	uint8_t *key, *cmd;
	struct keybinding *next;
};

/* A mace structure. Each structure represents a window and lua
   runtime. */

struct mace {
  bool running;

  struct font *font;

	struct keybinding *keybindings;
  
  struct pane *pane;

  struct textbox *mousefocus;

	struct selection *selections;
	struct cursor *cursors;
};



struct mace *
macenew(void);

void
macefree(struct mace *mace);

/* Sets a flag that informs whatever is drawing this can stop and free
   it. */
void
macequit(struct mace *mace);

bool
maceaddkeybinding(struct mace *m, uint8_t *key, uint8_t *cmd);

bool
command(struct mace *mace, const uint8_t *cmd);



struct font *
fontnew(void);

void
fontfree(struct font *font);

int
fontset(struct font *font, const char *pattern);

int
fontsettabwidth(struct font *font, size_t spaces);

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
       const uint8_t *filename, size_t flen,
       struct sequence *mainseq);

struct tab *
tabnewempty(struct mace *mace,
            const uint8_t *name, size_t nlen);

struct tab *
tabnewfromfile(struct mace *mace,
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



struct textbox *
textboxnew(struct mace *mace, struct tab *t,
                   struct colour *bg,
                   struct sequence *seq);

void
textboxfree(struct textbox *t);

bool
textboxresize(struct textbox *t, int width);

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

size_t
textboxfindpos(struct textbox *t, int x, int y);

void
textboxdraw(struct textbox *t, cairo_t *cr, int x, int y, 
                    int width, int height);



struct cursor *
cursoradd(struct mace *mace, struct textbox *t,
                size_t pos);

void
cursorremove(struct mace *mace, struct cursor *c);

void
cursorremoveall(struct mace *mace);

struct cursor *
cursorat(struct mace *mace, struct textbox *t,
              size_t pos);

void
shiftcursors(struct mace *mace, struct textbox *t,
                   size_t from, int dist);



struct selection *
selectionadd(struct mace *mace, struct textbox *t, 
                    selection_t type, size_t pos1, size_t pos2);

void
selectionremove(struct mace *mace, struct selection *s);

void
selectionremoveall(struct mace *mace);

bool
selectionupdate(struct selection *s, size_t pos);

struct selection *
inselections(struct mace *mace, struct textbox *t, size_t pos);

void
shiftselections(struct mace *mace, struct textbox *t,
                      size_t from, int dist);



/* Creates a new sequence around data, data may be NULL.
   len is the number of set bytes in data, max is the current size of the
   allocation for data. 

   font is controlled by the caller and should not be free'd until the
   new sequence is freed.
*/

struct sequence *
sequencenew(struct font *font, uint8_t *data, size_t len);

/* Frees a sequence and all it's pieces. */

void
sequencefree(struct sequence *s);

void
sequencesetlinewidth(struct sequence *s, int l);

/* Inserts data into the sequence at position pos. */

bool
sequenceinsert(struct sequence *s, size_t pos,
	       const uint8_t *data, size_t len);

/* Deletes from the sequence at pos a string of length len */

bool
sequencedelete(struct sequence *s, size_t pos, size_t len);

/* Finds the start and end of a word around pos. */

bool
sequencefindword(struct sequence *s, size_t pos,
                             size_t *start, size_t *len);

size_t
sequencecodepointlen(struct sequence *s, size_t pos);

size_t
sequenceprevcodepointlen(struct sequence *s, size_t pos);

/* Fulls out buf with the contentes of the sequence starting at pos
   and going for at most len bytes, buf must be of at least len + 1
   bytes and will be null terminated. */

size_t
sequenceget(struct sequence *s, size_t pos,
            uint8_t *buf, size_t len);

size_t
sequencelen(struct sequence *s);

int
sequenceheight(struct sequence *s);


/* Handle UI events. Return true if mace needs to be redrawn */

bool
handlekey(struct mace *mace, uint8_t *s, size_t n);

bool
handlebuttonpress(struct mace *mace, int x, int y, int b);

bool
handlebuttonrelease(struct mace *mace, int x, int y, int b);

bool
handlemotion(struct mace *mace, int x, int y);

bool
handlescroll(struct mace *mace, int x, int y, int dx, int dy);
