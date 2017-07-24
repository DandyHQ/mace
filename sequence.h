#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define SEQ_start   0
#define SEQ_end     1

#define CHANGE_root 0

struct piece {
	/* Offset of data in data buffer.
	   Never changes. */
	size_t off;
	
	/* Length of data.
	   Only changes if the piece was the last added
	   and more text is added onto the end of it. */
	size_t len;
	
	/* Position in sequence.
	   Changes whenever it needs to. */
	size_t pos;
	
	/* Previous and next piece in linked list.
	   Changes as it needs to. */
	ssize_t prev, next;
};

struct change {
	/* Pieces that were added. */
	ssize_t apieces[3];
	size_t napieces;
	
	/* Start and end of slice of pieces that were removed.
	   Should only be -1 if this change is the root change
	   which should never be undone. */
	ssize_t rstart, rend;
	
	/* Previous and next piece around pieces. */
	ssize_t prev, next; 
	
	/* Positions in sequence's changes array. */
	ssize_t parent;
	ssize_t children; /* Head of my sibling list. */
	ssize_t sibling; /* Next of my siblings. */
};

struct sequence {
  struct piece *pieces;
  size_t plen, pmax;
  
  struct change *changes;
  size_t clen, cmax;
  ssize_t changehead;

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

/* Finds the piece after piece p that contains index pos.
   Sets i to be the offset of pos in the piece and returns 
   the piece. */
ssize_t
sequencepiecefind(struct sequence *s, 
                  ssize_t p, 
                  size_t pos,
                  size_t *i);
                  
/* 
   Replaces the text from and including begin up to but not
   including end with the len bytes in data. 
   data must be valid utf8 (encoding, bad code points don't matter) 
   otherwise bad things will probably happen.
   If len is zero this works as a delete. If end is equal to begin
   this works as an insert. 
*/

bool
sequencereplace(struct sequence *s, 
                size_t begin, size_t end,
                const uint8_t *data, size_t len);

size_t
sequencecodepoint(struct sequence *s, size_t pos, int32_t *code);

size_t
sequenceprevcodepoint(struct sequence *s, size_t pos, int32_t *code);

/* Fulls out buf with the contentes of the sequence starting at pos
   and going for at most len bytes. Does NOT null terminate buf.
   
   Yes you could use sequencecodepoint in a loop but this will be
   much faster.
*/

size_t
sequenceget(struct sequence *s, size_t pos,
            uint8_t *buf, size_t len);

size_t
sequencelen(struct sequence *s);

/* Attempts to find the word that the byte pos is part of. */

bool
sequencefindword(struct sequence *s, size_t pos,
                 size_t *start, size_t *len);

/* Go up change tree. */
bool
sequencechangeup(struct sequence *s);

/* Go down change tree. */
bool
sequencechangedown(struct sequence *s);

/* Cycle through sibling branches. */
bool
sequencechangecycle(struct sequence *s);


size_t
sequenceindexnextline(struct sequence *s, size_t i);

size_t
sequenceindexprevline(struct sequence *s, size_t i);



/* For debugging. */

/* Prints tree from change c with string prefix h. */
void
sequenceprintchangetree(struct sequence *s, char *h, ssize_t c);

/* Prints the sequence in a format that shows pieces. */
void
sequenceprint(struct sequence *s);
