/* tee - the input to the output and to files.
 *
 *   tee [-a] [file ...]
 *
 * What comes in goes out unchanged, and into each file as well:
 * `List | tee RAM:list | grep foo`.
 *   -a add to the end of the files instead of replacing them
 *
 * Return code 0, 10 when a file could not be written (the rest go on),
 * 20 Ctrl-C. */
#include "cu.h"

const char *cu_name = "tee";
static const char version[] = "$VER: tee 0.1b2 (14.10.2026)";

#define MAXF 32

int main(void)
{
    BPTR fh[MAXF], in;
    char *fname[MAXF];
    char *buf, *v;
    int c, i, n = 0, append = 0;
    LONG k;
    (void)version;
    if (cu_args("tee [-a] [file ...]") < 0) return cu_rc;
    while ((c = cu_opt("ai", &v)) >= 0) switch (c) {
        case 'a': append = 1; break;
        case 'i': break;                     /* GNU's: ignore Ctrl-C; taken, not done */
        default: return cu_rc;
    }
    if (cu_argc > MAXF) { cu_err(10, NULL, "too many files"); return cu_rc; }
    for (i = 0; i < cu_argc; i++) {
        BPTR f = Open(cu_argv[i], append ? MODE_READWRITE : MODE_NEWFILE);
        if (f && append) Seek(f, 0, OFFSET_END);
        if (!f) cu_fault(10, cu_argv[i]);
        else { fname[n] = cu_argv[i]; fh[n++] = f; }
    }
    if (!(buf = cu_alloc(8192))) { cu_err(20, NULL, "out of memory"); return cu_rc; }
    in = Input();
    cu_out = Output();
    while ((k = Read(in, buf, 8192)) > 0) {
        Write(cu_out, buf, k);
        for (i = 0; i < n; i++)
            if (fh[i] && Write(fh[i], buf, k) != k) { cu_fault(10, fname[i]); Close(fh[i]); fh[i] = 0; }
        if (cu_break()) break;
    }
    for (i = 0; i < n; i++) if (fh[i]) Close(fh[i]);
    return cu_rc;
}
