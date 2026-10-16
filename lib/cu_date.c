/* cu_date.c - dates for touch, date, stat, diff (cu.h): the calendar
 * worked out by hand (days since 1.1.1978 both ways), dates read from
 * text, the English names, and the time zone (UTC <-> the Amiga's local
 * clock). dos.library, and locale.library for the zone when ENV:TZ is unset. */
#include <dos/datetime.h>
#include <libraries/locale.h>
#include <proto/locale.h>
#include "cu.h"

const char *const cu_months[12] = { "January", "February", "March", "April", "May", "June", "July",
                                    "August", "September", "October", "November", "December" };
const char *const cu_wdays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

/* days since 1.1.1978 for a date of the proleptic Gregorian calendar */
LONG cu_days(LONG y, LONG m, LONG d)
{
    LONG era, yoe, doy;
    y -= m <= 2;
    era = y / 400;
    yoe = y - era * 400;
    doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468 - 2922;
}

/* and back: the year, month (1-12) and day (1-31) of a day number */
void cu_civil(LONG days, LONG *y, LONG *m, LONG *d)
{
    LONG z = days + 719468 + 2922, era, doe, yoe, doy, mp;
    era = z / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    *d = doy - (153 * mp + 2) / 5 + 1;
    *m = mp < 10 ? mp + 3 : mp - 9;
    *y = yoe + era * 400 + (*m <= 2);
}

int cu_mdays(LONG y, LONG m)
{
    static const char n[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return m == 2 && ((y % 4 == 0 && y % 100) || y % 400 == 0) ? 29 : n[m - 1];
}

static int num(const char **p, int digits, LONG *v)
{
    int k = 0;
    *v = 0;
    while (**p >= '0' && **p <= '9' && k < digits) *v = *v * 10 + (*(*p)++ - '0'), k++;
    return k > 0;
}

/* H:MM[:SS] (one or two digits each) into minutes and ticks; 0 when it is not one */
int cu_parsetime(const char *p, struct DateStamp *ds)
{
    LONG h, mi, s = 0;
    if (!num(&p, 2, &h) || *p++ != ':' || !num(&p, 2, &mi)) return 0;
    if (*p == ':') { p++; if (!num(&p, 2, &s)) return 0; }
    if (*p || h > 23 || mi > 59 || s > 59) return 0;
    ds->ds_Minute = h * 60 + mi;
    ds->ds_Tick = s * TICKS_PER_SECOND;
    return 1;
}

int cu_parsedate(const char *s, struct DateStamp *ds)
{
    const char *p = s;
    LONG y, m, d;
    char date[40], *time = NULL;
    struct DateTime dt;
    if (num(&p, 4, &y) && p - s == 4 && *p == '-') {        /* ISO */
        p++;
        if (!num(&p, 2, &m) || *p++ != '-' || !num(&p, 2, &d)) return 0;
        if (y < 1978 || m < 1 || m > 12 || d < 1 || d > cu_mdays(y, m)) return 0;
        ds->ds_Days = cu_days(y, m, d);
        ds->ds_Minute = ds->ds_Tick = 0;
        if (!*p) return 1;
        if (*p != ' ' && *p != 'T') return 0;
        return cu_parsetime(p + 1, ds);
    }
    if (strchr(s, ':') && !strchr(s, ' ')) {                /* a time alone: today */
        DateStamp(ds);
        return cu_parsetime(s, ds);
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
    return !time || cu_parsetime(time, ds);
}

/* After 2046 the clock's seconds no longer fit a LONG and DateStamp()
 * gives negative days (dos divides them signed; DateToStr copes): the
 * stamp made right again from its seconds taken unsigned. */
void cu_dsfix(struct DateStamp *ds)
{
    ULONG s;
    if (ds->ds_Days >= 0 && ds->ds_Minute >= 0 && ds->ds_Tick >= 0) return;
    s = (ULONG)ds->ds_Days * 86400 + (ULONG)ds->ds_Minute * 60 + (ULONG)(ds->ds_Tick / TICKS_PER_SECOND);
    ds->ds_Tick = (ds->ds_Tick >= 0 ? ds->ds_Tick % TICKS_PER_SECOND : 0) + s % 60 * TICKS_PER_SECOND;
    ds->ds_Days = s / 86400;
    ds->ds_Minute = s % 86400 / 60;
}

/* "2026-10-14 18:36:41" (20 bytes with the 0) */
void cu_isodate(const struct DateStamp *dsin, char *buf)
{
    struct DateStamp fixed = *dsin, *ds = &fixed;
    LONG y, m, d, mi, s;
    int i = 0;
    cu_dsfix(ds);
    mi = ds->ds_Minute; s = ds->ds_Tick / TICKS_PER_SECOND;
    cu_civil(ds->ds_Days, &y, &m, &d);
    buf[i++] = '0' + y / 1000 % 10; buf[i++] = '0' + y / 100 % 10;
    buf[i++] = '0' + y / 10 % 10; buf[i++] = '0' + y % 10; buf[i++] = '-';
    buf[i++] = '0' + m / 10; buf[i++] = '0' + m % 10; buf[i++] = '-';
    buf[i++] = '0' + d / 10; buf[i++] = '0' + d % 10; buf[i++] = ' ';
    buf[i++] = '0' + mi / 600; buf[i++] = '0' + mi / 60 % 10; buf[i++] = ':';
    buf[i++] = '0' + mi % 60 / 10; buf[i++] = '0' + mi % 10; buf[i++] = ':';
    buf[i++] = '0' + s / 10; buf[i++] = '0' + s % 10; buf[i] = 0;
}

/* ---- the Amiga's time zone (from CTerm's files.c, 0.3b44) ----
 * The Amiga's clock is local time; Unix seconds (tar, gzip, HTTP) are UTC.
 * The zone: ENV:TZ when it is set, as POSIX writes it ("CET-1CEST,M3.5.0,
 * M10.5.0/3": the standard zone's hours west, the summer zone and the
 * days it starts and ends; "UTC0" none), else Locale's GMT offset
 * (Prefs/Locale's map; no summer time in it). */
static int tz_read, tz_has;
static LONG tz_std, tz_dst;              /* seconds east of UTC */
static UBYTE tz_m[2], tz_w[2], tz_d[2];  /* Mm.w.d: the start, the end */
static LONG tz_t[2];                     /* ... at this local time */

static const char *tz_num(const char *p, LONG *v)
{
    *v = 0;
    if (*p < '0' || *p > '9') return NULL;
    while (*p >= '0' && *p <= '9') *v = *v * 10 + *p++ - '0';
    return p;
}

static const char *tz_time(const char *p, LONG *s)   /* [+-]hh[:mm[:ss]] */
{
    LONG h, m = 0, x = 0, sg = 1;
    if (*p == '+' || *p == '-') sg = *p++ == '-' ? -1 : 1;
    if (!(p = tz_num(p, &h))) return NULL;
    if (*p == ':' && (p = tz_num(p + 1, &m)) && *p == ':') p = tz_num(p + 1, &x);
    if (!p) return NULL;
    *s = sg * (h * 3600 + m * 60 + x);
    return p;
}

static const char *tz_name(const char *p)
{
    if (*p == '<') { while (*p && *p != '>') p++; return *p ? p + 1 : p; }
    while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) p++;
    return p;
}

static const char *tz_rule(const char *p, int i)     /* ",Mm.w.d[/time]" */
{
    LONG v;
    if (*p++ != ',' || *p++ != 'M' || !(p = tz_num(p, &v)) || v < 1 || v > 12) return NULL;
    tz_m[i] = v;
    if (*p++ != '.' || !(p = tz_num(p, &v)) || v < 1 || v > 5) return NULL;
    tz_w[i] = v;
    if (*p++ != '.' || !(p = tz_num(p, &v)) || v > 6) return NULL;
    tz_d[i] = v;
    tz_t[i] = 7200;
    if (*p == '/' && !(p = tz_time(p + 1, &tz_t[i]))) return NULL;
    return p;
}

static void tz_init(void)
{
    char v[80];
    const char *p;
    LONG s;
    tz_read = 1;
    if (GetVar("TZ", v, sizeof v, 0) > 0 && (p = tz_name(v)) != v && (p = tz_time(p, &s))) {
        tz_std = tz_dst = -s;            /* POSIX counts west */
        if (*p) {
            const char *q = tz_name(p);
            if (q == p) return;
            tz_dst = tz_std + 3600;
            if (*q && *q != ',' && (q = tz_time(q, &s))) tz_dst = -s;
            if (q && (q = tz_rule(q, 0)) && tz_rule(q, 1)) tz_has = 1;
        }
        return;
    }
    {
        struct Library *LocaleBase = OpenLibrary("locale.library", 38);
        struct Locale *l;
        if (LocaleBase) {
            if ((l = OpenLocale(NULL))) {
                tz_std = tz_dst = -l->loc_GMTOffset * 60;   /* minutes west */
                CloseLocale(l);
            }
            CloseLibrary(LocaleBase);
        }
    }
}

/* Mm.w.d of year y: its day, in days since 1970 */
static LONG tz_day(LONG y, int i)
{
    LONG first = cu_days(y, tz_m[i], 1) + 2922;
    LONG next = (tz_m[i] == 12 ? cu_days(y + 1, 1, 1) : cu_days(y, tz_m[i] + 1, 1)) + 2922;
    LONG d = first + (tz_d[i] - (first + 4) % 7 + 7) % 7 + (tz_w[i] - 1) * 7;
    while (d >= next) d -= 7;           /* week 5: the last */
    return d;
}

LONG cu_tzeast(ULONG utc)
{
    LONG y, m, d, on, off;
    if (!tz_read) tz_init();
    if (!tz_has) return tz_std;
    cu_civil((LONG)(utc / 86400) - 2922, &y, &m, &d);
    on = tz_day(y, 0) * 86400 + tz_t[0] - tz_std;      /* both in UTC */
    off = tz_day(y, 1) * 86400 + tz_t[1] - tz_dst;
    if (on < off ? (LONG)utc >= on && (LONG)utc < off : (LONG)utc >= on || (LONG)utc < off)
        return tz_dst;
    return tz_std;
}

/* seconds since 1.1.1970 UTC of a stamp of the Amiga's (local) clock */
ULONG cu_unixtime(const struct DateStamp *ds)
{
    ULONG local = ((ULONG)ds->ds_Days + 2922) * 86400 + (ULONG)ds->ds_Minute * 60 + ds->ds_Tick / TICKS_PER_SECOND;
    LONG e = cu_tzeast(local);
    e = cu_tzeast(local - e);           /* right also near a summer time change */
    return local - e;
}

/* and back: the Amiga's (local) stamp of a UTC time; before 1978 is 1.1.1978 */
void cu_fromunix(ULONG utc, struct DateStamp *ds)
{
    LONG e = cu_tzeast(utc);
    ULONG s = e < 0 && utc < (ULONG)-e ? 0 : utc + (ULONG)e;    /* unsigned: right past 2038 */
    ULONG l = s < 2922UL * 86400 ? 0 : s - 2922UL * 86400;
    ds->ds_Days = l / 86400;
    ds->ds_Minute = l % 86400 / 60;
    ds->ds_Tick = l % 60 * TICKS_PER_SECOND;
}
