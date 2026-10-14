/* cz_all.c - every kind cz_open() can read: linked only by a program that
 * names none itself (CZ_READERS in cz.h), e.g. tar. */
#include "cz_int.h"

CZ_READERS(&cz_gzip, &cz_bzip2, &cz_xz, &cz_zstd);
