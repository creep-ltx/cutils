/* unxz - unpack xz files (xz -d).
 *
 *   unxz [-cfklqrtv] [-S .suf] [file | pattern | - ...]
 *
 * Each file.xz is replaced by file (file.txz by file.tar), its date,
 * protection bits and file note kept. With no file (or -) the input goes
 * unpacked to the output.
 *   -c  to the output, the files kept      -k  keep the .xz files
 *   -f  replace files that are there; with -c, pass on what is not xz
 *   -t  test: unpack, check, write nothing -l  list: packed, plain size
 *   -r  into drawers                       -S .suf  also take this suffix
 *   -v  say each file's ratio              -q  no warnings
 * Streams joined one after another (and the zero padding between them)
 * are read whole. LZMA2 with the BCJ filters (x86, ARM, ARM-Thumb, ARM64,
 * RISC-V, PowerPC, IA-64, SPARC); the checks CRC32, CRC64 and SHA-256 are
 * verified. The memory is the packer's window: 8 MB for xz -6 (the
 * default), 64 MB for xz -9. No packer.
 *
 * Return code 0, 5 a file skipped (there already, no .xz suffix, could not
 * be read), 10 bad arguments, not xz, damaged data, not enough memory or a
 * failed write (the partial file deleted), 20 Ctrl-C. */
#include "cu.h"
#include "czc.h"

const char *cu_name = "unxz";
static const char version[] = "$VER: unxz 0.1b1 (14.10.2026)";

CZ_READERS(&cz_xz);

static const CzcCmd cmd = { CZ_XZ, 0, 0, 6,
    "unxz [-cfklqrtv] [-S .suf] [file | pattern | - ...]", NULL };

int main(void)
{
    (void)version;
    return czc_main(&cmd);
}
