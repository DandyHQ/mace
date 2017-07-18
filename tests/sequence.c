#include "unity.h"
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
	struct sequence *s;
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

}

void
test_sequencecodepoint(void)
{

}

void
test_sequenceprevcodepoint(void)
{

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