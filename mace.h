
#define NAMEMAX 32
#define PADDING 5
#define PIECE_min 16

#define max(a, b) (a > b ? a : b)

/* Most keys are just text and a given to functions as a utf8 encoded
 * string. But some are not. And these are those special keys.
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

  KEY_escape,
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

struct tab {
  uint8_t name[NAMEMAX];
  struct tab *next;

  unsigned int acursor;
  struct piece *action;
  int actionbarheight;
  
  int voff;
  unsigned int mcursor;
  struct piece *main;
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

/* Fonts */

void
fontinit(void);

int
fontload(const uint8_t *name, size_t size);

bool
loadglyph(int32_t code);

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


/* Drawing */

void
drawline(uint8_t *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

void
drawrect(uint8_t *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

void
drawglyph(uint8_t *dest, int dw, int dh,
	  int dx, int dy,
	  int sx, int sy,
	  int w, int h,
	  struct colour *c);

int
drawstring(uint8_t *dest, int dw, int dh,
	   int dx, int dy,
	   int tx, int ty,
	   int tw, int th,
	   uint8_t *s, bool drawpart,
	   struct colour *c);

void
drawprerender(uint8_t *dest, int dw, int dh,
	      int dx, int dy,
	      uint8_t *src, int sw, int sh,
	      int sx, int sy,
	      int w, int h);

void
panedraw(struct pane *p);

int
panedrawtablist(struct pane *p);

void
panedrawaction(struct pane *p);

void
panedrawmain(struct pane *p);

/* Pane management */

struct pane *
panenew(struct pane *parent, struct tab *tabs);

void
panefree(struct pane *p);

void
paneresize(struct pane *p, int x, int y, int w, int h);

struct pane *
findpane(struct pane *p,
	 int x, int y);

struct pane *
panesplit(struct pane *h, struct tab *t,
	  pane_t type, bool na);

void
paneremovetab(struct pane *p, struct tab *t);

void
paneremove(struct pane *p);

void
panetablistscroll(struct pane *p, int dx, int dy);

void
panescroll(struct pane *p, int dx, int dy);



/* Tab Management */

struct tab *
tabnew(uint8_t *name);

void
tabfree(struct tab *t);

/* Piece management */

struct piece *
piecenewgive(uint8_t *s, size_t rl, size_t pl);

struct piece *
piecenewcopy(uint8_t *s, size_t l);

struct piece *
piecenewtag(void);

void
piecefree(struct piece *p);

/* Either returns the line that was clicked on and sets *pos to 
 * be the index in the line's string. Or returns NULL and sets *pos
 * to the distance from y checked
 */
struct piece *
findpos(struct piece *pieces,
	int x, int y, int linewidth,
	int *pos);

/* Finds pos in list of pieces.
 * Sets *i to index in piece returned. */
struct piece *
findpiece(struct piece *p, int pos, int *i);

bool
piecesplit(struct piece *p, size_t pos,
	   struct piece **lr, struct piece **rr);

struct piece *
pieceinsert(struct piece *old, size_t pos,
	    uint8_t *s, size_t l);


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
