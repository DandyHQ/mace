#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define SEQ_start  0
#define SEQ_end   1
#define SEQ_first   2

struct piece {
	ssize_t prev, next;
	size_t len;
	size_t pos; /* In sequence */
	size_t off; /* In data buffer */
};

struct sequence {
  struct piece *pieces;
  size_t plen, pmax;

  uint8_t *data;
  size_t dlen, dmax;
};

/* Creates a new sequence around data, data may be NULL.
   len is the number of set bytes in data, max is the current size of the
   allocation for data. 
*/

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

size_t
sequencecodepoint(struct sequence *s, size_t pos, int32_t *code);

size_t
sequenceprevcodepoint(struct sequence *s, size_t pos, int32_t *code);

/* Fulls out buf with the contentes of the sequence starting at pos
   and going for at most len bytes. Does NOT null terminate buf. */

size_t
sequenceget(struct sequence *s, size_t pos,
            uint8_t *buf, size_t len);

size_t
sequencelen(struct sequence *s);

