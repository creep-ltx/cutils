/* touch - make a file's date now, or make an empty file.
 *
 *   touch [-c] [-d date | -r file] file | pattern ...
 *
 * A file or drawer that exists gets the date now (SetFileDate); a name
 * that does not exist becomes an empty file.
 *   -c       never make a file, only date the ones there
 *   -d DATE  that date instead of now: 2026-10-14, "2026-10-14 13:45",
 *            2026-10-14T13:45:10, or the Amiga's own "14-Oct-26 13:45:10",
 *            Today, Yesterday, a weekday name
 *   -r FILE  the date FILE has
 *
 * Return code 0, 5 when a pattern matched nothing (said), 10 when a date
 * could not be set or a file made, or a bad -d / -r, 20 Ctrl-C. */
#include <dos/datetime.h>
#include "cu.h"

const char *cu_name = "touch";
static const char version[] = "$VER: touch 0.1b1 (14.10.2026)";

static int o_nocreate;
static struct DateStamp when;

static int num(const char **p, int digits, LONG *v)
{
    int k = 0;
    *v = 0;
    while (**p >= '0' && **p <= '9' && k < digits) *v = *v * 10 + (*(*p)++ - '0'), k++;
    return k > 0;
}

/* days since 1.1.1978 for a date of the proleptic Gregorian calendar */
static LONG days(LONG y, LONG m, LONG d)
{
    LONG era, yoe, doy;
    y -= m <= 2;
    era = y / 400;
    yoe = y - era * 400;
    doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468 - 2922;
}

static int mdays(LONG y, LONG m)
{
    static const char n[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return m == 2 && ((y % 4 == 0 && y % 100) || y % 400 == 0) ? 29 : n[m - 1];
}

/* HH:MM[:SS] into minutes and ticks; 0 when it is not one */
static int hhmm(const char *p, struct DateStamp *ds)
{
    LONG h, mi, s = 0;
    if (!num(&p, 2, &h) || *p++ != ':' || !num(&p, 2, &mi)) return 0;
    if (*p == ':') { p++; if (!num(&p, 2, &s)) return 0; }
    if (*p || h > 23 || mi > 59 || s > 59) return 0;
    ds->ds_Minute = h * 60 + mi;
    ds->ds_Tick = s * TICKS_PER_SECOND;
    return 1;
}

static int parsedate(const char *s, struct DateStamp *ds)
{
    const char *p = s;
    LONG y, m, d;
    char date[40], *time = NULL;
    struct DateTime dt;
    if (num(&p, 4, &y) && p - s == 4 && *p == '-') {        /* ISO */
        p++;
        if (!num(&p, 2, &m) || *p++ != '-' || !num(&p, 2, &d)) return 0;
        if (y < 1978 || m < 1 || m > 12 || d < 1 || d > mdays(y, m)) return 0;
        ds->ds_Days = days(y, m, d);
        ds->ds_Minute = ds->ds_Tick = 0;
        if (!*p) return 1;
        if (*p != ' ' && *p != 'T') return 0;
        return hhmm(p + 1, ds);
    }
    if (strlen(s) >= sizeof date) return 0;                 /* the Amiga's own */
    strcpy(date, s);
    if ((time = strchr(date, ' '))) *time++ = 0;
    memset(&dt, 0, sizeof dt);
    dt.dat_Format = FORMAT_DOS;
    dt.dat_StrDate = date;
    if (!StrToDate(&dt)) return 0;
    *ds = dt.dat_Stamp;
    ds->ds_Minute = ds->ds_Tick = 0;
    return !time || hhmm(time, ds);
}

static int one(const char *path, void *u)
{
    BPTR l;
    (void)u;
    if ((l = Lock((STRPTR)path, SHARED_LOCK))) {
        UnLock(l);
        if (!SetFileDate((STRPTR)path, &when)) cu_fault(10, path);
    } else if (IoErr() != ERROR_OBJECT_NOT_FOUND) cu_fault(10, path);
    else if (!o_nocreate) {
        BPTR fh = Open((STRPTR)path, MODE_NEWFILE);
        if (!fh) cu_fault(10, path);
        else {
            Close(fh);
            SetFileDate((STRPTR)path, &when);
        }
    }
    return cu_break();
}

int main(void)
{
    char *v, *d = NULL, *r = NULL;
    int c, i;
    (void)version;
    if (cu_args("touch [-c] [-d date | -r file] file | pattern ...") < 0) return cu_rc;
    while ((c = cu_opt("cd:r:", &v)) >= 0) switch (c) {
        case 'c': o_nocreate = 1; break;
        case 'd': d = v; r = NULL; break;
        case 'r': r = v; d = NULL; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no file named (touch ? for usage)"); return cu_rc; }
    DateStamp(&when);
    if (d && !parsedate(d, &when)) { cu_err(10, d, "not a date"); return cu_rc; }
    if (r) {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        BPTR l = Lock(r, SHARED_LOCK);
        if (!fib || !l || !Examine(l, fib)) {
            cu_fault(10, r);
            if (l) UnLock(l);
            if (fib) FreeDosObject(DOS_FIB, fib);
            return cu_rc;
        }
        when = fib->fib_Date;
        UnLock(l);
        FreeDosObject(DOS_FIB, fib);
    }
    for (i = 0; i < cu_argc && cu_rc < 20; i++) cu_each(cu_argv[i], one, NULL);
    cu_flush();
    return cu_rc;
}
