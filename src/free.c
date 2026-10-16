/* free - how much memory is free.
 *
 *   free [-h | -b | -k | -m]
 *
 * Chip memory (the custom chips can see it: pictures, sound), fast memory
 * (everything else), and both together: the size, what is in use, what is
 * free, and the largest free block (the biggest single piece a program can
 * get: memory in many small pieces is free but of little use).
 *   -k kilobytes (1024s, the default)   -m megabytes   -b bytes
 *   -h human: K, M, G as fits
 *
 * Return code 0, 10 bad arguments. */
#include "cu.h"

const char *cu_name = "free";
static const char version[] = "$VER: free 0.1b2 (16.10.2026)";

static int unit = 'k';

static void num(ULONG v)
{
    char b[16];
    if (unit == 'b') { cu_putnum(v, 11); return; }
    if (unit == 'm') { cu_putnum((v + 512 * 1024) >> 20, 11); return; }
    if (unit == 'k') { cu_putnum((v + 512) >> 10, 11); return; }
    /* human: one decimal under 10 */
    {
        const char *u = "BKMG";
        int k = 0;
        ULONG whole = v, tenth = 0;
        while (whole >= 1024 && k < 3) { tenth = (whole % 1024) * 10 / 1024; whole /= 1024; k++; }
        if (k && whole < 10) {
            int n = 0;
            b[n++] = '0' + whole;
            b[n++] = '.';
            b[n++] = '0' + tenth;
            b[n++] = u[k];
            b[n] = 0;
        } else {
            int n = 0, i;
            char t[12];
            do t[n++] = '0' + whole % 10; while ((whole /= 10));
            for (i = 0; i < n; i++) b[i] = t[n - 1 - i];
            if (k) b[n++] = u[k];
            b[n] = 0;
        }
        { int n = strlen(b); while (n++ < 11) cu_putc(' '); }
        cu_puts(b);
    }
}

static void row(const char *name, ULONG type)
{
    ULONG total = AvailMem(type | MEMF_TOTAL), avail = AvailMem(type), largest = AvailMem(type | MEMF_LARGEST);
    cu_puts(name);
    num(total);
    num(total - avail);
    num(avail);
    num(largest);
    cu_putc('\n');
}

int main(void)
{
    char *v;
    int c;
    (void)version;
    if (cu_args("free [-h | -b | -k | -m]") < 0) return cu_rc;
    while ((c = cu_opt("hbkmg", &v)) >= 0) switch (c) {
        case 'h': case 'b': case 'k': case 'm': unit = c; break;
        case 'g': unit = 'h'; break;
        default: return cu_rc;
    }
    cu_puts("     "); { static const char *const h[] = { "total", "used", "free", "largest" }; int i; for (i = 0; i < 4; i++) { int k = 11 - strlen(h[i]); while (k-- > 0) cu_putc(' '); cu_puts(h[i]); } } cu_putc('\n');
    row("chip ", MEMF_CHIP);
    if (AvailMem(MEMF_FAST | MEMF_TOTAL)) row("fast ", MEMF_FAST);
    row("total", MEMF_ANY);
    cu_flush();
    return cu_rc;
}
