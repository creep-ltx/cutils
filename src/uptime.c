/* uptime - how long since the Amiga was started.
 *
 *   uptime [-p | -s]
 *
 * The time now, how long since the last start (reset or power on), and
 * how many Shells are open:   21:45:03 up 2 days, 1:02, 3 shells
 * The time since the start is the timer's E-clock count, which runs from
 * the moment the system starts.
 *   -p pretty: "up 2 days, 1 hour, 2 minutes" only
 *   -s since: the date and time of the start
 *
 * Return code 0, 10 bad arguments, 20 the timer could not be opened. */
#include <devices/timer.h>
#include <proto/timer.h>
#include "cu.h"

const char *cu_name = "uptime";
static const char version[] = "$VER: uptime 0.1b1 (14.10.2026)";

struct Device *TimerBase;

static void unitword(ULONG n, const char *one, int *first)
{
    if (!*first) cu_puts(", ");
    *first = 0;
    cu_putnum(n, 0);
    cu_putc(' ');
    cu_puts(one);
    if (n != 1) cu_putc('s');
}

static void two(ULONG n) { cu_putc('0' + n / 10 % 10); cu_putc('0' + n % 10); }

int main(void)
{
    struct timerequest tr;
    struct EClockVal ev;
    ULONG freq, secs, days, hours, mins;
    unsigned long long ticks;
    char *v;
    int c, pretty = 0, since = 0, shells = 0, i, n;
    static CuTask t[128];
    struct DateStamp now;
    (void)version;
    if (cu_args("uptime [-p | -s]") < 0) return cu_rc;
    while ((c = cu_opt("ps", &v)) >= 0) switch (c) {
        case 'p': pretty = 1; break;
        case 's': since = 1; break;
        default: return cu_rc;
    }
    memset(&tr, 0, sizeof tr);
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, (struct IORequest *)&tr, 0)) { cu_err(20, NULL, "no timer.device"); return cu_rc; }
    TimerBase = tr.tr_node.io_Device;
    freq = ReadEClock(&ev);
    DateStamp(&now);
    CloseDevice((struct IORequest *)&tr);
    ticks = ((unsigned long long)ev.ev_hi << 32) | ev.ev_lo;
    secs = freq ? (ULONG)(ticks / freq) : 0;
    days = secs / 86400; hours = secs / 3600 % 24; mins = secs / 60 % 60;
    if (since) {
        /* now minus the time up, as a date: 2026-10-14 20:57:12 */
        LONG y, m, d, t2;
        LONG nowsecs;
        cu_dsfix(&now);
        nowsecs = now.ds_Days * 86400 + now.ds_Minute * 60 + now.ds_Tick / TICKS_PER_SECOND - (LONG)secs;
        cu_civil(nowsecs / 86400, &y, &m, &d);
        t2 = nowsecs % 86400;
        cu_putnum(y, 0); cu_putc('-'); two(m); cu_putc('-'); two(d); cu_putc(' ');
        two(t2 / 3600); cu_putc(':'); two(t2 / 60 % 60); cu_putc(':'); two(t2 % 60); cu_putc('\n');
        cu_flush();
        return cu_rc;
    }
    if (pretty) {
        int first = 1;
        cu_puts("up ");
        if (days) unitword(days, "day", &first);
        if (hours) unitword(hours, "hour", &first);
        if (mins || first) unitword(mins, "minute", &first);
        cu_putc('\n');
        cu_flush();
        return cu_rc;
    }
    n = cu_tasks(t, 128);
    for (i = 0; i < n; i++) if (t[i].cli) shells++;
    two(now.ds_Minute / 60); cu_putc(':'); two(now.ds_Minute % 60); cu_putc(':'); two(now.ds_Tick / TICKS_PER_SECOND);
    cu_puts(" up ");
    if (days) { cu_putnum(days, 0); cu_puts(days == 1 ? " day, " : " days, "); }
    cu_putnum(hours, 0); cu_putc(':'); two(mins);
    cu_puts(", ");
    cu_putnum(shells, 0);
    cu_puts(shells == 1 ? " shell\n" : " shells\n");
    cu_flush();
    return cu_rc;
}
