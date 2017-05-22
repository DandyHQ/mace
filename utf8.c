#include <utf8proc.h>

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
    a = utf8proc_iterate(s + *l, max - *l, &code);
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
