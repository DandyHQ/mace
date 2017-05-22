#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "sequence.h"

/* This needs to be improved. Needs some checks on how the chain is
   held together. eg 1 -> 2 -> 3 and 3 <- 2 <- 1. */

void
luaremove(lua_State *L, void *addr)
{
  /* Stub for sequences as in mace they can be accessed from lua, so
     when freed lua needs to know to stop using them. */
}

int
main(int argc, char *argv[])
{
  struct sequence *s;
  uint8_t *s1 = (uint8_t *) "Hello there.";
  uint8_t *s2 = (uint8_t *) "This is a test.";
  uint8_t *s12 = (uint8_t *) "Hello there.This is a test.";
  uint8_t *s3 = (uint8_t *) "Hello test.";
  uint8_t buf[512];
  size_t len;

  printf("Make empty sequence\n");
  s = sequencenew(NULL, 0);
  if (s == NULL) {
    return 1;
  }

  if (!sequenceinsert(s, 0, s1, strlen((const char *) s1))) {
    printf("failed to insert s1!\n");
    return 2;
  }
  
  if (!sequenceinsert(s, strlen((const char *) s1),
		      s2, strlen((const char *) s2))) {

    printf("failed to insert s2!\n");
    return 3;
  }

  len = sequenceget(s, 0, buf, sizeof(buf));
  if (len != strlen((const char *) s12)) {
    printf("length wrong after insert have %zu should have %zu\n",
	   len, strlen((const char *) s12));
    return 4;
  }

  if (strcmp((const char *) buf, (const char *) s12) != 0) {
    printf("retrieved string '%s' and expected string '%s' do not match!\n",
	   buf, s12);
    return 5;
  }

  if (!sequencedelete(s, 6, 16)) {
    printf("sequence delete failed!\n");
    return 6;
  }

  len = sequenceget(s, 0, buf, sizeof(buf));

  if (len != strlen((const char *) s3)) {
    printf("length wrong after delete have %zu should have %zu\n",
	   len, strlen((const char *) s3));
    return 7;
  }

  if (strcmp((const char *) buf, (const char *) s3) != 0) {
    printf("retrieved string '%s' and expected string '%s' do not match!\n",
	   buf, s3);
    return 8;
  }
 
  printf("free sequence\n");
  sequencefree(s);

  return 0;
}
