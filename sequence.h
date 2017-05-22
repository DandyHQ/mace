
#include "lib.h"

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

  int x, y, width;
};

#define SEQ_start  0
#define SEQ_end    1 
#define SEQ_first  2

struct sequence {
  lua_State *lua;
  
  struct piece *pieces;
  size_t plen, pmax;

  uint8_t *data;
  size_t dlen, dmax;
};

/* Creates a new sequence around data, data may be NULL.
   len is the number of set bytes in data, max is the size of the
   allocation for data. */

struct sequence *
sequencenew(uint8_t *data, size_t len);

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

size_t
sequencegetlen(struct sequence *s);
