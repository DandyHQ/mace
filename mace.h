
#define NAMEMAX 32
#define PIECE_min 16

#define max(a, b) (a > b ? a : b)

/* Most keys are just text and a given to functions as a utf8 encoded
   string. But some are not. And these are those special keys.
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

typedef enum { FOCUS_action, FOCUS_main, NFOCUS_T } focus_t;

struct colour {
  uint8_t r, g, b, a;
};

struct piece {
  struct piece *prev, *next;

  size_t rl; /* Real length of s. */
  size_t pl; /* Currently populated length of s. */
  uint8_t *s;
};

struct selection {
  unsigned int start, end;
  bool increasing;
  struct selection *next;
};

#define TEXTBOX_PADDING 5

struct textbox {
  unsigned int cursor;
  struct selection *selections, *cselection;

  struct piece *pieces;

  struct colour bg;

  bool noscroll;
  int yscroll;

  int width, height, textheight;
};

struct tab {
  uint8_t name[NAMEMAX];
  struct tab *next;

  struct textbox action, main;
};

typedef enum { PANE_norm, PANE_vsplit, PANE_hsplit } pane_t;

struct pane {
  pane_t type;
  struct pane *parent;

  int x, y;
  int width, height;
  
  union {
    struct {
      float ratio;
      struct pane *a, *b;
    } split;

    struct {
      struct tab *tabs, *focus;
      int loff;
    } norm;
  };
};

void
init(void);

void
luainit(void);

void
fontinit(void);

int
fontload(const uint8_t *name, size_t size);

bool
loadglyph(int32_t code);

/* Check if code is a line break. *l is the number of bytes in the 
   current code, it may be increased depending on what code is.
*/
bool
linebreak(int32_t code, uint8_t *s, int32_t max, int32_t *l);

/* User Input */

bool
handletyping(uint8_t *s, size_t n);

bool
handlekeypress(keycode_t k);

bool
handlekeyrelease(keycode_t k);

bool
handlebuttonpress(int x, int y, int b);

bool
handlebuttonrelease(int x, int y, int b);

bool
handlemotion(int x, int y);

bool
handlescroll(int x, int y, int dx, int dy);

bool
handlepanepress(struct pane *p, int x, int y,
		unsigned int button);

bool
handlepanerelease(struct pane *p, int x, int y,
		  unsigned int button);

bool
handlepanemotion(struct pane *p, int x, int y);

bool
handlepanetablistscroll(struct pane *p, int x, int y, int dx, int dy);

bool
handlepanescroll(struct pane *p, int x, int y, int dx, int dy);


/* Drawing */

/* If bounds are invalid then they should be fixed by the function. */

/* Only works with vertical or horizontal lines.
   Draw a line in a buffer dest of size bw * bh,
   from x1,y1 to (including) x2,y2 with colour c.
   Will blend c with the current values in dest.
*/ 
void
drawline(uint8_t *dest, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

/* Draw a rectangle in a buffer dest of size bw * bh,
   from x1,y1 to (including) x2,y2 with colour c.
   Will blend c with the current values in dest.
*/ 
void
drawrect(uint8_t *dest, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

/* Draw the currently loaded glyph to a buffer dest of size bw * bh,
   at location dx, dy, with offset in the glyph of sx,sy drawing to
   sx + w, sy + h. Blends the colour c scaled by the grayscale value 
   of the grlyph with the background.
*/ 

void
drawglyph(uint8_t *dest, int dw, int dh,
	  int dx, int dy,
	  int sx, int sy,
	  int w, int h,
	  struct colour *c);

/* Draws a string of utf8 glyphs. Puts them at dx,dy and starts at
   offset sx,sy in the string. Goes until sx+sw is reached and will
   only draw a maximum height of sh. If the next glyph will pass sx+sw
   and drawpart is true then part of a glyph is drawn and drawing 
   stops, if it is false drawing stops before the last glyph.
   Returns the number of bytes drawn before reaching the end or 
   reaching sx+sw.
*/
int
drawstring(uint8_t *dest, int dw, int dh,
	   int dx, int dy,
	   int sx, int sy,
	   int sw, int sh,
	   uint8_t *s, bool drawpart,
	   struct colour *c);

/* Draws a pane to the global image buffer. */
void
panedraw(struct pane *p);

/* Draws a component of a pane to the global image buffer. */

int
panedrawtablist(struct pane *p);

void
panedrawaction(struct pane *p);

void
panedrawmain(struct pane *p);

struct pane *
panenew(struct pane *parent, struct tab *tabs);

/* Does not free tabs in p. p should have no tabs left when this is
   called.
*/
void
panefree(struct pane *p);

/* Resized the pane p to be located at x,y and have dimentions w x h.
   Calls paneresize recursivly on all child panes to resize them 
   to the appropriate preportion of p.
*/
void
paneresize(struct pane *p, int x, int y, int w, int h);

/* Finds the child of p (or p) that contains the point x,y */
struct pane *
findpane(struct pane *p,
	 int x, int y);

/* Splits the pane h creating a new pane holding t and an copy
   of h holding all of h's tabs. h is then set up to 
   hold the new pane and the copyied pane. 
   type determines the type of split (horizontal or vertical)
   and na determines if the new tab will be h's a child or h's
   b child.
*/
struct pane *
panesplit(struct pane *h, struct tab *t,
	  pane_t type, bool na);

void
paneremovetab(struct pane *p, struct tab *t);

/* Removes the pane p by changing the type of p's parent and
   gives p's parents p's siblings children. 
   Free's both p and p's sibling.
*/
void
paneremove(struct pane *p);

struct tab *
tabnew(uint8_t *name);

void
tabfree(struct tab *t);

/* Creates a new piece. Claims the string s as its own,
   s should have a allocated length of rl and a used lenght of pl.
*/
struct piece *
piecenewgive(uint8_t *s, size_t rl, size_t pl);

/* Creates a new piece copying the string s of length l into it's own 
   buffer.
*/
struct piece *
piecenewcopy(uint8_t *s, size_t l);

/* Creates a new tag piece. Tag pieces have no buffers. */
struct piece *
piecenewtag(void);

/* Frees the piece and its buffer. */
void
piecefree(struct piece *p);

/* Finds pos in list of pieces. Sets *i to index in piece returned. */
struct piece *
findpiece(struct piece *p, int pos, int *i);

/* Splits the piece p and position pos and sets lr and rr to point
   to the newly created left and right pieces.
*/
bool
piecesplit(struct piece *p, size_t pos,
	   struct piece **lr, struct piece **rr);

/* Splits old at position pos and inserts a new piece created by
   copying the string s of lenght l.
*/
struct piece *
pieceinsert(struct piece *old, size_t pos,
	    uint8_t *s, size_t l);


bool
textboxinit(struct textbox *t, struct colour *bg, bool noscroll);

void
textboxfree(struct textbox *t);

void
textboxdraw(struct textbox *t, uint8_t *dest, int dw, int dh,
	    int x, int y, int w, int h, bool focus);

bool
textboxscroll(struct textbox *t, int dx, int dy);

bool
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button);

bool
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button);

bool
textboxmotion(struct textbox *t, int x, int y);

bool
textboxscroll(struct textbox *t, int dx, int dy);

bool
textboxtyping(struct textbox *t, uint8_t *s, size_t l);

bool
textboxkeypress(struct textbox *t, keycode_t k);

bool
textboxkeyrelease(struct textbox *t, keycode_t k);

struct selection *
selectionnew(unsigned int start, unsigned int end);

void
selectionfree(struct selection *s);

void
selectionupdate(struct selection *s, unsigned int end);

/* Variables */

extern unsigned int width, height;
extern uint8_t *buf;

extern struct colour bg, fg, abg;

extern FT_Face face;

extern int lineheight;
extern int baseline;

extern int tabwidth;

extern struct pane *root, *focus;
extern focus_t focustype;
