
#define NAMEMAX 32
#define PADDING 5

typedef enum { FOCUS_action, FOCUS_main, NFOCUS_T } focus_t;

struct colour {
  unsigned char r, g, b, a;
};

struct piece {
  struct piece **prev, *next;

  size_t n;
  char *s;
};

struct focus {
  struct piece *piece;
  unsigned int pos;
};

struct tab {
  char name[NAMEMAX];
  struct tab *next;

  /* Pre-rendered rgba buffer of header.
   * Has size (tabwidth * lineheight * 4) */
  unsigned char *buf;

  struct piece *action;
  struct focus actionfocus;

  int voff;
  struct piece *main;
  struct focus mainfocus;
};

/* PANE_norm has its norm structure populated to contain a list of
   tabs, a focus and a list offset (for scrolling).
   PANE_vsplit and PANE_hsplit have their split structure populated to
   work as a binary search tree representing the way the window has
   been split. Each leaf node will always be of type PANE_norm.
*/

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
fontinit(void);

int
loadglyph(char *s);

void
resize(unsigned char *nbuf, int w, int h);



void
drawline(unsigned char *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

void
drawrect(unsigned char *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

void
drawglyph(unsigned char *dest, int dw, int dh,
	  int dx, int dy,
	  int sx, int sy,
	  int w, int h,
	  struct colour *c);

int
drawstring(unsigned char *dest, int dw, int dh,
	   int dx, int dy,
	   int tx, int ty,
	   int tw, int th,
	   char *s, bool drawpart,
	   struct colour *c);

void
drawprerender(unsigned char *dest, int dw, int dh,
	      int dx, int dy,
	      unsigned char *src, int sw, int sh,
	      int sx, int sy,
	      int w, int h);


bool
handlekeypress(unsigned int code);

bool
handlekeyrelease(unsigned int code);

bool
handlebuttonpress(int x, int y, unsigned int button);

bool
handlebuttonrelease(int x, int y, unsigned int button);

bool
handlemotion(int x, int y);



struct pane *
panenew(struct pane *parent, struct tab *tabs);

void
panefree(struct pane *p);

void
resizepane(struct pane *p, int x, int y, int w, int h);

struct pane *
findpane(struct pane *p,
	 int x, int y);

void
panedraw(struct pane *p);

int
panedrawtablist(struct pane *p);

struct pane *
panesplit(struct pane *h, struct tab *t,
	  pane_t type, bool na);

void
paneremovetab(struct pane *p, struct tab *t);

void
paneremove(struct pane *p);

void
panetablistscroll(struct pane *p, int s);

bool
handlepanepress(struct pane *p, int x, int y,
		unsigned int button);

bool
handlepanerelease(struct pane *p, int x, int y,
		  unsigned int button);

bool
handlepanemotion(struct pane *p, int x, int y);



struct tab *
tabnew(char *name);

void
tabfree(struct tab *t);

void
tabprerender(struct tab *t);

/* Claims s */
struct piece *
piecenew(char *s, size_t n);

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

  
extern unsigned int width, height;
extern unsigned char *buf;

extern struct colour bg, fg, abg;

extern FT_Face face;
extern int fontsize;

extern int lineheight;
extern int baseline;

extern int tabwidth;

extern struct pane *root, *focus;
extern focus_t focustype;

