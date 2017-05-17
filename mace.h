
/* Rounds X up to nearest multiple of M, M must be a power of 2. */
#define RDUP(X, M) ((X + M - 1) & ~(M - 1))


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

/* A sequence is a collection of pieces that manages a string of text.
   Text can be added to a sequence at any position, text can be
   removed at any position, and text can be retreived from the
   sequence.
 
   The sequence is managed in such as way that no text is ever lost,
   so undo's should be easy to manage. We will just need a way of
   tracking the pieces that have been replaced.
*/

struct piece {
  ssize_t prev, next;
  size_t len;
  size_t pos; /* In sequence */
  size_t off; /* In data buffer */
};

#define SEQ_start  0
#define SEQ_end    1 
#define SEQ_first  2

struct sequence {
  struct piece *pieces;
  size_t plen, pmax;

  uint8_t *data;
  size_t dlen, dmax;
};

struct colour {
  double r, g, b;
};

/* Selections are handled horrible. They are a linked list that is
   global to a mace instance (effectively global at this point). There
   is no nice way to traverse them, nor to use them from lua. Some
   redeisgn is needed here. Especially with how they interact with
   text boxes. */

typedef enum { SELECTION_left, SELECTION_right } selection_t;

struct selection {
  struct textbox *textbox;

  int32_t start, end;

  selection_t direction;

  struct selection *next;
};

struct textbox {
  struct tab *tab;
  
  struct sequence *sequence;
  int32_t cursor;

  struct selection *csel;
  bool cselisvalid;
  
  struct colour bg;

  cairo_t *cr;
  cairo_surface_t *sfc;

  int yoff;
  int linewidth, height;
  int maxheight;
};

struct tab {
  uint8_t *name;

  int x, y, width, height;
  
  struct textbox *action, *main;
  struct tab *next;
};

struct mace {
  bool running;
  
  cairo_t *cr;

  FT_Library fontlibrary;
  FT_Face fontface;
  int baseline, lineheight;

  lua_State *lua;
  
  struct tab *tabs;
  struct textbox *focus;

  struct selection *selections;
};



struct mace *
macenew(cairo_t *cr);

void
macefree(struct mace *mace);

void
macequit(struct mace *mace);



int
luainit(struct mace *mace);

void
luaend(struct mace *mace);

/* Structs that lua uses must call this before freeing themselves. */
/* Currently tabs, textboxs, sequences, and selections */
void
luaremove(void *addr);

void
command(uint8_t *s);



int
fontinit(struct mace *mace);

void
fontend(struct mace *mace);

int
fontset(struct mace *mace, const uint8_t *pattern);

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



/* Takes ownership of seq */

struct textbox *
textboxnew(struct tab *tab, struct colour *bg,
	   struct sequence *seq);

void
textboxfree(struct textbox *t);

bool
textboxresize(struct textbox *t, int width, int maxheight);

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

void
textboxpredraw(struct textbox *t);



/* Creates a new empty sequence around data, data may be NULL.
   dlen is the number of set bytes in data, dmax is the size of the
   allocation for data. */

struct sequence *
sequencenew(uint8_t *data, size_t len, size_t max);

/* Frees a sequence and all it's pieces, does not remove any
   selections that point to it. */

void
sequencefree(struct sequence *s);

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

/* Fulls out buf with the contentes of the sequence starting at pos
   and going for at most len bytes, buf must be of at least len + 1
   bytes and will be null terminated. */

size_t
sequenceget(struct sequence *s, size_t pos,
	    uint8_t *buf, size_t len);


/* Adds a new selection to the selection list */

struct selection *
selectionadd(struct textbox *t, int32_t pos);

/* Removes all previous selections */

struct selection *
selectionreplace(struct textbox *t, int32_t pos);

/* Free's the selection */
void
selectionremove(struct selection *s);

/* Updates the selection to include pos, returns true if something
   changed, false otherwise. */

bool
selectionupdate(struct selection *s, int32_t pos);

/* Checks if the position pos in the textbox t is in a selection,
   returns the selection if it is. */

struct selection *
inselections(struct textbox *t, int32_t pos);



/* Handle UI events */

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


/* This is not as seperated as I would like. In future we should be
   able to have multiple instances of mace. Why this is good I'm not
   sure but global variables tend to be messy. */

extern struct mace *mace;
