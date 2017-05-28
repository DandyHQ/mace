#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

/* Checks if code warrents a line break.
   May update l if code requires consuming more code points such as a 
   "\r\n". */
bool
islinebreak(int32_t code, uint8_t *s, int32_t max, int32_t *l);

/* Checks if code is a word break. */
bool
iswordbreak(int32_t code);

size_t
utf8iterate(uint8_t *s, size_t len, int32_t *code);

size_t
utf8codepoints(uint8_t *s, size_t len);
