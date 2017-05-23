#include "lib.h"

bool
islinebreak(int32_t code, uint8_t *s, int32_t max, int32_t *l)
{
  size_t a;
  
  switch (code) {
  default:
    return false;

  case 0x000A: /* Line Feed */
    return true;

  case 0x000B: /* Vertical Tab */
    return true;

  case 0x000C: /* Form Feed */
    return true;

  case 0x000D: /* Carriage Return */
    a = utf8iterate(s + *l, max - *l, &code);
    if (a > 0 && code == 0x000A) {
      /* Carriage Return Followed by Line Feed. */
      *l += a;
      return true;
    } else {
      return true;
    }

  case 0x0085: /* Next Line */
    return true;

  case 0x2028: /* Line Separator */
    return true;

  case 0x2029: /* Paragraph Separator */
    return true;
  }
}

bool
iswordbreak(int32_t code)
{
  switch (code) {
  default:
    return false;

  case '\n':
  case ' ':
    return true;

    /* TODO: other word breaks. */
  }
}

/*
1 7  U+0000  U+007F   0xxxxxxx 			
2 11 U+0080  U+07FF   110xxxxx 10xxxxxx 		
3 16 U+0800  U+FFFF   1110xxxx 10xxxxxx 10xxxxxx 	
4 21 U+10000 U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

*/

static uint8_t utfbyte[5] = { 0x80,    0, 0xc0, 0xe0, 0xf0 };
static uint8_t utfmask[5] = { 0xc0, 0x80, 0xe0, 0xf0, 0xf8 };

static int32_t
utf8decodebyte(uint8_t b, size_t *l)
{
  for (*l = 0; *l < 5; (*l)++) {
    if ((b & utfmask[*l]) == utfbyte[*l]) {
      return b & ~utfmask[*l];
    }
  }

  return 0;
}

size_t
utf8iterate(uint8_t *s, size_t slen, int32_t *code)
{
  size_t len, i, type;

  if (slen == 0) {
    return 0;
  }
  
  *code = utf8decodebyte(s[0], &len);
  if (len == 0 || len > 4) {
    return 0;
  }

  for (i = 1; i < slen && i < len; i++) {
    *code = (*code << 6) | utf8decodebyte(s[i], &type);
    if (type != 0) {
      return i;
    }
  }

  if (i < len) {
    return 0;
  }

  return i;
}

