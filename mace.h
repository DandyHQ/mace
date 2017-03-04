
#define NAMEMAX 32

struct tab {
  struct tab *next;

  uint8_t name[NAMEMAX];
  uint8_t *content;
  size_t len;
};

typedef enum { SPLIT_none, SPLIT_horz, SPLIT_vert } split_t;

struct pane {
  float ratio;
  split_t split;
  struct pane *a, *b;

  /* Color for now */
  char cr, cg, cb;
};

bool
init(void);

void
resize(char *nbuf, size_t w, size_t h, size_t bpp);

void
redraw(void);

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

extern size_t width, height, bpp;
extern char *buf;

extern struct pane *root;
