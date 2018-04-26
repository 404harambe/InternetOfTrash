#include "crc8.h"

/* Taken from: https://chromium.googlesource.com/chromiumos/platform/vboot_reference/+/stabilize2/firmware/lib/crc8.c */

/* Return CRC-8 of the data, using x^8 + x^2 + x + 1 polynomial.  A
 * table-based algorithm would be faster, but for only a few bytes it isn't
 * worth the code size. */
uint8_t crc8(const void* vptr, int len) {
  const uint8_t *data = (const uint8_t*) vptr;
  unsigned crc = 0;
  int i, j;
  for (j = len; j; j--, data++) {
    crc ^= (*data << 8);
    for(i = 8; i; i--) {
      if (crc & 0x8000)
        crc ^= (0x1070 << 3);
      crc <<= 1;
    }
  }
  return (uint8_t)(crc >> 8);
}
