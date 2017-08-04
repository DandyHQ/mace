#include "../resources/unity/src/unity.h"
#include "../utf8.h"

/* Should add more tests for these cases but that is 
   too much work.
   Or don't test these first few functions. They don't
   really have any logic. There tests are basically there
   implimentations.
   
   Do need to add some tests with invalid utf8 though.
*/

void
test_islinebreak(void)
{
	TEST_ASSERT_TRUE(islinebreak('\n'));
	TEST_ASSERT_TRUE(islinebreak('\r'));
}

void
test_iswordbreak(void)
{
	TEST_ASSERT_TRUE(iswordbreak(' '));
	TEST_ASSERT_TRUE(iswordbreak('\t'));
	TEST_ASSERT_TRUE(iswordbreak('\r'));
	TEST_ASSERT_TRUE(iswordbreak('\n'));
	/* Should this really be a word break? In natural languages
	   yes but not in programming languages. */
	TEST_ASSERT_TRUE(iswordbreak('.'));
}

void
test_iswhitespace(void)
{
	TEST_ASSERT_TRUE(iswhitespace(' '));
	TEST_ASSERT_TRUE(iswhitespace('\t'));
}

uint8_t   su[] = { 0x24, 0xc2, 0xa2, 0xe2, 0x82, 0xac, 0xf0, 0xa4, 0xad, 0xa2 };
int32_t   sU[] = { 0x24, 0xa2, 0x20ac, 0x24b62 };
size_t    sl[] = { 1, 2, 3, 4 };

size_t sumax = sizeof(su)/sizeof(su[0]);
size_t sUmax = sizeof(sU)/sizeof(sU[0]);
size_t slmax = sizeof(sl)/sizeof(sl[0]);

void
test_iterate(void)
{
	size_t l, ou, oU, ol;
	int32_t c;
	
	ou = oU = ol = 0;
	while (ou != sumax && oU != sUmax && ol != slmax) {
		TEST_ASSERT(ou < sumax);
		TEST_ASSERT(oU < sUmax);
		TEST_ASSERT(ol < slmax);

		l = utf8iterate(su + ou, sizeof(su) - ou, &c);

		TEST_ASSERT_EQUAL_INT(sl[ol], l);
		TEST_ASSERT_EQUAL_INT32(sU[oU], c);

		ou += l;
		ol++;
		oU++;
	}
	
	/* What if we go past the end? */
	l = utf8iterate(su + ou, 0, &c);
	TEST_ASSERT_EQUAL_INT(0, l);
}

void
test_deiterate(void)
{
	size_t l, ou, oU, ol;
	int32_t c;
	
	ou = sumax;
	oU = sUmax;
	ol = slmax;

	while (ou > 0 && oU > 0 && ol > 0) {
		oU--;
		ol--;

		l = utf8deiterate(su, sizeof(su), ou, &c);

		TEST_ASSERT_EQUAL_INT(sl[ol], l);
		TEST_ASSERT_EQUAL_INT32(sU[oU], c);

		ou -= l;
	}

	TEST_ASSERT_EQUAL_INT(0, ou);
	TEST_ASSERT_EQUAL_INT(0, oU);
	TEST_ASSERT_EQUAL_INT(0, ol);
	
	/* What if we go past the start? */
	l = utf8iterate(su, 0, &c);
	TEST_ASSERT_EQUAL_INT(0, l);
}

void
test_codepoints(void)
{
	size_t len;

	len = utf8codepoints(su, sumax);
	TEST_ASSERT_EQUAL_INT(sUmax, len);
}

void
test_encode(void)
{
	uint8_t enc[sumax];
	size_t l, ou, oU, ol;

	ou = oU = ol = 0;
	while (ou != sumax && oU != sUmax && ol != slmax) {
		TEST_ASSERT(ou < sumax);
		TEST_ASSERT(oU < sUmax);
		TEST_ASSERT(ol < slmax);

		l = utf8encode(enc + ou, sizeof(enc) - ou, sU[oU]);
		TEST_ASSERT_EQUAL_INT(sl[ol], l);

		ou += l;
		oU++;
		ol++;
	}

	TEST_ASSERT_EQUAL_UINT8_ARRAY(su, enc, sumax);
}

int
main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_islinebreak);
	RUN_TEST(test_iswordbreak);
	RUN_TEST(test_iswhitespace);
	RUN_TEST(test_iterate);
	RUN_TEST(test_deiterate);
	RUN_TEST(test_codepoints);
	RUN_TEST(test_encode);

	return UNITY_END();
}