/* date - show or set the date and time. Takes the place of the Amiga's
 * C:Date, so both forms work:
 *
 *   date [DAY] [DATE] [TIME] [TO|VER file]        (DAY,DATE,TIME,TO=VER/K)
 *   date [-u] [-r file | -d date] [+FORMAT]       (Unix)
 *
 * With nothing: the date as the Amiga's Date prints it (DateToStr, so in
 * the locale's words): Wednesday 14-Oct-26 18:36:41
 *
 * Setting the clock, as the Amiga's Date (the system clock, through
 * timer.device; the battery clock only with SetClock SAVE afterwards):
 *   DATE  14-Oct-26, 14-10-26, 14-Oct-2026 or 2026-10-14: that day, the
 *         time of day kept (two-digit years: 78-99 are 19xx, else 20xx)
 *   TIME  18:30 or 18:30:15 (H:M works too): that time, the day kept
 *   DAY   Tomorrow, Yesterday, Today, or a weekday: the next one (on a
 *         Wednesday, Wednesday is a week ahead)
 * The words may come in any order (date 12:00 1-jan-30); setting prints
 * nothing. TO file (or VER file, TO=file): the date into the file instead
 * (after setting, the new one).
 *
 * Unix:
 *   +FORMAT  strftime's: %Y %m %d %H %M %S %y %b %B %a %A %j %e %p %Z
 *            (empty: the Amiga keeps no zone) %s (seconds since 1970, the
 *            clock taken as UTC) %F %T %D %R %h %I %k %l %u %w %C %n %t %%;
 *            names in English. Quote a FORMAT with spaces in it
 *   -r FILE  the file's date instead of now    -d DATE  that date (as
 *            touch -d: 2026-10-14 13:45, 14-Oct-26, Yesterday, 13:45)
 *   -u       taken and ignored (no zones)
 * Without +FORMAT the Amiga's form above.
 *
 * Return code 0, 10 a bad date or argument (the Amiga's says 20), a clock
 * that could not be set, or a file that could not be read or written,
 * 20 Ctrl-C. */
#include <dos/datetime.h>
#include <devices/timer.h>
#include "cu.h"

const char *cu_name = "date";
static const char version[] = "$VER: date 0.1b2 (16.10.2026)";

static BPTR out;
static char obuf[512];
static int olen, werr;

static void put(const char *s, int n)
{
    while (n-- > 0) {
        if (olen == (int)sizeof obuf) { if (Write(out, obuf, olen) != olen) werr = 1; olen = 0; }
        obuf[olen++] = *s++;
    }
}
static void puts_(const char *s) { put(s, strlen(s)); }
static void num(LONG v, int width, int pad)
{
    char t[12];
    int i = 0;
    ULONG u = v < 0 ? -v : v;
    do t[i++] = '0' + u % 10, u /= 10; while (u);
    if (v < 0) t[i++] = '-';
    while (i < width--) put(pad == ' ' ? " " : "0", 1);
    while (i) put(&t[--i], 1);
}

/* the Amiga's own: "Wednesday 14-Oct-26 18:36:41" */
static void amigadate(const struct DateStamp *ds)
{
    char day[32], date[32], time[32];
    struct DateTime dt;
    memset(&dt, 0, sizeof dt);
    dt.dat_Stamp = *ds;
    dt.dat_Format = FORMAT_DOS;
    dt.dat_StrDay = day; dt.dat_StrDate = date; dt.dat_StrTime = time;
    day[0] = date[0] = time[0] = 0;
    DateToStr(&dt);
    puts_(day); put(" ", 1); puts_(date); put(" ", 1); puts_(time); put("\n", 1);
}

static void format(const char *f, const struct DateStamp *dsin)
{
    struct DateStamp fixed = *dsin, *ds = &fixed;
    LONG y, m, d, h, mi, s, wd, yd;
    cu_dsfix(ds);
    h = ds->ds_Minute / 60; mi = ds->ds_Minute % 60; s = ds->ds_Tick / TICKS_PER_SECOND; wd = ds->ds_Days % 7;
    cu_civil(ds->ds_Days, &y, &m, &d);
    yd = ds->ds_Days - cu_days(y, 1, 1) + 1;
    for (; *f; f++) {
        if (*f != '%' || !f[1]) { put(f, 1); continue; }
        switch (*++f) {
            case 'Y': num(y, 4, '0'); break;
            case 'C': num(y / 100, 2, '0'); break;
            case 'y': num(y % 100, 2, '0'); break;
            case 'm': num(m, 2, '0'); break;
            case 'd': num(d, 2, '0'); break;
            case 'e': num(d, 2, ' '); break;
            case 'H': num(h, 2, '0'); break;
            case 'k': num(h, 2, ' '); break;
            case 'I': num(h % 12 ? h % 12 : 12, 2, '0'); break;
            case 'l': num(h % 12 ? h % 12 : 12, 2, ' '); break;
            case 'M': num(mi, 2, '0'); break;
            case 'S': num(s, 2, '0'); break;
            case 'j': num(yd, 3, '0'); break;
            case 'u': num(wd ? wd : 7, 1, '0'); break;
            case 'w': num(wd, 1, '0'); break;
            case 'b': case 'h': put(cu_months[m - 1], 3); break;
            case 'B': puts_(cu_months[m - 1]); break;
            case 'a': put(cu_wdays[wd], 3); break;
            case 'A': puts_(cu_wdays[wd]); break;
            case 'p': puts_(h < 12 ? "AM" : "PM"); break;
            case 'Z': break;
            case 's': {
                char t[12];
                int i = 0;
                ULONG u = cu_unixtime(ds);
                do t[i++] = '0' + u % 10, u /= 10; while (u);
                while (i) put(&t[--i], 1);
                break;
            }
            case 'F': num(y, 4, '0'); put("-", 1); num(m, 2, '0'); put("-", 1); num(d, 2, '0'); break;
            case 'T': num(h, 2, '0'); put(":", 1); num(mi, 2, '0'); put(":", 1); num(s, 2, '0'); break;
            case 'R': num(h, 2, '0'); put(":", 1); num(mi, 2, '0'); break;
            case 'D': num(m, 2, '0'); put("/", 1); num(d, 2, '0'); put("/", 1); num(y % 100, 2, '0'); break;
            case 'n': put("\n", 1); break;
            case 't': put("\t", 1); break;
            case '%': put("%", 1); break;
            default: put(f - 1, 2); break;
        }
    }
    put("\n", 1);
}

static int flushout(void)
{
    if (olen && Write(out, obuf, olen) != olen) werr = 1;
    olen = 0;
    return !werr;
}

/* ---- setting ---- */

static int ieq(const char *a, const char *b)
{
    for (;; a++, b++) {
        int x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return 0;
        if (!x) return 1;
    }
}

static int digits(const char **p, int max, LONG *v)
{
    int k = 0;
    *v = 0;
    while (**p >= '0' && **p <= '9' && k < max) *v = *v * 10 + (*(*p)++ - '0'), k++;
    return k;
}

/* D-M-Y (M a number or an English month, Y two or four digits) or Y-M-D */
static int parseday(const char *s, LONG *days)
{
    const char *p = s;
    LONG a, m = 0, y, d;
    int k = digits(&p, 4, &a);
    if (!k || *p++ != '-') return 0;
    if (k == 4) {                                         /* 2026-10-14 */
        y = a;
        if (!digits(&p, 2, &m) || *p++ != '-' || !digits(&p, 2, &d) || *p) return 0;
    } else {
        d = a;
        if (*p >= '0' && *p <= '9') { if (!digits(&p, 2, &m)) return 0; }
        else {
            char w[12];
            int i = 0, j;
            while (*p && *p != '-' && i < 11) { int c = *p++; w[i++] = c >= 'A' && c <= 'Z' ? c + 32 : c; }
            w[i] = 0;
            if (i < 3) return 0;
            for (j = 0; j < 12 && !m; j++) {
                const char *n = cu_months[j];
                int q;
                for (q = 0; q < i && (n[q] | 0x20) == w[q]; q++) ;
                if (q == i) m = j + 1;
            }
            if (!m) return 0;
        }
        if (*p++ != '-') return 0;
        k = digits(&p, 4, &y);
        if (*p || (k != 2 && k != 4)) return 0;
        if (k == 2) y += y < 78 ? 2000 : 1900;
    }
    if (y < 1978 || y > 2100 || m < 1 || m > 12 || d < 1 || d > cu_mdays(y, m)) return 0;
    *days = cu_days(y, m, d);
    return 1;
}

static int setclock(const struct DateStamp *ds)
{
    struct MsgPort *port = CreateMsgPort();
    struct TimeRequest *tr = port ? CreateIORequest(port, sizeof *tr) : NULL;
    int ok = 0;
    if (tr && !OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0)) {
        tr->tr_node.io_Command = TR_SETSYSTIME;
        tr->tr_time.tv_secs = (ULONG)ds->ds_Days * 86400 + (ULONG)ds->ds_Minute * 60 + ds->ds_Tick / TICKS_PER_SECOND;
        tr->tr_time.tv_micro = ds->ds_Tick % TICKS_PER_SECOND * (1000000 / TICKS_PER_SECOND);
        ok = !DoIO((struct IORequest *)tr);
        CloseDevice((struct IORequest *)tr);
    }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (port) DeleteMsgPort(port);
    return ok;
}

static void bad(void)
{
    cu_err(10, NULL, "bad number");
    if (!cu_silent) {
        struct Process *me = (struct Process *)FindTask(NULL);
        BPTR e = me->pr_CES ? me->pr_CES : Output();
        static const char hint[] = "- use DD-MMM-YY or YYYY-MM-DD etc. to set date\n"
                                   "       HH:MM:SS etc. to set time\n";
        Write(e, (APTR)hint, sizeof hint - 1);
    }
}

static int amigaform(void)
{
    static const char *const rel[] = { "today", "tomorrow", "yesterday" };
    struct DateStamp now, ds;
    char *to = NULL;
    int i, j, set = 0, pos = 0;
    DateStamp(&now);
    cu_dsfix(&now);
    ds = now;
    for (i = 0; i < cu_argc; i++) {
        char *a = cu_argv[i];
        if (ieq(a, "TO") || ieq(a, "VER")) {
            if (++i >= cu_argc) { cu_err(10, a, "needs a file name"); return cu_rc; }
            to = cu_argv[i];
            continue;
        }
        if ((a[0] | 0x20) == 't' && (a[1] | 0x20) == 'o' && a[2] == '=') { to = a + 3; continue; }
        if ((a[0] | 0x20) == 'v' && (a[1] | 0x20) == 'e' && (a[2] | 0x20) == 'r' && a[3] == '=') { to = a + 4; continue; }
        if (++pos > 3) { cu_err(10, a, "too many arguments (date ? for usage)"); return cu_rc; }
        set = 1;
        if (strchr(a, ':')) { if (!cu_parsetime(a, &ds)) { bad(); return cu_rc; } continue; }
        for (j = 0; j < 3 && !ieq(a, rel[j]); j++) ;
        if (j < 3) { ds.ds_Days += j == 1 ? 1 : j == 2 ? -1 : 0; continue; }
        for (j = 0; j < 7 && !ieq(a, cu_wdays[j]); j++) ;
        if (j < 7) {                                       /* the next one: 1 to 7 days ahead */
            LONG ahead = (j - ds.ds_Days % 7 + 7) % 7;
            ds.ds_Days += ahead ? ahead : 7;
            continue;
        }
        if (!parseday(a, &ds.ds_Days)) { bad(); return cu_rc; }
    }
    if (set) {
        if (!setclock(&ds)) { cu_err(10, NULL, "the clock could not be set (timer.device)"); return cu_rc; }
        if (!to) return cu_rc;
        DateStamp(&ds);
    }
    if (to && !(out = Open(to, MODE_NEWFILE))) { cu_fault(10, to); return cu_rc; }
    amigadate(&ds);
    if (!flushout()) cu_fault(10, to ? to : NULL);
    if (to) Close(out);
    return cu_rc;
}

int main(void)
{
    struct DateStamp ds;
    char *v, *fmt = NULL, *d = NULL, *r = NULL;
    int c;
    (void)version;
    if (cu_args("date [DAY] [DATE] [TIME] [TO|VER file]  |  date [-u] [-r file | -d date] [+FORMAT]") < 0) return cu_rc;
    out = Output();
    if (!cu_argc || (cu_argv[0][0] != '-' && cu_argv[0][0] != '+')) return amigaform();
    while ((c = cu_opt("ur:d:", &v)) >= 0) switch (c) {
        case 'u': break;
        case 'r': r = v; d = NULL; break;
        case 'd': d = v; r = NULL; break;
        default: return cu_rc;
    }
    if (cu_argc && cu_argv[0][0] == '+') fmt = cu_argv[0] + 1, cu_argc--, cu_argv++;
    if (cu_argc) { cu_err(10, cu_argv[0], "not a +FORMAT (date ? for usage)"); return cu_rc; }
    DateStamp(&ds);
    if (d && !cu_parsedate(d, &ds)) { cu_err(10, d, "not a date"); return cu_rc; }
    if (r) {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        BPTR l = Lock(r, SHARED_LOCK);
        if (!fib || !l || !Examine(l, fib)) cu_fault(10, r);
        else ds = fib->fib_Date;
        if (l) UnLock(l);
        if (fib) FreeDosObject(DOS_FIB, fib);
        if (cu_rc) return cu_rc;
    }
    if (fmt) format(fmt, &ds); else amigadate(&ds);
    flushout();
    return cu_rc;
}
