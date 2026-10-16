/* dirname - a path without its last part.
 *
 *   dirname NAME ...
 *
 * Everything before the last part, the Amiga way: "Work:a/b" gives
 * Work:a, "Work:a" gives Work:, "Work:" stays Work:, "a/b" gives a. A
 * name with no drawer in it ("file") gives an empty line: on the Amiga
 * the current drawer is "" (Unix says ".", which is an ordinary name
 * here), so `CD "`dirname x`"` and joining with / both work. A slash at
 * the end is left out first ("a/b/" gives a). Text only: nothing is
 * looked up on the disk. One line per NAME.
 *
 * Return code 0, 10 bad arguments. */
#include "cu.h"

const char *cu_name = "dirname";
static const char version[] = "$VER: dirname 0.1b2 (16.10.2026)";

static void one(const char *s)
{
    int n = strlen(s), e;
    while (n > 1 && s[n - 1] == '/' && s[n - 2] != ':' && s[n - 2] != '/') n--;   /* "a/b/" */
    if (n && s[n - 1] == ':') e = n;                                         /* "Work:" */
    else {
        for (e = n; e > 0 && s[e - 1] != '/' && s[e - 1] != ':'; e--) ;     /* the last part off */
        if (e > 1 && s[e - 1] == '/' && s[e - 2] != '/' && s[e - 2] != ':') e--;   /* "a/b": a, but "/b" and "a:/b" keep it */
    }
    cu_put(s, e);
    cu_putc('\n');
}

int main(void)
{
    char *v;
    int c, i;
    (void)version;
    if (cu_args("dirname NAME ...") < 0) return cu_rc;
    while ((c = cu_opt("", &v)) >= 0) return cu_rc;
    if (!cu_argc) { cu_err(10, NULL, "no name (dirname ? for usage)"); return cu_rc; }
    for (i = 0; i < cu_argc; i++) one(cu_argv[i]);
    cu_flush();
    return cu_rc;
}
