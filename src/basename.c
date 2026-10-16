/* basename - the last part of a path.
 *
 *   basename NAME [SUFFIX]
 *   basename -a [-s SUFFIX] NAME ...
 *
 * What comes after the last / or : ("Work:dir/file.c" gives file.c),
 * slashes at the end left out first ("Work:dir/" gives dir). A volume or
 * assign alone stays as it is ("Work:" gives Work:, as Unix keeps "/"):
 * it is the name of that place. SUFFIX is taken off the end when the name
 * is longer than it (basename file.c .c gives file). Paths are text only:
 * nothing is looked up on the disk.
 *   -a  every word is a NAME   -s SUFFIX  take it off each (implies -a)
 *
 * Return code 0, 10 bad arguments. */
#include "cu.h"

const char *cu_name = "basename";
static const char version[] = "$VER: basename 0.1b2 (16.10.2026)";

static void one(const char *s, const char *suf)
{
    int n = strlen(s), e, b, k;
    while (n > 1 && s[n - 1] == '/' && s[n - 2] != ':' && s[n - 2] != '/') n--;   /* "dir/" */
    e = n;
    if (e && s[e - 1] == ':') { cu_put(s, e); cu_putc('\n'); return; }      /* "Work:" */
    if (e && s[e - 1] == '/') {                                             /* "/" or "a:/" or "a//": parents */
        cu_put(s, e); cu_putc('\n'); return;
    }
    for (b = e; b > 0 && s[b - 1] != '/' && s[b - 1] != ':'; b--) ;
    if (suf && (k = strlen(suf)) && e - b > k && !memcmp(s + e - k, suf, k)) e -= k;
    cu_put(s + b, e - b);
    cu_putc('\n');
}

int main(void)
{
    char *v, *suf = NULL;
    int c, i, all = 0;
    (void)version;
    if (cu_args("basename NAME [SUFFIX] | -a [-s SUFFIX] NAME ...") < 0) return cu_rc;
    while ((c = cu_opt("as:", &v)) >= 0) switch (c) {
        case 'a': all = 1; break;
        case 's': suf = v; all = 1; break;
        default: return cu_rc;
    }
    if (!cu_argc) { cu_err(10, NULL, "no name (basename ? for usage)"); return cu_rc; }
    if (!all) {
        if (cu_argc > 2) { cu_err(10, cu_argv[2], "one name and a suffix at most (-a for more)"); return cu_rc; }
        one(cu_argv[0], cu_argc == 2 ? cu_argv[1] : NULL);
    } else for (i = 0; i < cu_argc; i++) one(cu_argv[i], suf);
    cu_flush();
    return cu_rc;
}
