/* gzip - pack files with gzip (deflate), or unpack them (-d).
 *
 *   gzip [-cdfklnqrtv1..9] [-S .suf] [file | pattern | - ...]
 *
 * Each file is replaced by file.gz (its date, protection bits and file
 * note kept); with no file (or -) the input goes packed to the output.
 *   -c  to the output, the files kept      -d  unpack (gzip -d is gunzip)
 *   -k  keep the input files               -f  replace files that are there,
 *       pack a .gz again, write packed data to a window
 *   -1 .. -9  fast .. small (6)            -n  no name and date in the header
 *   -t  test the packed files              -l  list: packed, plain size, ratio
 *   -r  into drawers                       -S .suf  another suffix than .gz
 *   -v  say each file's ratio              -q  no warnings
 * Long forms as GNU's: --stdout --decompress --keep --force --test --list
 * --recursive --no-name --verbose --quiet --fast --best.
 * Reading takes multi-member and joined files, checks every CRC and
 * length. Unpacking takes .gz and .tgz (to .tar); gzip -d foo finds foo.gz.
 *
 * Return code 0, 5 a file skipped (there already, a wrong suffix, could not
 * be read, trailing garbage), 10 bad arguments, damaged data or a failed
 * write (the partial file deleted), 20 Ctrl-C. */
#include "cu.h"
#include "czc.h"

const char *cu_name = "gzip";
static const char version[] = "$VER: gzip 0.1b1 (14.10.2026)";

CZ_READERS(&cz_gzip);
CZ_PACKERS(&cz_gzip_pack);

static const CzcCmd cmd = { CZ_GZIP, 1, 0, 6,
    "gzip [-cdfklnqrtv1..9] [-S .suf] [file | pattern | - ...]", czc_packfile };

int main(void)
{
    (void)version;
    return czc_main(&cmd);
}
