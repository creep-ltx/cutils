/* cu_date.c - dates for touch, date, stat, diff (cu.h): the calendar
 * worked out by hand (days since 1.1.1978 both ways), dates read from
 * text, and the English names. dos.library only. */
#include <dos/datetime.h>
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

/* seconds since 1.1.1970, the Amiga's clock taken as UTC */
ULONG cu_unixtime(const struct DateStamp *ds)
{
    return ((ULONG)ds->ds_Days + 2922) * 86400 + (ULONG)ds->ds_Minute * 60 + ds->ds_Tick / TICKS_PER_SECOND;
}
