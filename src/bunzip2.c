/* bunzip2 - unpack bzip2 files (bzip2 -d).
 *
 *   bunzip2 [-cfkqrtv] [-S .suf] [file | pattern | - ...]
 *
 * Each file.bz2 (or .bz) is replaced by file, file.tbz2 and file.tbz by
 * file.tar, its date, protection bits and file note kept. With no file
 * (or -) the input goes unpacked to the output.
 *   -c  to the output, the files kept      -k  keep the .bz2 files
 *   -f  replace files that are there; with -c, pass on what is not bzip2
 *   -t  test: unpack, check, write nothing -r  into drawers
 *   -S .suf  also take this suffix         -s  taken (small mode is used
 *       by itself when memory is short)
 *   -v  say each file's ratio              -q  no warnings
 * Streams joined one after another are read whole, every block's CRC
 * checked. A -9 file needs about 3.7 MB (small mode 2.3 MB). No packer.
 *
 * Return code 0, 5 a file skipped (there already, no .bz2 suffix, could
 * not be read, trailing garbage), 10 bad arguments, not bzip2, damaged
 * data or a failed write (the partial file deleted), 20 Ctrl-C. */
#include "cu.h"
#include "czc.h"

const char *cu_name = "bunzip2";
static const char version[] = "$VER: bunzip2 0.1b1 (14.10.2026)";

CZ_READERS(&cz_bzip2);

static const CzcCmd cmd = { CZ_BZIP2, 0, 0, 9,
    "bunzip2 [-cfkqrtv] [-S .suf] [file | pattern | - ...]", NULL };

int main(void)
{
    (void)version;
    return czc_main(&cmd);
}
