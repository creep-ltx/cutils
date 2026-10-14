/* tui - see tui.h. Grown from Ahiru's terminal mode (Ahiru src/term.c).
 *
 * Keys come without blocking: an ACTION_WAIT_CHAR packet (a quarter of a
 * second at most) goes to the console with a reply port of our own, so
 * its signal sits in the one Wait() beside Ctrl-C..F (which the console
 * sends as signals, never as bytes) and whatever the program adds; when
 * it says a key is there, Read() takes what came. Always answered soon:
 * quitting never waits on the person.
 *
 * Both key conventions are understood: the Amiga console's (0x9B CSI,
 * CSI T / S / SP A / SP @ for the shifted arrows, CSI n~ the function
 * keys, CSI ?~ Help) and xterm's (ESC [ A, ESC [ 1;5D, ESC [ 3~, ESC O P),
 * which CTerm sends for Ctrl/Alt/Shift+arrows to a program that asked
 * for bracketed paste. */
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include "tui.h"

#define ID_CTRM  0x4354524DUL   /* CTerm's console answers this to DISK_INFO */
#define WAITUS   250000         /* the longest a key wait packet stays out */
#define BREAKS   (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F)
#define MAXW     400
#define MAXH     200

int tui_cols = 80, tui_rows = 24, tui_cterm, tui_utf8, tui_cellw, tui_cellh;

static BPTR tin, tout;
static struct MsgPort *conport, *rport;
static struct DosPacket *wpkt;   /* AllocDosObject's: the packet itself */
static int wpending, topen, tsusp;

/* ---- output ---------------------------------------------------------- */

static UBYTE obuf[4096];
static int olen;

static void oflush(void)
{
    if (olen && tout) Write(tout, obuf, olen);
    olen = 0;
}

static void out(const char *s, int n)
{
    if (olen + n > (int)sizeof(obuf)) oflush();
    if (n > (int)sizeof(obuf)) {
        Write(tout, (APTR)s, n);
        return;
    }
    memcpy(obuf + olen, s, n);
    olen += n;
}

static void outs(const char *s)
{
    out(s, (int)strlen(s));
}

static void outnum(int v)
{
    char b[12];
    int i = sizeof(b);
    if (v < 0) v = 0;
    do b[--i] = (char)('0' + v % 10); while ((v /= 10) && i);
    out(b + i, (int)sizeof(b) - i);
}

/* ---- the grid: what should be shown, and what the terminal shows ----- */

typedef struct {
    ULONG c;
    UBYTE fg, bg, at, pad;
} Cell;

static Cell *want, *shown;       /* tui_rows x tui_cols each */
static int gw, gh;               /* their size */
static int curx = -1, cury = -1, curfg = -1, curbg = -1, curat = -1;
static int cux, cuy, cuon, cushown = -1, cuxs = -1, cuys = -1;

static void gridfree(void)
{
    if (want) FreeVec(want);
    if (shown) FreeVec(shown);
    want = shown = NULL;
    gw = gh = 0;
}

static int gridalloc(void)
{
    int i, n;
    if (gw == tui_cols && gh == tui_rows && want) return 1;
    gridfree();
    n = tui_cols * tui_rows;
    want = AllocVec(n * sizeof(Cell), MEMF_ANY);
    shown = AllocVec(n * sizeof(Cell), MEMF_ANY);
    if (!want || !shown) {
        gridfree();
        return 0;
    }
    gw = tui_cols;
    gh = tui_rows;
    for (i = 0; i < n; i++) {
        want[i].c = ' ';
        want[i].fg = 7;
        want[i].bg = TC_DEFAULT;
        want[i].at = 0;
        shown[i] = want[i];
        shown[i].c = 0xFFFFFFFF;        /* unknown: sent on the next flush */
    }
    return 1;
}

void tui_redraw(void)
{
    int i, n = gw * gh;
    for (i = 0; i < n; i++) shown[i].c = 0xFFFFFFFF;
    curx = cury = curfg = curbg = curat = -1;
    cushown = cuxs = cuys = -1;
    if (topen) outs("\x1b[0m\x1b[H\x1b[J");
}

/* a code point a cell can show: wide (CJK, emoji) and combining marks
 * would move everything after them, controls would steer the terminal */
static ULONG cellable(ULONG c)
{
    if (c < 32 || (c >= 0x7F && c < 0xA0)) return '?';
    if ((c >= 0x300 && c < 0x370) || (c >= 0x1100 && c < 0x1160) ||
        (c >= 0x2E80 && c < 0xA4D0) || (c >= 0xAC00 && c < 0xD7A4) ||
        (c >= 0xF900 && c < 0xFB00) || (c >= 0xFE00 && c < 0xFE50) ||
        (c >= 0xFF00 && c < 0xFF61) || (c >= 0xFFE0 && c < 0xFFE7) ||
        (c >= 0xD800 && c < 0xE000) || c >= 0x1F000 || c == 0x200B ||
        c == 0x200D || c == 0xFEFF)
        return '?';
    if (!tui_utf8 && c > 0xFF) return '?';
    return c;
}

void tui_putc(int col, int row, ULONG c, int fg, int bg, int attr)
{
    Cell *p;
    if (col < 0 || row < 0 || col >= gw || row >= gh) return;
    p = &want[row * gw + col];
    p->c = cellable(c);
    p->fg = (UBYTE)fg;
    p->bg = (UBYTE)bg;
    p->at = (UBYTE)attr;
}

/* s as Latin-1 bytes (a program with UTF-8 text decodes it itself and
 * calls tui_putc) */
void tui_put(int col, int row, const char *s, int len, int fg, int bg, int attr)
{
    int i;
    for (i = 0; i < len && col + i < gw; i++)
        tui_putc(col + i, row, (unsigned char)s[i], fg, bg, attr);
}

void tui_fill(int col, int row, int w, ULONG c, int fg, int bg, int attr)
{
    int i;
    for (i = 0; i < w; i++) tui_putc(col + i, row, c, fg, bg, attr);
}

void tui_clear(void)
{
    int r;
    for (r = 0; r < gh; r++) tui_fill(0, r, gw, ' ', 7, TC_DEFAULT, 0);
}

void tui_cursor(int col, int row, int on)
{
    cux = col;
    cuy = row;
    cuon = on;
}

/* a plain console's pens are its screen's: Workbench's (0 grey, 1 black,
 * 2 white, 3 blue) - the ANSI sixteen as near as they go */
static const UBYTE wbpen[17] = { 1, 1, 1, 1, 3, 1, 3, 2, 1, 1, 1, 2, 3, 1, 3, 2, 0 };

static void sgr(int fg, int bg, int at)
{
    if (fg == curfg && bg == curbg && at == curat) return;
    curfg = fg;
    curbg = bg;
    curat = at;
    outs("\x1b[0");
    if (at & TA_BOLD) outs(";1");
    if (at & TA_UNDER) outs(";4");
    if (at & TA_REV) outs(";7");
    if (!tui_cterm) {
        fg = fg == TC_DEFAULT ? 1 : wbpen[fg];
        bg = bg == TC_DEFAULT ? TC_DEFAULT : wbpen[bg];
    }
    out(";", 1);
    if (fg == TC_DEFAULT) outs("39");
    else if (fg < 8) outnum(30 + fg);
    else outnum(90 + fg - 8);
    out(";", 1);
    if (bg == TC_DEFAULT) outs("49");
    else if (bg < 8) outnum(40 + bg);
    else outnum(100 + bg - 8);
    out("m", 1);
}

static void moveto(int col, int row)
{
    if (col == curx && row == cury) return;
    outs("\x1b[");
    outnum(row + 1);
    out(";", 1);
    outnum(col + 1);
    out("H", 1);
    curx = col;
    cury = row;
}

static void putchar_(ULONG c)
{
    char b[4];
    if (c < 0x80 || !tui_utf8) {
        b[0] = (char)c;
        out(b, 1);
    } else if (c < 0x800) {
        b[0] = (char)(0xC0 | (c >> 6));
        b[1] = (char)(0x80 | (c & 0x3F));
        out(b, 2);
    } else if (c < 0x10000) {
        b[0] = (char)(0xE0 | (c >> 12));
        b[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        b[2] = (char)(0x80 | (c & 0x3F));
        out(b, 3);
    } else {
        b[0] = (char)(0xF0 | (c >> 18));
        b[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        b[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        b[3] = (char)(0x80 | (c & 0x3F));
        out(b, 4);
    }
}

static void cursorvis(int on)
{
    if (on == cushown) return;
    if (tui_cterm) outs(on ? "\x1b[?25h" : "\x1b[?25l");
    else outs(on ? "\x1b[ p" : "\x1b[0 p");
    cushown = on;
}

void tui_flush(void)
{
    int r, c;
    if (!topen || tsusp || !want) return;
    for (r = 0; r < gh; r++) {
        Cell *w = want + r * gw, *s = shown + r * gw;
        for (c = 0; c < gw; c++) {
            if (w[c].c == s[c].c && w[c].fg == s[c].fg && w[c].bg == s[c].bg && w[c].at == s[c].at)
                continue;
            if (!tui_cterm && r == gh - 1 && c == gw - 1) {
                s[c] = w[c];                /* a console that would scroll there */
                continue;
            }
            if (cushown) cursorvis(0);      /* no cursor jumping about while drawing */
            moveto(c, r);
            sgr(w[c].fg, w[c].bg, w[c].at);
            putchar_(w[c].c);
            s[c] = w[c];
            curx++;
            if (curx >= gw) curx = -1;      /* the wrap is not ours to guess */
        }
    }
    if (cuon && cux >= 0 && cuy >= 0 && cux < gw && cuy < gh) {
        moveto(cux, cuy);
        cursorvis(1);
    } else
        cursorvis(0);
    oflush();
}

void tui_title(const char *t)
{
    if (!tui_cterm || !topen) return;
    outs("\x1b]2;");
    outs(t);
    out("\x07", 1);
}

void tui_beep(void)
{
    out("\x07", 1);
    oflush();
}

ULONG tui_box(int which)
{
    static const ULONG uni[] = { 0x2500, 0x2502, 0x256D, 0x256E, 0x2570, 0x256F, 0x25B8, 0x00B7 };
    static const char asc[] = "-|++++>.";
    if (tui_cterm && tui_utf8) return uni[which];
    if (which == BX_DOT) return 0xB7;   /* Latin-1 has the middle dot */
    return (unsigned char)asc[which];
}

/* ---- input ------------------------------------------------------------ */

#define EVQ 64
static TuiEv evq[EVQ];
static int evh, evt;
static UBYTE ibuf[256];
static int ilen;
static UBYTE *pastebuf, *pastegive;  /* a paste being gathered; the one handed out */
static long pastelen, pastecap;
static int inpaste;

static void push(int type, int key)
{
    int n = (evt + 1) % EVQ;
    TuiEv *e;
    if (n == evh) return;               /* full: one too many is dropped */
    e = &evq[evt];
    memset(e, 0, sizeof(*e));
    e->type = type;
    e->key = key;
    CurrentTime(&e->secs, &e->mics);
    evt = n;
}

static void pushmouse(int act, int col, int row, int mods)
{
    TuiEv *e;
    push(TE_MOUSE, 0);
    e = &evq[(evt + EVQ - 1) % EVQ];
    e->mact = act;
    e->col = col;
    e->row = row;
    e->mods = mods;
}

static void setsize(int rows, int cols)
{
    if (rows < 3 || cols < 10) return;
    if (cols > MAXW) cols = MAXW;
    if (rows > MAXH) rows = MAXH;
    if (rows == tui_rows && cols == tui_cols) return;
    tui_rows = rows;
    tui_cols = cols;
    push(TE_RESIZE, 0);
}

static void pasteadd(const UBYTE *b, long n)
{
    if (pastelen + n > pastecap) {
        long cap = (pastelen + n) * 2 + 1024;
        UBYTE *p = AllocVec(cap, MEMF_ANY);
        if (!p) return;                 /* no memory: the rest is lost */
        if (pastebuf) {
            CopyMem(pastebuf, p, pastelen);
            FreeVec(pastebuf);
        }
        pastebuf = p;
        pastecap = cap;
    }
    CopyMem((APTR)b, pastebuf + pastelen, n);
    pastelen += n;
}

/* a modifier number (xterm's 1 + shift + 2 alt + 4 ctrl) as TM_* */
static int mods(int m)
{
    int r = 0;
    if (m < 2) return 0;
    m--;
    if (m & 1) r |= TM_SHIFT;
    if (m & 2) r |= TM_ALT;
    if (m & 4) r |= TM_CTRL;
    return r;
}

/* one finished CSI; fromamiga: it came as 0x9B (the Amiga console's codes) */
static void csi(const int *p, int np, int priv, int inter, int f, int fromamiga)
{
    int p0 = np > 0 ? p[0] : 0, m = np > 1 ? mods(p[1]) : 0;
    if (priv == '<' && (f == 'M' || f == 'm')) {        /* SGR mouse */
        int b = p0, x = np > 1 ? p[1] - 1 : 0, y = np > 2 ? p[2] - 1 : 0, mm = 0;
        if (b & 4) mm |= TM_SHIFT;
        if (b & 8) mm |= TM_ALT;
        if (b & 16) mm |= TM_CTRL;
        if (b & 64) pushmouse((b & 1) ? TB_WHEELDOWN : TB_WHEELUP, x, y, mm);
        else if (f == 'm') pushmouse(TB_RELEASE, x, y, mm);
        else if (b & 32) pushmouse(TB_DRAG, x, y, mm);
        else if ((b & 3) == 0) pushmouse(TB_PRESS, x, y, mm);
        return;
    }
    if (inter == ' ') {
        if (f == 'r' && np >= 4) setsize(p[2], p[3]);    /* window bounds */
        else if (f == 'A') push(TE_KEY, K_LEFT | TM_SHIFT);   /* Amiga */
        else if (f == '@') push(TE_KEY, K_RIGHT | TM_SHIFT);
        return;
    }
    if (priv == '?') {
        if (f == '~') push(TE_KEY, K_HELP);
        return;
    }
    if (priv) return;
    switch (f) {
    case 'A': push(TE_KEY, K_UP | m); break;
    case 'B': push(TE_KEY, K_DOWN | m); break;
    case 'C': push(TE_KEY, K_RIGHT | m); break;
    case 'D': push(TE_KEY, K_LEFT | m); break;
    case 'H': push(TE_KEY, K_HOME | m); break;
    case 'F': push(TE_KEY, K_END | m); break;
    case 'T': push(TE_KEY, K_UP | TM_SHIFT); break;     /* Amiga Shift+Up */
    case 'S': push(TE_KEY, K_DOWN | TM_SHIFT); break;   /* Amiga Shift+Down */
    case 'Z': push(TE_KEY, K_TAB | TM_SHIFT); break;
    case '~':
        if (fromamiga) {
            if (np && p0 <= 9) push(TE_KEY, K_F1 + p0);
            else if (p0 >= 10 && p0 <= 19) push(TE_KEY, (K_F1 + p0 - 10) | TM_SHIFT);
            break;
        }
        switch (p0) {
        case 1: case 7: push(TE_KEY, K_HOME | m); break;
        case 2: push(TE_KEY, K_INS | m); break;
        case 3: push(TE_KEY, K_DEL | m); break;
        case 4: case 8: push(TE_KEY, K_END | m); break;
        case 5: push(TE_KEY, K_PGUP | m); break;
        case 6: push(TE_KEY, K_PGDN | m); break;
        case 11: case 12: case 13: case 14: case 15:
            push(TE_KEY, (K_F1 + p0 - 11) | m); break;
        case 17: case 18: case 19: case 20: case 21:
            push(TE_KEY, (K_F1 + 5 + p0 - 17) | m); break;
        case 23: case 24: push(TE_KEY, (K_F1 + 10 + p0 - 23) | m); break;
        case 200: inpaste = 1; pastelen = 0; break;
        }
        break;
    case '|':                       /* a raw input event: 12 is a resize */
        if (p0 == 12) {
            outs("\x1b[0 q");
            oflush();
        }
        break;
    case 't':
        if (p0 == 8 && np >= 3) setsize(p[1], p[2]);
        else if (p0 == 6 && np >= 3) {
            tui_cellh = p[1];
            tui_cellw = p[2];
        }
        break;
    }
}

/* how long the UTF-8 sequence starting with b is (1: not a lead byte) */
static int u8len(UBYTE b)
{
    if (b >= 0xC2 && b <= 0xDF) return 2;
    if (b >= 0xE0 && b <= 0xEF) return 3;
    if (b >= 0xF0 && b <= 0xF4) return 4;
    return 1;
}

/* the bytes in ibuf: whole sequences become events, a cut one stays
 * (final: nothing more is coming, a stub is what it is) */
static void parse(int final)
{
    int i = 0;
    while (i < ilen) {
        UBYTE c = ibuf[i];
        int start = i, cs = -1, fromamiga = 0;
        if (inpaste) {
            /* everything up to ESC [ 201 ~ is the paste's */
            int j;
            for (j = i; j < ilen; j++)
                if (ibuf[j] == 0x1B) break;
            if (j > i) pasteadd(ibuf + i, j - i);
            i = j;
            if (i >= ilen) break;
            if (ilen - i < 6) {
                if (!final) break;
                pasteadd(ibuf + i, ilen - i);
                i = ilen;
                break;
            }
            if (!memcmp(ibuf + i, "\x1b[201~", 6)) {
                TuiEv *e;
                inpaste = 0;
                if (pastegive) FreeVec(pastegive);
                pastegive = pastebuf;
                pastebuf = NULL;
                push(TE_PASTE, 0);
                e = &evq[(evt + EVQ - 1) % EVQ];
                e->text = pastegive;
                e->len = pastelen;
                pastelen = pastecap = 0;
                i += 6;
            } else {
                pasteadd(ibuf + i, 1);
                i++;
            }
            continue;
        }
        if (c == 0x9B) {
            cs = i + 1;
            fromamiga = 1;
        } else if (c == 0x1B) {
            if (i + 1 >= ilen) {
                if (!final) break;          /* more may come */
                push(TE_KEY, K_ESC);
                i++;
                continue;
            }
            if (ibuf[i + 1] == '[') cs = i + 2;
            else if (ibuf[i + 1] == 'O') {  /* SS3 */
                int k = 0;
                if (i + 2 >= ilen) {
                    if (!final) break;
                    push(TE_KEY, K_ESC);
                    i += 2;
                    continue;
                }
                switch (ibuf[i + 2]) {
                case 'P': k = K_F(1); break;
                case 'Q': k = K_F(2); break;
                case 'R': k = K_F(3); break;
                case 'S': k = K_F(4); break;
                case 'A': k = K_UP; break;
                case 'B': k = K_DOWN; break;
                case 'C': k = K_RIGHT; break;
                case 'D': k = K_LEFT; break;
                case 'H': k = K_HOME; break;
                case 'F': k = K_END; break;
                }
                if (k) push(TE_KEY, k);
                i += 3;
                continue;
            } else {
                /* ESC and a key: Alt+key as xterm sends it (meta) */
                UBYTE n = ibuf[i + 1];
                if (n >= 32 && n < 127) push(TE_KEY, TM_ALT | n);
                else if (n == 0x7F || n == 8) push(TE_KEY, TM_ALT | K_BS);
                else push(TE_KEY, K_ESC), i--;
                i += 2;
                continue;
            }
        }
        if (cs >= 0) {
            int p[8], np = 0, priv = 0, inter = 0, j = cs, have = 0, done = 0;
            memset(p, 0, sizeof(p));
            if (j < ilen && (ibuf[j] == '?' || ibuf[j] == '<' || ibuf[j] == '>' || ibuf[j] == '=')) priv = ibuf[j++];
            for (; j < ilen; j++) {
                UBYTE b = ibuf[j];
                if (b >= '0' && b <= '9') {
                    if (np < 8) p[np] = p[np] * 10 + (b - '0');
                    have = 1;
                } else if (b == ';') {
                    if (np < 8) np++;
                    have = 1;
                } else if (b >= 0x20 && b <= 0x2F) inter = b;
                else if (b >= 0x40 && b <= 0x7E) {
                    if (have && np < 8) np++;
                    csi(p, np, priv, inter, b, fromamiga);
                    done = 1;
                    break;
                } else {
                    done = 1;               /* not a sequence after all */
                    break;
                }
            }
            if (!done) {
                if (!final) break;          /* cut off: wait for the rest */
                j = ilen - 1;               /* a stub never finished: dropped */
            }
            i = j + 1;
            continue;
        }
        if (c >= 0x80) {
            int n = u8len(c), k;
            if (n > 1) {
                if (i + n > ilen && !final) break;
                for (k = 1; k < n && i + k < ilen && (ibuf[i + k] & 0xC0) == 0x80; k++) ;
                if (k == n) {
                    ULONG u = c & (0x7F >> n);
                    for (k = 1; k < n; k++) u = (u << 6) | (ibuf[i + k] & 0x3F);
                    push(TE_KEY, (int)u);
                    i += n;
                    continue;
                }
            }
            push(TE_KEY, c);                /* Latin-1, as the keymap gives it */
            i++;
            continue;
        }
        switch (c) {
        case 8: push(TE_KEY, K_BS); break;
        case 9: push(TE_KEY, K_TAB); break;
        case 10: case 13: push(TE_KEY, K_ENTER); break;
        case 0x7F: push(TE_KEY, K_DEL); break;
        default:
            if (c < 32) push(TE_KEY, TM_CTRL | (c + 'a' - 1));
            else push(TE_KEY, c);
        }
        i++;
        (void)start;
    }
    if (i > 0) {
        memmove(ibuf, ibuf + i, ilen - i);
        ilen -= i;
    }
}

/* what the console has now (nothing waiting: nothing is read) */
static void readnow(void)
{
    int guard = 16;
    while (guard-- && WaitForChar(tin, 0)) {
        LONG n = Read(tin, ibuf + ilen, sizeof(ibuf) - ilen);
        if (n <= 0) break;
        ilen += (int)n;
        parse(0);
        if (ilen >= (int)sizeof(ibuf) - 8) {    /* one long run (a paste): take it as it is */
            parse(1);
            if (ilen >= (int)sizeof(ibuf) - 8) ilen = 0;
        }
    }
    if (ilen) {
        /* a lone Esc, or a sequence cut between reads: a moment for the rest */
        if (WaitForChar(tin, 40000)) {
            LONG n = Read(tin, ibuf + ilen, sizeof(ibuf) - ilen);
            if (n > 0) ilen += (int)n;
            parse(0);
        }
        if (ilen && !inpaste) parse(1);
    }
}

static void arm(long us)
{
    if (wpending || tsusp || !topen) return;
    wpkt->dp_Type = ACTION_WAIT_CHAR;
    wpkt->dp_Arg1 = us > 0 && us < WAITUS ? us : WAITUS;
    SendPkt(wpkt, conport, rport);
    wpending = 1;
}

/* the wait packet back (if out): what it says is taken; 1 = it came back */
static int collect(int block)
{
    int back = 0;
    while (wpending) {
        if (!GetMsg(rport)) {
            if (!block) return back;
            WaitPort(rport);
            continue;
        }
        wpending = 0;
        back = 1;
        if (wpkt->dp_Res1) readnow();
    }
    return back;
}

static void breaks(ULONG sig)
{
    /* Ctrl-C..F come as signals: keys like the others */
    if (sig & SIGBREAKF_CTRL_C) push(TE_KEY, TM_CTRL | 'c');
    if (sig & SIGBREAKF_CTRL_D) push(TE_KEY, TM_CTRL | 'd');
    if (sig & SIGBREAKF_CTRL_E) push(TE_KEY, TM_CTRL | 'e');
    if (sig & SIGBREAKF_CTRL_F) push(TE_KEY, TM_CTRL | 'f');
}

int tui_wait(TuiEv *e, long us, ULONG extra, ULONG *got)
{
    ULONG waited = 0;
    if (got) *got = 0;
    for (;;) {
        ULONG sig;
        breaks(SetSignal(0, BREAKS) & BREAKS);
        if (evh != evt) {
            *e = evq[evh];
            evh = (evh + 1) % EVQ;
            return 1;
        }
        collect(0);
        if (evh != evt) continue;
        if (us == 0) {
            e->type = TE_NONE;
            return 0;
        }
        tui_flush();
        arm(us);
        sig = Wait((rport ? 1UL << rport->mp_SigBit : 0) | BREAKS | extra);
        if (got) *got |= sig & extra;
        breaks(sig & BREAKS);
        if (sig & extra) {
            collect(0);
            if (evh == evt) {
                e->type = TE_NONE;
                return 0;
            }
            continue;
        }
        if (collect(0) && us > 0) {
            waited += WAITUS;
            if (evh == evt && (long)waited >= us) {
                e->type = TE_NONE;
                return 0;
            }
        }
    }
}

/* ---- opening and closing ------------------------------------------------ */

static void modes_on(void)
{
    outs("\x1b[?1049h");            /* its own screen (CTerm; others ignore it) */
    outs("\x1b[?7l\x1b[>1l");       /* no wrap, no scroll at the last cell */
    outs("\x1b[12{");               /* resizes reported (raw event class 12) */
    if (tui_cterm) {
        outs("\x1b%G");             /* UTF-8 both ways: every character shows */
        tui_utf8 = 1;
        outs("\x1b[?1000h\x1b[?1002h\x1b[?1006h");  /* the mouse, with drags */
        outs("\x1b[?2004h");        /* pastes marked (and modern keys: CTerm) */
    } else
        tui_utf8 = 0;
    curx = cury = curfg = curbg = curat = -1;
    cushown = -1;
}

static void modes_off(void)
{
    outs("\x1b[12}\x1b[0m\x1b[?7h\x1b[>1h");
    if (tui_cterm) {
        outs("\x1b[?2004l\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?25h");
        outs("\x1b%@");             /* the Shell's characters again */
    } else
        outs("\x1b[ p\x1b[H\x1b[J");    /* cursor on; no second screen: cleared */
    outs("\x1b[?1049l");
    oflush();
}

static void readreplies(int tries, int *flag)
{
    while (tries-- > 0 && !*flag) {
        if (!WaitForChar(tin, 100000)) continue;
        {
            LONG n = Read(tin, ibuf + ilen, sizeof(ibuf) - ilen);
            if (n > 0) ilen += (int)n;
            parse(0);
        }
    }
}

/* the size: the Amiga window bounds report, else xterm's, else 80 x 24;
 * CTerm's cell in pixels too */
static void asksize(void)
{
    int r0 = tui_rows, c0 = tui_cols, got = 0, h0 = tui_cellh;
    tui_rows = tui_cols = 0;
    outs("\x1b[0 q");
    oflush();
    readreplies(10, &tui_rows);
    if (!tui_rows) {
        outs("\x1b[18t");
        oflush();
        readreplies(5, &tui_rows);
    }
    if (!tui_rows) {
        tui_rows = r0 > 0 ? r0 : 24;
        tui_cols = c0 > 0 ? c0 : 80;
    }
    if (tui_cterm) {
        outs("\x1b[16t");
        oflush();
        tui_cellh = 0;
        readreplies(5, &tui_cellh);
        if (!tui_cellh) tui_cellh = h0;
    }
    (void)got;
    {   /* the size events pushed on the way are not news */
        int i, j = evh;
        for (i = evh; i != evt; i = (i + 1) % EVQ)
            if (evq[i].type != TE_RESIZE) {
                evq[j] = evq[i];
                j = (j + 1) % EVQ;
            }
        evt = j;
    }
}

/* the console's input: Input(), or (Input() a pipe) the console opened as "*" */
static BPTR conin;

void tui_console(BPTR in)
{
    conin = in;
}

int tui_possible(void)
{
    struct FileHandle *fh;
    BPTR in = conin ? conin : Input(), o = Output();
    if (!in || !o || !IsInteractive(in) || !IsInteractive(o)) return 0;
    fh = (struct FileHandle *)BADDR(in);
    return fh->fh_Type != NULL;         /* NIL: has none */
}

static int iscterm(struct MsgPort *port)
{
    struct InfoData *id;
    int yes = 0;
    if ((id = AllocVec(sizeof(*id), MEMF_PUBLIC | MEMF_CLEAR)) != NULL) {
        if (DoPkt(port, ACTION_DISK_INFO, MKBADDR(id), 0, 0, 0, 0))
            yes = id->id_DiskType == ID_CTRM;
        FreeVec(id);
    }
    return yes;
}

int tui_open(void)
{
    if (topen) return 1;
    if (!tui_possible()) return 0;
    tin = conin ? conin : Input();
    tout = Output();
    conport = ((struct FileHandle *)BADDR(tin))->fh_Type;
    tui_cterm = iscterm(conport);
    if (!(rport = CreateMsgPort())) return 0;
    if (!(wpkt = AllocDosObject(DOS_STDPKT, NULL))) {
        DeleteMsgPort(rport);
        rport = NULL;
        return 0;
    }
    evh = evt = ilen = 0;
    tsusp = 0;
    SetSignal(0, BREAKS);
    SetMode(tin, 1);
    modes_on();
    asksize();
    if (!gridalloc()) {
        modes_off();
        SetMode(tin, 0);
        FreeDosObject(DOS_STDPKT, wpkt);
        DeleteMsgPort(rport);
        rport = NULL;
        return 0;
    }
    topen = 1;
    tui_redraw();
    return 1;
}

void tui_close(void)
{
    if (!topen) return;
    collect(1);                         /* answered within WAITUS */
    modes_off();
    SetMode(tin, 0);
    FreeDosObject(DOS_STDPKT, wpkt);
    DeleteMsgPort(rport);
    wpkt = NULL;
    rport = NULL;
    gridfree();
    if (pastebuf) FreeVec(pastebuf);
    if (pastegive) FreeVec(pastegive);
    pastebuf = pastegive = NULL;
    topen = 0;
}

void tui_suspend(void)
{
    if (!topen) return;
    collect(1);
    tsusp = 1;
    modes_off();
    SetMode(tin, 0);
}

/* after a command in the terminal: its output stays until a key */
void tui_keyafter(void)
{
    static const char m[] = "\n\x1b[7m a key goes back \x1b[0m";
    char c;
    if (!topen) return;
    Write(tout, (APTR)m, sizeof(m) - 1);
    SetMode(tin, 1);
    Read(tin, &c, 1);
    while (WaitForChar(tin, 20000)) Read(tin, &c, 1);
    Write(tout, "\r\x1b[K", 4);
    SetMode(tin, 0);
}

void tui_resume(void)
{
    if (!topen) return;
    SetMode(tin, 1);
    tsusp = 0;
    modes_on();
    asksize();
    gridalloc();
    tui_redraw();
}

/* resizes: the grid follows (the program draws everything again) */
int tui_resized(void)
{
    if (gw == tui_cols && gh == tui_rows) return 0;
    gridalloc();
    tui_redraw();
    return 1;
}
