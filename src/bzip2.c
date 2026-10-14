/* bzip2 - pack files with bzip2, or unpack them (-d).
 *
 *   bzip2 [-cdfkqrtv1..9] [-S .suf] [file | pattern | - ...]
 *
 * Each file is replaced by file.bz2 (its date, protection bits and file
 * note kept); with no file (or -) the input goes packed to the output.
 *   -c  to the output, the files kept      -d  unpack (bzip2 -d is bunzip2)
 *   -k  keep the input files               -f  replace files that are there,
 *       pack a .bz2 again, write packed data to a window
 *   -1 .. -9  the block, 100 .. 900 KB (9; packing needs 1.2 .. 7.6 MB)
 *   -t  test the packed files              -r  into drawers
 *   -S .suf  another suffix than .bz2      -s  taken (bzip2's small mode:
 *       used by itself when memory is short)
 *   -v  say each file's ratio              -q  no warnings
 * Long forms: --stdout --decompress --keep --force --test --recursive
 * --small --verbose --quiet --fast --best. Unpacking takes .bz2 .bz .tbz2
 * .tbz (the last two to .tar), streams joined one after another.
 *
 * Return code 0, 5 a file skipped (there already, a wrong suffix, could not
 * be read, trailing garbage), 10 bad arguments, damaged data or a failed
 * write (the partial file deleted), 20 Ctrl-C. */
#include "cu.h"
#include "czc.h"

const char *cu_name = "bzip2";
static const char version[] = "$VER: bzip2 0.1b1 (14.10.2026)";

CZ_READERS(&cz_bzip2);
CZ_PACKERS(&cz_bzip2_pack);

static const CzcCmd cmd = { CZ_BZIP2, 1, 0, 9,
    "bzip2 [-cdfkqrtv1..9] [-S .suf] [file | pattern | - ...]", czc_packfile };

int main(void)
{
    (void)version;
    return czc_main(&cmd);
}
