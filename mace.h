
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
  struct mace *mace;
  
  struct piece *pieces;
  size_t plen, pmax;

  uint8_t *data;
  size_t dlen, dmax;
};

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

  struct sequence *sequence;

  int32_t cursor;

  struct selection *csel, *selections;
  bool cselisvalid;
  
  struct colour bg;

  cairo_t *cr;
  cairo_surface_t *sfc;

  int yoff;
  int linewidth, height;

  int maxheight;

  size_t startpos;
  int starty;
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

/* Structs that lua uses must call this before freeing themselves. */
/* Currently tabs, textboxs, sequences, and selections */
void
luaremove(lua_State *L, void *addr);

void
luaruninit(lua_State *L);

void
command(lua_State *L, uint8_t *s);



struct font *
fontnew(void);

void
fontfree(struct font *font);

int
fontset(struct font *font, const uint8_t *pattern);

bool
loadglyph(FT_Face face, int32_t code);

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

void
textboxpredraw(struct textbox *t,
	       bool startchanged,
	       bool heightchanged);



/* Creates a new sequence around data, data may be NULL.
   len is the number of set bytes in data, max is the size of the
   allocation for data. */

struct sequence *
sequencenew(struct mace *mace, uint8_t *data,
	    size_t len, size_t max);

/* Frees a sequence and all it's pieces. */

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

ssize_t
sequencefindpiece(struct sequence *s, size_t pos, size_t *i);


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
