
#define NAMEMAX 32

struct colour {
  float r, g, b, a;
};

struct tab {
  uint8_t name[NAMEMAX];
  struct tab *next;
  int voff;

  /* Pre-rendered rgba buffer of header.
   * Has size (tabwidth * listheight * 4) */
  char *buf;
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

void
resize(char *nbuf, int w, int h);

void
drawline(char *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

void
drawrect(char *buf, int bw, int bh,
	 int x1, int y1, int x2, int y2,
	 struct colour *c);

bool
loadchar(long c);

void
drawglyph(char *dest, int dw, int dh,
	  int dx, int dy,
	  int sx, int sy,
	  int w, int h,
	  struct colour *c);

void
drawprerender(char *dest, int dw, int dh,
	      int dx, int dy,
	      char *src, int sw, int sh,
	      int sx, int sy,
	      int w, int h);

void
drawpane(struct pane *p);

void
drawtablist(struct pane *p);

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

struct pane *
panesplit(struct pane *h, struct tab *t,
	  pane_t type, bool na);

void
paneremovetab(struct pane *p, struct tab *t);

void
paneremove(struct pane *p);

void
panetablistscroll(struct pane *p, int s);

struct tab *
tabnew(uint8_t *name);

void
tabfree(struct tab *t);

void
tabprerender(struct tab *t);

extern unsigned int width, height;
extern unsigned char *buf;

extern FT_Face face;
extern struct colour bg;
extern struct colour fg;

extern int fontsize;
extern int listheight;
extern int tabwidth;
extern int scrollwidth;

extern struct pane *root;
