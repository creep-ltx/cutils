/* gunzip - unpack gzip files (gzip -d).
 *
 *   gunzip [-cfklqrtv] [-S .suf] [file | pattern | - ...]
 *
 * Each file.gz is replaced by file (file.tgz by file.tar), its date,
 * protection bits and file note kept; gunzip foo finds foo.gz. With no
 * file (or -) the input goes unpacked to the output.
 *   -c  to the output, the files kept      -k  keep the .gz files
 *   -f  replace files that are there; with -c, pass on what is not gzip
 *   -t  test: unpack, check, write nothing -l  list: packed, plain size, ratio
 *   -r  into drawers                       -S .suf  also take this suffix
 *   -v  say each file's ratio              -q  no warnings
 * Multi-member and joined .gz files are read whole; every member's CRC
 * and length are checked. Only the kernel of the program: no packer.
 *
 * Return code 0, 5 a file skipped (there already, no .gz suffix, could not
 * be read, trailing garbage), 10 bad arguments, not gzip, damaged data or a
 * failed write (the partial file deleted), 20 Ctrl-C. */
#include "cu.h"
#include "czc.h"

const char *cu_name = "gunzip";
static const char version[] = "$VER: gunzip 0.1b2 (16.10.2026)";

CZ_READERS(&cz_gzip);

static const CzcCmd cmd = { CZ_GZIP, 0, 0, 6,
    "gunzip [-cfklqrtv] [-S .suf] [file | pattern | - ...]", NULL };

int main(void)
{
    (void)version;
    return czc_main(&cmd);
}
