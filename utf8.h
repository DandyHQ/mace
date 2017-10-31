#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

/* Checks if code warrents a line break.
   There should be some way of specifying if it needs to eat another byte
   such as with \n\r but I can not be bothered at the moment. */
bool
islinebreak(int32_t code);

/* Checks if code is a word break. */
bool
iswordbreak(int32_t code);

/* Checks if code is whitespace. */
bool
iswhitespace(int32_t code);

size_t
utf8iterate(const uint8_t *s, size_t len, int32_t *code);

/* Deiterate is now officially a word.
    Calculates the codepoint imediatly before off. */
size_t
utf8deiterate(const uint8_t *s, size_t slen, size_t off,
              int32_t *code);

size_t
utf8codepoints(const uint8_t *s, size_t len);

/* Encodes code into s if it can fit, returns the encoded length. */
size_t
utf8encode(uint8_t *s, size_t len, int32_t code);