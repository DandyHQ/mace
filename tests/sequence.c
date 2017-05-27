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
  size_t len, s1len, s2len, s12len, s3len;
  const char *s1 = "Hello there.";
  const char *s2 = "This is a test.";
  const char *s12 = "Hello there.This is a test.";
  const char *s3 = "Hello test.";
  struct sequence *s;
  uint8_t buf[512];

  s1len = strlen(s1);
  s2len = strlen(s2);
  s12len = strlen(s12);
  s3len = strlen(s3);
  
  printf("Start with empty sequence\n");
  s = sequencenew(NULL, 0);
  if (s == NULL) {
    return 1;
  }

  printf("Insert %zu : '%s'\n", s1len, s1);

  if (!sequenceinsert(s, 0, (const uint8_t *) s1, s1len)) {
    printf("Failed to insert s1!\n");
    return 2;
  }

  len = sequenceget(s, 0, buf, sizeof(buf));
  if (len != s1len) {
    printf("Retrieved string has incorrect length: %zu expected %zu\n",
	   len, s1len);
    return 3;
  }

  if (strcmp((const char *) buf, s1) != 0) {
    printf("Retrieved string '%s' does not match expected '%s'!\n",
	   buf, s1);
    return 4;
  }
  
  if (!sequenceinsert(s, s1len, (const uint8_t *) s2, s2len)) {
    printf("Failed to insert s2!\n");
    return 5;
  }

  len = sequenceget(s, 0, buf, sizeof(buf));
  if (len != s12len) {
    printf("Retrieved string has incorrect length: %zu expected %zu\n",
	   len, s12len);
    return 6;
  }

  if (strcmp((const char *) buf, s12) != 0) {
    printf("Retrieved string '%s' does not match expected '%s'!\n",
	   buf, s12);
    return 7;
  }

  if (!sequencedelete(s, 6, 16)) {
    printf("Sequence delete failed!\n");
    return 8;
  }

  len = sequenceget(s, 0, buf, sizeof(buf));
  if (len != s3len) {
    printf("Length after delete have %zu should have %zu\n",
	   len, s3len);
    return 9;
  }

  if (strcmp((const char *) buf, (const char *) s3) != 0) {
    printf("Retrieved string '%s' does not match expected '%s'!\n",
	   buf, s3);
    return 10;
  }
 
  printf("free sequence\n");
  sequencefree(s);

  printf("Sequence tested\n");
  return 0;
}
