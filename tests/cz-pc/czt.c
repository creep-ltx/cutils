/* czt.c - the squeezers (lib/cz*.c) driven by hand, the same source
 * built for Linux (with fake/ and shim.c) and for the 68020 (with
 * build/libcu.a, run under vamos), so run.sh can compare the two:
 *
 *   czt d IN OUT [N]           unpack IN (any kind) to OUT, reading N bytes a time
 *   czt z KIND LEVEL IN OUT    pack IN as gzip (1) or bzip2 (2)
 *
 * It says one line: "KIND ok BYTES" or "KIND error: WHY". Not part of
 * CUtils' commands. */
#include <stdlib.h>
#include "cu.h"
#include "cz.h"

const char *cu_name = "czt";

static void say(const char *s) { Write(Output(), (APTR)s, strlen(s)); }

static void sayn(unsigned long v)
{
    char b[16];
    int i = 15;
    b[i] = 0;
    do { b[--i] = '0' + v % 10; v /= 10; } while (v);
    say(b + i);
}

int main(int argc, char **argv)
{
    static UBYTE buf[65536];
    BPTR in, out;
    unsigned long total = 0;
    int bad = 0;
    if (argc < 4) { say("usage: czt d IN OUT [N] | czt z KIND LEVEL IN OUT\n"); return 10; }
    if (argv[1][0] == 'd') {
        CzIn *z;
        int found;
        LONG n, step = argc > 4 ? atol(argv[4]) : (LONG)sizeof buf;
        if (step < 1 || step > (LONG)sizeof buf) step = sizeof buf;
        if (!(in = Open((STRPTR)argv[2], MODE_OLDFILE))) { say("cannot open\n"); return 10; }
        if (!(out = Open((STRPTR)argv[3], MODE_NEWFILE))) { say("cannot create\n"); return 10; }
        z = cz_open(in, CZ_AUTO, &found);
        say(cz_kind_name(found));
        if (!z) { say(" error: "); say(cz_error()); say("\n"); Close(in); Close(out); return 10; }
        while ((n = cz_read(z, buf, step)) > 0) {
            Write(out, buf, n);
            total += n;
        }
        if (n < 0) { say(" error: "); say(cz_error()); bad = 1; }
        else {
            say(" ok "); sayn(total);
            if (cz_trailing(z)) say(" trailing");
        }
        say("\n");
        cz_close(z);
    } else {
        CzOut *z;
        LONG n;
        int kind = atoi(argv[2]);
        if (argc < 6) { say("usage\n"); return 10; }
        if (!(in = Open((STRPTR)argv[4], MODE_OLDFILE))) { say("cannot open\n"); return 10; }
        if (!(out = Open((STRPTR)argv[5], MODE_NEWFILE))) { say("cannot create\n"); return 10; }
        if (!(z = cz_create(out, kind, atoi(argv[3])))) { say("create error: "); say(cz_error()); say("\n"); return 10; }
        while ((n = Read(in, buf, sizeof buf)) > 0) {
            if (cz_write(z, buf, n) < 0) { bad = 1; break; }
            total += n;
        }
        if (cz_finish(z) < 0) bad = 1;
        say(cz_kind_name(kind));
        if (bad) { say(" error: "); say(cz_error()); }
        else { say(" ok "); sayn(total); }
        say("\n");
    }
    Close(in);
    Close(out);
    return bad ? 10 : 0;
}
