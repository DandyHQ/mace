#include <stdio.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mace.h"

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
  uint8_t *s1 = "Hello there.";
  uint8_t *s2 = "This is a test.";
  uint8_t *s12 = "Hello there.This is a test.";
  uint8_t *s3 = "Hello test.";
  uint8_t buf[512];
  size_t len;

  printf("Make empty sequence\n");
  s = sequencenew(NULL, 0, 0);
  if (s == NULL) {
    return 1;
  }

  if (!sequenceinsert(s, 0, s1, strlen(s1))) {
    printf("failed to insert s1!\n");
    return 2;
  }
  
  if (!sequenceinsert(s, strlen(s1), s2, strlen(s2))) {
    printf("failed to insert s2!\n");
    return 3;
  }

  len = sequenceget(s, 0, buf, sizeof(buf));
  if (len != strlen(s12)) {
    printf("length wrong after insert have %zu should have %zu\n",
	   len, strlen(s12));
    return 4;
  }

  if (strcmp(buf, s12) != 0) {
    printf("retrieved string '%s' and expected string '%s' do not match!\n",
	   buf, s12);
    return 5;
  }

  if (!sequencedelete(s, 6, 16)) {
    printf("sequence delete failed!\n");
    return 6;
  }

  len = sequenceget(s, 0, buf, sizeof(buf));

  if (len != strlen(s3)) {
    printf("length wrong after delete have %zu should have %zu\n",
	   len, strlen(s3));
    return 7;
  }

  if (strcmp(buf, s3) != 0) {
    printf("retrieved string '%s' and expected string '%s' do not match!\n",
	   buf, s3);
    return 8;
  }
 
  printf("free sequence\n");
  sequencefree(s);

  return 0;
}
