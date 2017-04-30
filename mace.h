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
  struct piece *prev, *next;

  size_t rl; /* Allocated length of s. */
  size_t pl; /* Currently populated length of s. */
  uint8_t *s;
};

struct colour {
  double r, g, b;
};

struct textbox {
  struct tab *tab;
  
  struct piece *pieces;

  int32_t cursor;
  struct piece *cpiece;

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
fontinit(void);

int
fontload(const uint8_t *name, size_t size);

bool
loadglyph(int32_t code);

bool
islinebreak(int32_t code, uint8_t *s, int32_t max, int32_t *l);

bool
iswordbreak(int32_t code);
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
piecefind(struct piece *pieces, int pos, int *i);

/* Finds the start and end of a word around pos. Returns false if pos
   is not inside a word.
*/
bool
piecefindword(struct piece *pieces, int pos, int *start, int *end);

/* Splits the piece p and position pos and sets lr and rr to point
   to the newly created left and right pieces.
*/
bool
piecesplit(struct piece *p, size_t pos,
	   struct piece **lr, struct piece **rr);

/* Splits old at position pos and inserts a new piece created by
   copying the string s of lenght l. Returns the new piece.
*/
struct piece *
pieceinsert(struct piece *old, size_t pos,
	    uint8_t *s, size_t l);

/* Appends string s of len l to the piece. Returns false if this was
   not possible else true.
*/
bool
pieceappend(struct piece *p, uint8_t *s, size_t l);


extern int width, height;
extern cairo_t *cr;

extern FT_Face face;
extern int baseline, lineheight;

extern struct tab *tab;
extern struct textbox *focus;
