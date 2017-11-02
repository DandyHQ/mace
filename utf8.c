#include "utf8.h"
#include <stdio.h>

bool
islinebreak(int32_t code)
{
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
    return true;

  case 0x0085: /* Next Line */
    return true;

  case 0x2028: /* Line Separator */
    return true;

  case 0x2029: /* Paragraph Separator */
    return true;
  }
}

bool
iswhitespace(int32_t code)
{
  if (code == '\t' || code == ' ') {
    return true;
  }

  return false;
}

/* Where could we get a proper list for this? Do libraries exist
	for such things? */

static int32_t wordbreaks[] = {
  '\t',
  '\n',
  '\r',
  ' ',
  ',',
  '.',
  '(',
  ')',
  '[',
  ']',
  '|',
  '!',
  '@',
  '#',
  '$',
  '%',
  '^',
  '&',
  '+',
  ':',
  ';',
  '\'',
  '"',
};

bool
iswordbreak(int32_t code)
{
  int i;

  for (i = 0; i < sizeof(wordbreaks) / sizeof(wordbreaks[0]);
       i++) {
    if (code == wordbreaks[i]) {
      return true;
    }
  }

  return false;
}

/* Lots of help from st (http://st.suckless.org/) */
/*
1 7   U+0000  U+007F   0xxxxxxx
2 11  U+0080  U+07FF   110xxxxx 10xxxxxx
3 16  U+0800  U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
4 21  U+10000 U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/

#define UTF_MAX 0x10ffff

static uint8_t utfbyte[5] = { 0x80,    0, 0xc0, 0xe0, 0xf0 };
static uint8_t utfmask[5] = { 0xc0, 0x80, 0xe0, 0xf0, 0xf8 };
static size_t  utfmax[5]  = { 0, 0x7f, 0x7ff, 0xffff, 0x10ffff };

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
utf8iterate(const uint8_t *s, size_t slen, int32_t *code)
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

size_t
utf8deiterate(const uint8_t *s, size_t slen, size_t off,
              int32_t *code)
{
  size_t l, ll;

  for (l = 1; l <= off; l++) {
    if ((s[off - l] & 0xc0) != 0x80) {
      ll = utf8iterate(s + off - l, l, code);

      if (l == ll) {
        return ll;

      } else {
        /* We were probably given an offput that was not a codepoint
           boundary. */
        return 0;
      }
    }
  }

  return 0;
}

size_t
utf8codepoints(const uint8_t *s, size_t len)
{
  size_t i, j, a;
  int32_t code;
  i = 0;
  j = 0;

  while (i < len) {
    a = utf8iterate(s + i, len - i, &code);

    if (a == 0) {
      break;

    } else {
      i += a;
      j++;
    }
  }

  return j;
}

uint8_t
utf8encodebyte(int32_t c, size_t i)
{
  return utfbyte[i] | (c & ~utfmask[i]);
}

size_t
utf8encode(uint8_t *s, size_t len, int32_t code)
{
  size_t i, l;

  if (code > UTF_MAX) {
    return 0;
  }

  for (l = 1; l < 5 && utfmax[l] < code; l++)
    ;

  if (l == 5) {
    return 0;
  }

  for (i = l - 1; i != 0; i--) {
    s[i] = utf8encodebyte(code, 0);
    code >>= 6;
  }

  s[0] = utf8encodebyte(code, l);
  return l;
}