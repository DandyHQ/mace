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

struct piece {
  ssize_t prev, next;
  size_t len;
  size_t pos; /* In sequence */
  size_t off; /* In data buffer */
};

#define SEQ_start  0
#define SEQ_end    1


struct sequence {
  struct piece *pieces;
  size_t plen, pmax;
  
  uint8_t *data;
  size_t dlen, dmax;
};

struct colour {
  double r, g, b;
};

typedef enum { SELECTION_left, SELECTION_right } selection_t;

struct selection {
  struct textbox *textbox;
  
  int32_t start, end;
  selection_t direction;

  struct selection *next;
};

struct textbox {
  struct tab *tab;
  
  struct sequence *text;

  int32_t cursor;
  struct selection *csel;
  
  struct colour bg;

  cairo_t *cr;
  cairo_surface_t *sfc;

  int yoff;
  int linewidth, height;
};

struct tab {
  uint8_t *name;

  int x, y, width, height;
  
  struct textbox *action, *main;

  struct tab *next;
};

void
init(void);

void
luainit(void);

void
fontinit(void);

int
fontset(const uint8_t *name, size_t size);

bool
loadglyph(int32_t code);

/* Checks if code warrents a line break.
   May update l if code requires consuming more code points such as a 
   "\r\n". */
bool
islinebreak(int32_t code, uint8_t *s, int32_t max, int32_t *l);

/* Checks if code is a word break. */
bool
iswordbreak(int32_t code);

void
command(struct textbox *main, uint8_t *s);

struct tab *
tabnew(uint8_t *name, size_t len);

void
tabfree(struct tab *t);

bool
tabresize(struct tab *t, int x, int y, int w, int h);

void
tabdraw(struct tab *t);

void
tabscroll(struct tab *t, int x, int y, int dy);

void
tabbuttonpress(struct tab *t, int x, int y,
	       unsigned int button);

void
tabbuttonrelease(struct tab *t, int x, int y,
		 unsigned int button);

void
tabmotion(struct tab *t, int x, int y);

struct textbox *
textboxnew(struct tab *tab,
	   struct colour *bg);

void
textboxfree(struct textbox *t);

bool
textboxresize(struct textbox *t, int w);

void
textboxscroll(struct textbox *t, int x, int y, int dy);

void
textboxbuttonpress(struct textbox *t, int x, int y,
		   unsigned int button);

void
textboxbuttonrelease(struct textbox *t, int x, int y,
		     unsigned int button);

void
textboxmotion(struct textbox *t, int x, int y);

void
textboxtyping(struct textbox *t, uint8_t *s, size_t l);

void
textboxkeypress(struct textbox *t, keycode_t k);

void
textboxkeyrelease(struct textbox *t, keycode_t k);

struct selection *
selectionnew(struct textbox *t, int32_t pos);

void
selectionfree(struct selection *s);

bool
selectionupdate(struct selection *s, int32_t pos);

struct selection *
inselections(struct textbox *t, int32_t pos);



struct sequence *
sequencenew(void);

void
sequencefree(struct sequence *s);

bool
sequenceinsert(struct sequence *s, size_t pos,
	       uint8_t *data, size_t len);

bool
sequencedelete(struct sequence *s, size_t pos, size_t len);

bool
sequencefindword(struct sequence *s, size_t pos,
		 size_t *start, size_t *len);

bool
sequencecopytobuf(struct sequence *s, size_t pos,
		  uint8_t *buf, size_t len);



void
handletyping(uint8_t *s, size_t n);

void
handlekeypress(keycode_t k);

void
handlekeyrelease(keycode_t k);

void
handlebuttonpress(int x, int y, int b);

void
handlebuttonrelease(int x, int y, int b);

void
handlemotion(int x, int y);

void
handlescroll(int x, int y, int dy);


extern int width, height;
extern cairo_t *cr;

extern FT_Face face;
extern int baseline, lineheight;

extern struct tab *tab;
extern struct textbox *focus;
extern struct selection *selections;
