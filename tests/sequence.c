#include <string.h>

#include "../resources/Unity/src/unity.h"
#include "sequence.h"

void
test_sequencenewfree(void)
{
	struct sequence *s;

	s = sequencenew(NULL, 0);

	TEST_ASSERT_NOT_NULL(s);

	sequencefree(s);	
}

void
test_sequenceinsert(void)
{

}

void
test_sequencelen(void)
{

}

void
test_sequencedelete(void)
{

}

void
test_sequencefindword(void)
{
	uint8_t *str = "Hello there. This is a test";

	/* Word starts and lengths. */
	size_t starts[] = { 0, 6, 13, 18, 21, 23 };
	size_t lens[]   = { 5, 5, 4, 2, 1, 4 };

	/* Test points */
	size_t p[]      = { 0, 2, 10, 12, 26, 27 };
	/* Associated word (-1 for fail) */
	ssize_t w[]     = { 0, 0,  1, -1, 5, -1 };

	struct sequence *s;
	size_t i, start, len;
	bool r;

	s = sequencenew(NULL, 0);
	TEST_ASSERT_NOT_NULL(s);

	sequenceinsert(s, 0, str, strlen(str));

	for (i = 0; i < sizeof(p)/sizeof(p[0]); i++) {
		r = sequencefindword(s, p[i], &start, &len);

		TEST_ASSERT_EQUAL_INT(w[i] != -1, r);

		if (!r) /* Failed to find word so don't check boundaries. */
			continue;

		TEST_ASSERT_EQUAL_INT(starts[w[i]], start);
		TEST_ASSERT_EQUAL_INT(lens[w[i]], len);
	}

	sequencefree(s);
}

/* Copied from tests/utf8.c */

uint8_t   su[] = { 0x24, 0xc2, 0xa2, 0xe2, 0x82, 0xac, 0xf0, 0xa4, 0xad, 0xa2 };
int32_t   sU[] = { 0x24, 0xa2, 0x20ac, 0x24b62 };
size_t    sl[] = { 1, 2, 3, 4 };

size_t sumax = sizeof(su)/sizeof(su[0]);
size_t sUmax = sizeof(sU)/sizeof(sU[0]);
size_t slmax = sizeof(sl)/sizeof(sl[0]);

void
test_sequencecodepoint(void)
{
	size_t l, ou, oU, ol;
	struct sequence *s;
	int32_t code;

	s = sequencenew(NULL, 0);
	TEST_ASSERT_NOT_NULL(s);

	sequenceinsert(s, 0, su, sumax);

	ou = oU = ol = 0;
	while (ou != sumax && oU != sUmax && ol != slmax) {
		TEST_ASSERT(ou < sumax);
		TEST_ASSERT(oU < sUmax);
		TEST_ASSERT(ol < slmax);

		l = sequencecodepoint(s, ou, &code);

		TEST_ASSERT_EQUAL_INT(sl[ol], l);
		TEST_ASSERT_EQUAL_INT32(sU[oU], code);

		ou += l;
		ol++;
		oU++;
	}

	sequencefree(s);
}

void
test_sequenceprevcodepoint(void)
{
	size_t l, ou, oU, ol;
	struct sequence *s;
	int32_t code;

	s = sequencenew(NULL, 0);
	TEST_ASSERT_NOT_NULL(s);

	sequenceinsert(s, 0, su, sumax);
	
	ou = sumax;
	oU = sUmax;
	ol = slmax;

	while (ou > 0 && oU > 0 && ol > 0) {
		oU--;
		ol--;

		l = sequenceprevcodepoint(s, ou, &code);

		TEST_ASSERT_EQUAL_INT(sl[ol], l);
		TEST_ASSERT_EQUAL_INT32(sU[oU], code);

		ou -= l;
	}

	TEST_ASSERT_EQUAL_INT(0, ou);
	TEST_ASSERT_EQUAL_INT(0, oU);
	TEST_ASSERT_EQUAL_INT(0, ol);
}

void
test_sequenceget(void)
{

}

int
main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_sequencenewfree);
	RUN_TEST(test_sequenceinsert);
	RUN_TEST(test_sequencelen);
	RUN_TEST(test_sequencedelete);
	RUN_TEST(test_sequencefindword);
	RUN_TEST(test_sequencecodepoint);
	RUN_TEST(test_sequenceprevcodepoint);
	RUN_TEST(test_sequenceget);

	return UNITY_END();
}