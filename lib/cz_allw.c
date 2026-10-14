/* cz_allw.c - every kind cz_create() can make: linked only by a program
 * that names none itself (CZ_PACKERS in cz.h), e.g. tar. */
#include "cz_int.h"

CZ_PACKERS(&cz_gzip_pack, &cz_bzip2_pack);
