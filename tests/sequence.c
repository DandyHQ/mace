#include <string.h>
#include <stdlib.h>

#include "resources/unity/src/unity.h"
#include "sequence.h"

void
test_sequencenewfree(void)
{
	struct sequence *s;
	uint8_t *buf;
	size_t len;

	/* Test without starting data. */
	s = sequencenew(NULL, 0);
	TEST_ASSERT_NOT_NULL(s);
	sequencefree(s);	

	/* Test with starting data. */
	len = 512;
	buf = malloc(sizeof(uint8_t) * len);
	TEST_ASSERT_NOT_NULL(buf);
	
	s = sequencenew(buf, len);
	TEST_ASSERT_NOT_NULL(s);
	sequencefree(s);
}

void
test_sequencereplaceget(void)
{
	uint8_t *str = (uint8_t *) "This is a test.";
	size_t delstart = 0, delend = 8;
	uint8_t *delstr = (uint8_t *) "a test.";
	size_t repstart = 2, repend = 6;
	uint8_t *replacestr = (uint8_t *) "strange\tthing indeed\n";
	uint8_t *endstr = (uint8_t *) "a strange\tthing indeed\n.";
	
	struct sequence *s;
	uint8_t buf[512], *bbuf;
	size_t len, l;
	bool r;
	int i;
	
	s = sequencenew(NULL, 0);

	TEST_ASSERT_NOT_NULL(s);
	
	r = sequencereplace(s, 0, 0, str, strlen((char *) str));
	
	TEST_ASSERT_TRUE(r);
	
	len = sequenceget(s, 0, buf, sizeof(buf));
	
	TEST_ASSERT_EQUAL_INT(strlen((char *) str), len);
	
	TEST_ASSERT_EQUAL_UINT8_ARRAY(str, buf, len);
	
	/* Try get something past the end. */
	
	len = sequenceget(s, strlen((char *) str) + 100, 
	                  buf, sizeof(buf));
	
	/* Should fail. */
	TEST_ASSERT_EQUAL_INT(0, len);
	
	/* Try replace something past the end. */
	r = sequencereplace(s, strlen((char *) str) + 100, 
	                    strlen((char *) str) + 200, 
	                    NULL, 0);
	
	/* Should fail. */
	TEST_ASSERT_TRUE(!r);
	
	/* Try replace something overlapping the end. */
	r = sequencereplace(s, strlen((char *) str) / 2, 
	                    strlen((char *) str) + 200, 
	                    NULL, 0);
	
	/* Should fail. */
	TEST_ASSERT_TRUE(!r);

	/* Now test pure removing stuff. */
	r = sequencereplace(s, delstart, delend, NULL, 0);
	
	TEST_ASSERT_TRUE(r);
	
	len = sequenceget(s, 0, buf, sizeof(buf));
	
	TEST_ASSERT_EQUAL_INT(strlen((char *) delstr), len);
	
	TEST_ASSERT_EQUAL_UINT8_ARRAY(delstr, buf, len);	
	
	/* Now test replacing stuff. */
	r = sequencereplace(s, repstart, repend, replacestr,
	                    strlen((char *) replacestr));
	
	TEST_ASSERT_TRUE(r);
	
	len = sequenceget(s, 0, buf, sizeof(buf));
	
	TEST_ASSERT_EQUAL_INT(strlen((char *) endstr), len);
	
	TEST_ASSERT_EQUAL_UINT8_ARRAY(endstr, buf, len);

	/* Now try add a whole heap. */
	for (i = 0; i < 1000; i++) {
		r = sequencereplace(s, 0, 0, str, strlen((char *) str));
		TEST_ASSERT_TRUE(r);
	}
	
	/* Try to grow the last piece. */
	
	for (i = 0; i < 1000; i++) {
		len = sequencelen(s);
		r = sequencereplace(s, len, len, str, strlen((char *) str));
		TEST_ASSERT_TRUE(r);
	}
	
	len = sequencelen(s);
	
	bbuf = malloc(len);
	TEST_ASSERT_NOT_NULL(bbuf);
	
	l = sequenceget(s, 0, bbuf, len);
	
	TEST_ASSERT_EQUAL_INT(len, l);
	
	/* Should probably check if it was good? */
	
	free(bbuf);
	
	/* No way should it be able to allocate this much, at least 
	   lets hope it cant. */

	r = sequencereplace(s, 0, 0, NULL, SIZE_MAX / 2);
	TEST_ASSERT_TRUE(!r);
	
	sequencefree(s);	
}

void
test_sequencelen(void)
{
	uint8_t *str = (uint8_t *) "This is a test.";
	struct sequence *s;
	size_t len;
	bool r;
	
	s = sequencenew(NULL, 0);

	TEST_ASSERT_NOT_NULL(s);
	
	r = sequencereplace(s, 0, 0, str, strlen((char *) str));
	
	TEST_ASSERT_TRUE(r);
	
	len = sequencelen(s);
	
	TEST_ASSERT_EQUAL_INT(strlen((char *) str), len);

	sequencefree(s);	
}

void
test_sequencefindword(void)
{
	uint8_t *str = (uint8_t *) "Hello there. This is a test";

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

	r = sequencereplace(s, 0, 0, str, strlen((char *) str));
	TEST_ASSERT_TRUE(r);

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
	bool r;

	s = sequencenew(NULL, 0);
	TEST_ASSERT_NOT_NULL(s);

	/* Load it in in parts */
	r = sequencereplace(s, 0, 0, su, 3);
	TEST_ASSERT_TRUE(r);
	
	r = sequencereplace(s, 3, 3, su + 3, sumax - 3);
	TEST_ASSERT_TRUE(r);

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
	
	/* Try get a code point at the end. */
	l = sequencecodepoint(s, sumax, &code);
	/* Should fail. */
	TEST_ASSERT_EQUAL_INT(0, l);

	/* Try get a code point from past the end. */
	l = sequencecodepoint(s, sumax + 100, &code);
	/* Should fail */
	TEST_ASSERT_EQUAL_INT(0, l);
	
	sequencefree(s);
}

void
test_sequenceprevcodepoint(void)
{
	size_t l, ou, oU, ol;
	struct sequence *s;
	int32_t code;
	bool r;

	s = sequencenew(NULL, 0);
	TEST_ASSERT_NOT_NULL(s);

	/* Load it in in parts */
	r = sequencereplace(s, 0, 0, su, 3);
	TEST_ASSERT_TRUE(r);
	
	r = sequencereplace(s, 3, 3, su + 3, sumax - 3);
	TEST_ASSERT_TRUE(r);
	
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
	
	/* Try get a code point at the start. */
	l = sequenceprevcodepoint(s, 0, &code);
	/* Should fail. */
	TEST_ASSERT_EQUAL_INT(0, l);

	/* Try get a code point from past the end. */
	l = sequenceprevcodepoint(s, sumax + 100, &code);
	/* Should fail */
	TEST_ASSERT_EQUAL_INT(0, l);
	
	sequencefree(s);
}

int
main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_sequencenewfree);
	RUN_TEST(test_sequencereplaceget);
	RUN_TEST(test_sequencelen);
	RUN_TEST(test_sequencefindword);
	RUN_TEST(test_sequencecodepoint);
	RUN_TEST(test_sequenceprevcodepoint);

	return UNITY_END();
}