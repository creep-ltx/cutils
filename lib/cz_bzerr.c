/* cz_bzerr.c - what libbzip2 calls (built with BZ_NO_STDIO) when one of
 * its own checks fails: a bug in the library, never bad input. Said, and
 * the program ends (libnix gives back its memory and files). */
#include <stdlib.h>
#include "cz_int.h"

void bz_internal_error(int errcode)
{
    char t[24] = "internal error ";
    int i = 15, k;
    char d[8];
    for (k = 0; k < 7 && (errcode || !k); k++, errcode /= 10) d[k] = '0' + errcode % 10;
    while (k) t[i++] = d[--k];
    t[i] = 0;
    cu_err(20, "bzip2", t);
    exit(20);
}
