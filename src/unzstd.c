/* unzstd - unpack zstd files (zstd -d).
 *
 *   unzstd [-cfklqrtv] [--rm] [-S .suf] [file | pattern | - ...]
 *
 * Each file.zst gives file (file.tzst gives file.tar), its date,
 * protection bits and file note kept. As zstd: the .zst file is kept,
 * unless --rm. With no file (or -) the input goes unpacked to the output.
 *   -c  to the output                      -k  keep the .zst files (so it is)
 *   --rm  delete each .zst file once unpacked
 *   -f  replace files that are there; with -c, pass on what is not zstd
 *   -t  test: unpack, check, write nothing -l  list: packed, plain size
 *   -r  into drawers                       -S .suf  also take this suffix
 *   -v  say each file's ratio              -q  no warnings
 * Frames one after another are read whole, skippable frames passed over,
 * each frame's checksum checked when it has one. The memory is the
 * packer's window: the file's size for small files, 8 MB for most; at most
 * 128 MB (--long files beyond it are refused). No legacy (v0.7 and older)
 * frames. No packer.
 *
 * Return code 0, 5 a file skipped (there already, no .zst suffix, could not
 * be read), 10 bad arguments, not zstd, damaged data, not enough memory or
 * a failed write (the partial file deleted), 20 Ctrl-C. */
#include "cu.h"
#include "czc.h"

const char *cu_name = "unzstd";
static const char version[] = "$VER: unzstd 0.1b1 (14.10.2026)";

CZ_READERS(&cz_zstd);

static const CzcCmd cmd = { CZ_ZSTD, 0, 1, 3,
    "unzstd [-cfklqrtv] [--rm] [-S .suf] [file | pattern | - ...]", NULL };

int main(void)
{
    (void)version;
    return czc_main(&cmd);
}
