/* kill - stop a program: send it a break (Ctrl-C).
 *
 *   kill [-C | -D | -E | -F | -s SIG] TARGET ...
 *   kill -l
 *
 * A TARGET is a Shell's number (as ps and Status show), a task's address
 * (0x... from ps), or a name: every task whose name, or whose Shell's
 * command, is that name (case blind; a pattern with #? or * too) gets the
 * break - the way pkill and killall do it. A Shell's number reaches the
 * command it runs (a break goes to the Shell's process, where the command
 * runs). The break is what Ctrl-C in its window would do: a program that
 * checks for it ends, one that does not goes on.
 *   -C Ctrl-C (the default; also -INT, -TERM, -HUP, -2, -15)
 *   -D -E -F Ctrl-D, Ctrl-E, Ctrl-F   -s SIG the same by name (C D E F,
 *   INT, TERM)   -l the signals that can be sent
 *
 * There is no way to force a program out on the Amiga without risking the
 * machine (its memory and files stay taken), so kill only asks; -9 and
 * KILL are refused with a word.
 *
 * Return code 0, 5 a target matched no task, 10 bad arguments. */
#include <stdlib.h>
#include "cu.h"

const char *cu_name = "kill";
static const char version[] = "$VER: kill 0.1b1 (14.10.2026)";

#define MAXT 256

static ULONG sigof(const char *s)
{
    if (*s == '-') s++;
    if (!stricmp(s, "C") || !stricmp(s, "INT") || !stricmp(s, "TERM") || !stricmp(s, "HUP") ||
        !strcmp(s, "2") || !strcmp(s, "15") || !strcmp(s, "1") || !stricmp(s, "SIGINT") || !stricmp(s, "SIGTERM"))
        return SIGBREAKF_CTRL_C;
    if (!stricmp(s, "D")) return SIGBREAKF_CTRL_D;
    if (!stricmp(s, "E")) return SIGBREAKF_CTRL_E;
    if (!stricmp(s, "F")) return SIGBREAKF_CTRL_F;
    if (!strcmp(s, "9") || !stricmp(s, "KILL") || !stricmp(s, "SIGKILL")) return 1;
    return 0;
}

static int isnum(const char *s) { if (!*s) return 0; for (; *s; s++) if (*s < '0' || *s > '9') return 0; return 1; }

int main(void)
{
    static CuTask t[MAXT];
    char *v;
    ULONG sig = SIGBREAKF_CTRL_C;
    int c, i, j, n;
    APTR me = FindTask(NULL);
    (void)version;
    if (cu_args("kill [-C | -D | -E | -F | -s SIG] TARGET ... | kill -l") < 0) return cu_rc;
    /* -INT, -TERM, -9, -15: words cu_opt would take for bundles */
    for (i = 0; i < cu_argc; i++) {
        char *w = cu_argv[i];
        if (w[0] == '-' && w[1] && w[1] != 's' && (w[2] || (w[1] >= '0' && w[1] <= '9')) && w[1] != '-') {
            ULONG s = sigof(w);
            if (!s) { cu_err(10, w, "unknown signal (kill -l lists them)"); return cu_rc; }
            sig = s;
            memmove(&cu_argv[i], &cu_argv[i + 1], (cu_argc - i) * sizeof *cu_argv);
            cu_argc--; i--;
        }
    }
    while ((c = cu_opt("CDEFls:", &v)) >= 0) switch (c) {
        case 'C': sig = SIGBREAKF_CTRL_C; break;
        case 'D': sig = SIGBREAKF_CTRL_D; break;
        case 'E': sig = SIGBREAKF_CTRL_E; break;
        case 'F': sig = SIGBREAKF_CTRL_F; break;
        case 's': if (!(sig = sigof(v))) { cu_err(10, v, "unknown signal (kill -l lists them)"); return cu_rc; } break;
        case 'l':
            cu_puts("C (INT TERM HUP 2 15)  D  E  F\n");
            cu_flush();
            return 0;
        default: return cu_rc;
    }
    if (sig == 1) { cu_err(10, NULL, "the Amiga can not force a program out safely; kill sends a break (Ctrl-C)"); return cu_rc; }
    if (!cu_argc) { cu_err(10, NULL, "which program? (a Shell's number, 0x address or name; kill ? for help)"); return cu_rc; }
    n = cu_tasks(t, MAXT);
    for (i = 0; i < cu_argc; i++) {
        char *a = cu_argv[i], parsed[400];
        int hits = 0, gone = 0;
        if (isnum(a)) {
            long num = atol(a);
            for (j = 0; j < n; j++) if (t[j].cli == num) { hits++; if (!cu_signal(t[j].addr, sig)) gone++; }
        } else if ((a[0] == '0' && (a[1] == 'x' || a[1] == 'X')) || a[0] == '$') {
            ULONG addr = 0;
            char *p = a + (a[0] == '$' ? 1 : 2);
            for (; *p; p++) {
                int d = (*p >= '0' && *p <= '9') ? *p - '0' : (*p | 32) >= 'a' && (*p | 32) <= 'f' ? (*p | 32) - 'a' + 10 : -1;
                if (d < 0) break;
                addr = addr << 4 | d;
            }
            for (j = 0; j < n; j++) if ((ULONG)t[j].addr == addr) { hits++; if (!cu_signal(t[j].addr, sig)) gone++; }
        } else {
            if (!cu_pattern(a, parsed, sizeof parsed)) { cu_err(10, a, "bad pattern"); continue; }
            for (j = 0; j < n; j++) {
                CuTask *e = &t[j];
                if (e->addr == me) continue;              /* not kill itself */
                if (cu_match(parsed, e->name) || (e->cmd[0] && (cu_match(parsed, e->cmd) || cu_match(parsed, FilePart(e->cmd))))) {
                    hits++;
                    if (!cu_signal(e->addr, sig)) gone++;
                }
            }
        }
        if (!hits) cu_err(5, a, "no such program (ps -e lists them)");
        else if (gone == hits) cu_err(5, a, "already gone");
    }
    cu_flush();
    return cu_rc;
}
