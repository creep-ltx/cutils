/* cz_crc.c - CRC-32 as gzip (and zlib's crc32()) has it: polynomial
 * 0xedb88320, the bits low first, inverted before and after. */
#include "cz_int.h"

void cz_crc_init(ULONG *t)
{
    ULONG c;
    int i, k;
    for (i = 0; i < 256; i++) {
        c = i;
        for (k = 0; k < 8; k++) c = c & 1 ? 0xedb88320UL ^ (c >> 1) : c >> 1;
        t[i] = c;
    }
}

ULONG cz_crc32(const ULONG *t, ULONG crc, const UBYTE *p, LONG n)
{
    crc = ~crc;
    while (n >= 4) {
        crc = t[(crc ^ p[0]) & 0xff] ^ (crc >> 8);
        crc = t[(crc ^ p[1]) & 0xff] ^ (crc >> 8);
        crc = t[(crc ^ p[2]) & 0xff] ^ (crc >> 8);
        crc = t[(crc ^ p[3]) & 0xff] ^ (crc >> 8);
        p += 4; n -= 4;
    }
    while (n-- > 0) crc = t[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    return ~crc;
}
