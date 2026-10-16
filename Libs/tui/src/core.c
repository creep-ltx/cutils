/* tui core - see libraries/tui.h. The one copy of the toolkit: built into
 * tui.library (lib.c puts the calls in its jump table), or straight into
 * a program (TUI_STATIC, include/tui.h). Grown from Ahiru's terminal mode
 * (Ahiru src/term.c), then CUtils lib/tui.c, with Markdown's italic, dim,
 * strike, TuiDirty and TuiRaw merged in (2026-10-16). Version 2: 24-bit
 * colour, TuiPutUTF8, and the nearest character for a Latin-1 console.
 *
 * Everything a program's terminal needs lives in its own struct (Priv,
 * from TuiOpen): the library is shared, so nothing may be global.
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
#ifdef TUI_LIB
/* the library has no C library: lib.c has these (libnix's string.h
 * would turn memmove into a call of its own) */
void *memcpy(void *, const void *, unsigned long);
void *memmove(void *, const void *, unsigned long);
void *memset(void *, int, unsigned long);
int memcmp(const void *, const void *, unsigned long);
unsigned long strlen(const char *);
#else
#include <string.h>
#endif
#include <libraries/tui.h>
#include <clib/tui_protos.h>

#define ID_CTRM  0x4354524DUL   /* CTerm's console answers this to DISK_INFO */
#define WAITUS   250000         /* the longest a key wait packet stays out */
#define BREAKS   (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F)
#define MAXW     400
#define MAXH     200
#define EVQ      64

typedef struct {
    ULONG c;
    ULONG fg, bg;                /* 0-15, TC_DEFAULT, TC_X(n) or TC_RGB(r, g, b) */
    UBYTE at, pad[3];
} Cell;

/* cellable()'s answer: show it reversed (a full block in a plain console
 * is a reversed space) */
#define FB_REV 0x80000000UL

typedef struct {
    struct Tui pub;              /* what the program reads: first */
    BPTR tin, tout;
    struct MsgPort *conport, *rport;
    struct DosPacket *wpkt;      /* AllocDosObject's: the packet itself */
    int wpending, tsusp;
    /* the terminal has gone: CTerm closing a tab or quitting answers every
     * WAIT_CHAR "ready" and every Read 0 bytes, for ever. Seen once, the
     * program gets TE_END and is told so again on every wait (a program
     * that kept waiting would spin, and hold CTerm open: it has the
     * window). The same fix as Maki 263def2. */
    int ended;
    /* output */
    int olen;
    UBYTE obuf[4096];
    /* the grid: what should be shown, and what the terminal shows */
    Cell *want, *shown;          /* Rows x Cols each */
    int gw, gh;                  /* their size */
    int curx, cury, curfg, curbg, curat;
    int cux, cuy, cuon, cushown;
    /* input */
    int evh, evt;
    TuiEv evq[EVQ];
    int ilen;
    UBYTE ibuf[256];
    UBYTE *pastebuf, *pastegive; /* a paste being gathered; the one handed out */
    long pastelen, pastecap;
    int inpaste;
} Priv;

#define P(t) ((Priv *)(t))

/* ---- output ---------------------------------------------------------- */

static void oflush(Priv *p)
{
    if (p->olen && p->tout) Write(p->tout, p->obuf, p->olen);
    p->olen = 0;
}

static void out(Priv *p, const char *s, int n)
{
    if (p->olen + n > (int)sizeof(p->obuf)) oflush(p);
    if (n > (int)sizeof(p->obuf)) {
        Write(p->tout, (APTR)s, n);
        return;
    }
    memcpy(p->obuf + p->olen, s, n);
    p->olen += n;
}

static void outs(Priv *p, const char *s)
{
    out(p, s, (int)strlen(s));
}

static void outnum(Priv *p, int v)
{
    char b[12];
    int i = sizeof(b);
    if (v < 0) v = 0;
    do b[--i] = (char)('0' + v % 10); while ((v /= 10) && i);
    out(p, b + i, (int)sizeof(b) - i);
}

/* ---- the grid --------------------------------------------------------- */

static void gridfree(Priv *p)
{
    if (p->want) FreeVec(p->want);
    if (p->shown) FreeVec(p->shown);
    p->want = p->shown = NULL;
    p->gw = p->gh = 0;
}

static int gridalloc(Priv *p)
{
    int i, n;
    if (p->gw == p->pub.Cols && p->gh == p->pub.Rows && p->want) return 1;
    gridfree(p);
    n = p->pub.Cols * p->pub.Rows;
    p->want = AllocVec(n * sizeof(Cell), MEMF_ANY);
    p->shown = AllocVec(n * sizeof(Cell), MEMF_ANY);
    if (!p->want || !p->shown) {
        gridfree(p);
        return 0;
    }
    p->gw = p->pub.Cols;
    p->gh = p->pub.Rows;
    for (i = 0; i < n; i++) {
        p->want[i].c = ' ';
        p->want[i].fg = 7;
        p->want[i].bg = TC_DEFAULT;
        p->want[i].at = 0;
        p->shown[i] = p->want[i];
        p->shown[i].c = 0xFFFFFFFF;     /* unknown: sent on the next flush */
    }
    return 1;
}

static void forget(Priv *p)
{
    p->curx = p->cury = p->curfg = p->curbg = p->curat = -1;
    p->cushown = -1;
}

void TuiRedraw(struct Tui *t)
{
    Priv *p = P(t);
    int i, n;
    if (!p) return;
    n = p->gw * p->gh;
    for (i = 0; i < n; i++) p->shown[i].c = 0xFFFFFFFF;
    forget(p);
    if (!p->tsusp) outs(p, "\x1b[0m\x1b[H\x1b[J");
}

/* Latin Extended-A (U+0100-017F) without its accents: Czech, Polish,
 * Turkish... names still read in a Latin-1 console */
static const char latinA[] =
    "AaAaAaCcCcCcCcDd" "DdEeEeEeEeEeGgGg" "GgGgHhHhIiIiIiIi" "IiIiJjKkkLlLlLlL"
    "lLlNnNnNnnNnOoOo" "OoOoRrRrRrSsSsSs" "SsTtTtTtUuUuUuUu" "UuUuWwYyYZzZzZzs";

/* a character beyond Latin-1, for a terminal that has only Latin-1 (a
 * plain console): the nearest it has, else '?' */
static ULONG fallback(ULONG c)
{
    if (c >= 0x100 && c < 0x180) return (UBYTE)latinA[c - 0x100];
    if (c >= 0x2500 && c < 0x2580) {            /* box lines */
        switch (c) {
        case 0x2500: case 0x2501: case 0x2504: case 0x2505: case 0x2508: case 0x2509:
        case 0x254C: case 0x254D: case 0x2550: case 0x2574: case 0x2576: case 0x2578:
        case 0x257A: case 0x257C: case 0x257E:
            return '-';
        case 0x2502: case 0x2503: case 0x2506: case 0x2507: case 0x250A: case 0x250B:
        case 0x254E: case 0x254F: case 0x2551: case 0x2575: case 0x2577: case 0x2579:
        case 0x257B: case 0x257D: case 0x257F:
            return '|';
        case 0x2571: return '/';
        case 0x2572: return '\\';
        case 0x2573: return 'X';
        }
        return '+';
    }
    if (c >= 0x2580 && c < 0x25A0) {            /* blocks and shades */
        if (c == 0x2588) return FB_REV | ' ';
        if (c == 0x2591) return ':';
        if (c == 0x2592 || c == 0x2593) return '#';
        if (c == 0x2580 || c == 0x2594) return '"';
        if (c == 0x2584 || c == 0x2581 || c == 0x2582 || c == 0x2583) return '_';
        return '#';
    }
    if (c >= 0x2800 && c < 0x2900) return c == 0x2800 ? ' ' : ':';  /* braille */
    switch (c) {
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2015:
    case 0x2212: case 0x2043:
        return '-';
    case 0x2018: case 0x2019: case 0x201A: case 0x201B: case 0x2032:
        return '\'';
    case 0x201C: case 0x201D: case 0x201E: case 0x201F: case 0x2033:
        return '"';
    case 0x2039: case 0x2190: case 0x25C0: case 0x25C2: case 0x25C4: case 0x2264:
        return '<';
    case 0x203A: case 0x2192: case 0x25B6: case 0x25B8: case 0x25BA: case 0x2265:
    case 0x279C: case 0x27A4:
        return '>';
    case 0x2191: case 0x25B2: case 0x25B4: return '^';
    case 0x2193: case 0x25BC: case 0x25BE: return 'v';
    case 0x2194: case 0x2195: return '+';
    case 0x2022: case 0x2219: case 0x22C5: case 0x2027: case 0x25E6: return 0xB7;
    case 0x2026: return '.';
    case 0x25CF: case 0x25CB: case 0x25C9: case 0x25EF: return 'o';
    case 0x25A0: case 0x25A1: case 0x25AA: case 0x25AB: return '#';
    case 0x2605: case 0x2606: case 0x2731: case 0x273B: return '*';
    case 0x2713: case 0x2714: case 0x2611: return 'v';
    case 0x2717: case 0x2718: case 0x2612: return 'x';
    case 0x20AC: return 'E';
    case 0x2122: return 'T';
    case 0x2248: return '~';
    case 0x2260: return '#';
    case 0x221E: return '8';
    case 0x00A0: return ' ';
    case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004: case 0x2005:
    case 0x2006: case 0x2007: case 0x2008: case 0x2009: case 0x200A: case 0x202F:
    case 0x205F: case 0x3000:
        return ' ';
    }
    return '?';
}

/* a code point a cell can show: wide (CJK, emoji) and combining marks
 * would move everything after them, controls would steer the terminal;
 * beyond Latin-1 without UTF-8: the nearest (fallback). FB_REV may be
 * or'ed in. */
static ULONG cellable(Priv *p, ULONG c)
{
    if (c < 32 || (c >= 0x7F && c < 0xA0)) return '?';
    if ((c >= 0x300 && c < 0x370) || (c >= 0x1100 && c < 0x1160) ||
        (c >= 0x2E80 && c < 0xA4D0) || (c >= 0xAC00 && c < 0xD7A4) ||
        (c >= 0xF900 && c < 0xFB00) || (c >= 0xFE00 && c < 0xFE50) ||
        (c >= 0xFF00 && c < 0xFF61) || (c >= 0xFFE0 && c < 0xFFE7) ||
        (c >= 0xD800 && c < 0xE000) || c >= 0x1F000 || c == 0x200B ||
        c == 0x200D || c == 0xFEFF)
        return c == 0x3000 && !p->pub.UTF8 ? ' ' : '?';
    if (!p->pub.UTF8 && c > 0xFF) return fallback(c);
    return c;
}

/* one cell (col and row on the grid) */
static void setcell(Priv *p, Cell *q, ULONG c, LONG fg, LONG bg, LONG attr)
{
    c = cellable(p, c);
    if (c & FB_REV) {
        c &= ~FB_REV;
        attr ^= TA_REV;
    }
    q->c = c;
    q->fg = (ULONG)fg;
    q->bg = (ULONG)bg;
    q->at = (UBYTE)attr;
}

void TuiPutC(struct Tui *t, LONG col, LONG row, ULONG c, LONG fg, LONG bg, LONG attr)
{
    Priv *p = P(t);
    if (!p || col < 0 || row < 0 || col >= p->gw || row >= p->gh) return;
    setcell(p, &p->want[row * p->gw + col], c, fg, bg, attr);
}

/* s as UTF-8 (len < 0: up to its 0), one cell a character; a byte that
 * is not good UTF-8 counts as Latin-1, so Latin-1 text still shows.
 * Returns how many cells the text takes (also what fell off the edges) */
LONG TuiPutUTF8(struct Tui *t, LONG col, LONG row, CONST_STRPTR s, LONG len, LONG fg, LONG bg, LONG attr)
{
    Priv *p = P(t);
    const UBYTE *b = (const UBYTE *)s, *e;
    LONG n = 0;
    if (!s) return 0;
    if (len < 0) len = strlen((const char *)s);
    e = b + len;
    while (b < e) {
        ULONG c = *b++;
        if (c >= 0xC2 && c <= 0xF4) {
            int k = c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3, i;
            ULONG u = c & (0x3F >> k);
            for (i = 0; i < k && b + i < e && (b[i] & 0xC0) == 0x80; i++)
                u = (u << 6) | (b[i] & 0x3F);
            /* whole, and not too long a way to write a smaller one */
            if (i == k && u >= (k == 1 ? 0x80UL : k == 2 ? 0x800UL : 0x10000UL) && u < 0x110000) {
                c = u;
                b += k;
            }
        }
        if (p && row >= 0 && row < p->gh && col + n >= 0 && col + n < p->gw)
            setcell(p, &p->want[row * p->gw + col + n], c, fg, bg, attr);
        n++;
    }
    return n;
}

/* s as Latin-1 bytes (a program with UTF-8 text decodes it itself and
 * calls TuiPutC). TuiPut and TuiFill write the cells directly: a full
 * screen drawn through TuiPutC cost top a fifth of a 14 MHz 68020 every
 * second. */
void TuiPut(struct Tui *t, LONG col, LONG row, CONST_STRPTR s, LONG len, LONG fg, LONG bg, LONG attr)
{
    Priv *p = P(t);
    Cell *q;
    int i;
    if (!p || row < 0 || row >= p->gh) return;
    if (col < 0) { s -= col; len += col; col = 0; }
    if (len > p->gw - col) len = p->gw - col;
    q = &p->want[row * p->gw + col];
    for (i = 0; i < len; i++, q++) {
        unsigned char c = s[i];
        q->c = c >= 32 && c < 0x7F ? c : cellable(p, c);   /* never FB_REV */
        q->fg = (ULONG)fg;
        q->bg = (ULONG)bg;
        q->at = (UBYTE)attr;
    }
}

void TuiFill(struct Tui *t, LONG col, LONG row, LONG w, ULONG c, LONG fg, LONG bg, LONG attr)
{
    Priv *p = P(t);
    Cell *q;
    int i;
    if (!p || row < 0 || row >= p->gh) return;
    if (col < 0) { w += col; col = 0; }
    if (w > p->gw - col) w = p->gw - col;
    c = cellable(p, c);
    if (c & FB_REV) {
        c &= ~FB_REV;
        attr ^= TA_REV;
    }
    q = &p->want[row * p->gw + col];
    for (i = 0; i < w; i++, q++) {
        q->c = c;
        q->fg = (ULONG)fg;
        q->bg = (ULONG)bg;
        q->at = (UBYTE)attr;
    }
}

void TuiClear(struct Tui *t)
{
    Priv *p = P(t);
    int r;
    if (!p) return;
    for (r = 0; r < p->gh; r++) TuiFill(t, 0, r, p->gw, ' ', 7, TC_DEFAULT, 0);
}

void TuiCursor(struct Tui *t, LONG col, LONG row, LONG on)
{
    Priv *p = P(t);
    if (!p) return;
    p->cux = col;
    p->cuy = row;
    p->cuon = on;
}

/* a plain console's pens are its screen's: Workbench's (0 grey, 1 black,
 * 2 white, 3 blue) - the ANSI sixteen as near as they go */
static const UBYTE wbpen[17] = { 1, 1, 1, 1, 3, 1, 3, 2, 1, 1, 1, 2, 3, 1, 3, 2, 0 };

/* one of the 256 (TC_X) as the nearest of the sixteen */
static int to16(int x)
{
    int r, g, b, hi;
    if (x < 16) return x;
    if (x >= 232) { x -= 232; return x < 6 ? 0 : x < 14 ? 8 : x < 20 ? 7 : 15; }
    x -= 16;
    r = x / 36; g = x / 6 % 6; b = x % 6;
    hi = r > g ? r : g;
    if (b > hi) hi = b;
    if (!hi) return 0;
    x = (r * 2 >= hi && r >= 2 ? 1 : 0) | (g * 2 >= hi && g >= 2 ? 2 : 0) | (b * 2 >= hi && b >= 2 ? 4 : 0);
    if (!x) return 8;
    return hi >= 4 ? x + 8 : x;
}

/* a 24-bit colour (TC_RGB) as the nearest of the 256: the 6 x 6 x 6
 * cube or the grey ramp, whichever is closer */
static int rgbto256(int c)
{
    static const UBYTE lv[6] = { 0, 95, 135, 175, 215, 255 };
    int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255, i, ri, gi, bi, y, gy, dc, dg;
    ri = r < 48 ? 0 : r < 115 ? 1 : (r - 35) / 40;
    gi = g < 48 ? 0 : g < 115 ? 1 : (g - 35) / 40;
    bi = b < 48 ? 0 : b < 115 ? 1 : (b - 35) / 40;
    y = (r + g + b) / 3;
    i = y > 238 ? 23 : y < 8 ? 0 : (y - 3) / 10;
    gy = 8 + 10 * i;
    dc = (r - lv[ri]) * (r - lv[ri]) + (g - lv[gi]) * (g - lv[gi]) + (b - lv[bi]) * (b - lv[bi]);
    dg = (r - gy) * (r - gy) + (g - gy) * (g - gy) + (b - gy) * (b - gy);
    return dg < dc ? 232 + i : 16 + 36 * ri + 6 * gi + bi;
}

/* any colour as one of the sixteen (a plain console) */
static int to16any(int c)
{
    if (c & 0x1000000) return to16(rgbto256(c));
    if (c & 0x100) return to16(c & 0xFF);
    return c & 15;
}

static void sgrcol(Priv *p, int c, int base)
{
    if (c == TC_DEFAULT) { outnum(p, base + 9); return; }
    if (c & 0x1000000) {            /* CTerm: 24-bit */
        outnum(p, base + 8);
        outs(p, ";2;");
        outnum(p, (c >> 16) & 255);
        out(p, ";", 1);
        outnum(p, (c >> 8) & 255);
        out(p, ";", 1);
        outnum(p, c & 255);
        return;
    }
    if (c & 0x100) {                /* CTerm: the 256 */
        outnum(p, base + 8);
        outs(p, ";5;");
        outnum(p, c & 0xFF);
        return;
    }
    if (c < 8) outnum(p, base + c);
    else outnum(p, base + 60 + c - 8);
}

static void sgr(Priv *p, int fg, int bg, int at)
{
    if (fg == p->curfg && bg == p->curbg && at == p->curat) return;
    p->curfg = fg;
    p->curbg = bg;
    p->curat = at;
    outs(p, "\x1b[0");
    if (at & TA_BOLD) outs(p, ";1");
    if (at & TA_UNDER) outs(p, ";4");
    if (at & TA_REV) outs(p, ";7");
    if (at & TA_ITAL) outs(p, ";3");
    if (p->pub.CTerm) {
        if (at & TA_STRIKE) outs(p, ";9");
        if (at & TA_DIM) outs(p, ";2");
    } else {
        fg = fg == TC_DEFAULT ? 1 : wbpen[to16any(fg)];
        bg = bg == TC_DEFAULT ? TC_DEFAULT : wbpen[to16any(bg)];
    }
    out(p, ";", 1);
    sgrcol(p, fg, 30);
    out(p, ";", 1);
    sgrcol(p, bg, 40);
    out(p, "m", 1);
}

static void moveto(Priv *p, int col, int row)
{
    if (col == p->curx && row == p->cury) return;
    outs(p, "\x1b[");
    outnum(p, row + 1);
    out(p, ";", 1);
    outnum(p, col + 1);
    out(p, "H", 1);
    p->curx = col;
    p->cury = row;
}

static void putch(Priv *p, ULONG c)
{
    char b[4];
    if (c < 0x80 || !p->pub.UTF8) {
        b[0] = (char)c;
        out(p, b, 1);
    } else if (c < 0x800) {
        b[0] = (char)(0xC0 | (c >> 6));
        b[1] = (char)(0x80 | (c & 0x3F));
        out(p, b, 2);
    } else if (c < 0x10000) {
        b[0] = (char)(0xE0 | (c >> 12));
        b[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        b[2] = (char)(0x80 | (c & 0x3F));
        out(p, b, 3);
    } else {
        b[0] = (char)(0xF0 | (c >> 18));
        b[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        b[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        b[3] = (char)(0x80 | (c & 0x3F));
        out(p, b, 4);
    }
}

static void cursorvis(Priv *p, int on)
{
    if (on == p->cushown) return;
    if (p->pub.CTerm) outs(p, on ? "\x1b[?25h" : "\x1b[?25l");
    else outs(p, on ? "\x1b[ p" : "\x1b[0 p");
    p->cushown = on;
}

void TuiFlush(struct Tui *t)
{
    Priv *p = P(t);
    int r, c;
    if (!p || p->tsusp || !p->want) return;
    for (r = 0; r < p->gh; r++) {
        Cell *w = p->want + r * p->gw, *s = p->shown + r * p->gw;
        for (c = 0; c < p->gw; c++) {
            if (w[c].c == s[c].c && w[c].fg == s[c].fg && w[c].bg == s[c].bg && w[c].at == s[c].at)
                continue;
            if (!p->pub.CTerm && r == p->gh - 1 && c == p->gw - 1) {
                s[c] = w[c];                /* a console that would scroll there */
                continue;
            }
            if (p->cushown) cursorvis(p, 0);    /* no cursor jumping about while drawing */
            moveto(p, c, r);
            sgr(p, w[c].fg, w[c].bg, w[c].at);
            putch(p, w[c].c);
            s[c] = w[c];
            p->curx++;
            if (p->curx >= p->gw) p->curx = -1;  /* the wrap is not ours to guess */
        }
    }
    if (p->cuon && p->cux >= 0 && p->cuy >= 0 && p->cux < p->gw && p->cuy < p->gh) {
        moveto(p, p->cux, p->cuy);
        cursorvis(p, 1);
    } else
        cursorvis(p, 0);
    oflush(p);
}

/* cells the terminal may show otherwise now (a picture lies over them):
 * sent again on the next flush */
void TuiDirty(struct Tui *t, LONG col, LONG row, LONG w, LONG h)
{
    Priv *p = P(t);
    int r, c;
    if (!p || !p->want) return;
    for (r = row; r < row + h && r < p->gh; r++)
        for (c = col; c < col + w && c < p->gw; c++)
            if (r >= 0 && c >= 0) p->shown[r * p->gw + c].c = 0xFFFFFFFF;
}

/* bytes straight to the terminal at a cell (a picture), after a flush:
 * where the cursor is and the colours are forgotten */
void TuiRaw(struct Tui *t, LONG col, LONG row, CONST_STRPTR s, LONG n)
{
    Priv *p = P(t);
    if (!p || p->tsusp) return;
    cursorvis(p, 0);
    moveto(p, col, row);
    sgr(p, TC_DEFAULT, TC_DEFAULT, 0);
    oflush(p);
    if (n > 0 && p->tout) Write(p->tout, (APTR)s, n);
    forget(p);
}

void TuiTitle(struct Tui *t, CONST_STRPTR title)
{
    Priv *p = P(t);
    if (!p || !p->pub.CTerm) return;
    outs(p, "\x1b]2;");
    outs(p, (const char *)title);
    out(p, "\x07", 1);
}

void TuiBeep(struct Tui *t)
{
    Priv *p = P(t);
    if (!p) return;
    out(p, "\x07", 1);
    oflush(p);
}

ULONG TuiBox(struct Tui *t, LONG which)
{
    static const ULONG uni[] = { 0x2500, 0x2502, 0x256D, 0x256E, 0x2570, 0x256F, 0x25B8, 0x00B7 };
    static const char asc[] = "-|++++>.";
    if (which < 0 || which > BX_DOT) return '?';
    if (t && t->CTerm && t->UTF8) return uni[which];
    if (which == BX_DOT) return 0xB7;   /* Latin-1 has the middle dot */
    return (unsigned char)asc[which];
}

/* ---- input ------------------------------------------------------------ */

static TuiEv *push(Priv *p, int type, int key)
{
    int n = (p->evt + 1) % EVQ;
    TuiEv *e;
    if (n == p->evh) return NULL;       /* full: one too many is dropped */
    e = &p->evq[p->evt];
    memset(e, 0, sizeof(*e));
    e->type = type;
    e->key = key;
    CurrentTime(&e->secs, &e->mics);
    p->evt = n;
    return e;
}

static void pushmouse(Priv *p, int act, int col, int row, int mods)
{
    TuiEv *e = push(p, TE_MOUSE, 0);
    if (!e) return;
    e->mact = act;
    e->col = col;
    e->row = row;
    e->mods = mods;
}

static void setsize(Priv *p, int rows, int cols)
{
    if (rows < 3 || cols < 10) return;
    if (cols > MAXW) cols = MAXW;
    if (rows > MAXH) rows = MAXH;
    if (rows == p->pub.Rows && cols == p->pub.Cols) return;
    p->pub.Rows = rows;
    p->pub.Cols = cols;
    push(p, TE_RESIZE, 0);
}

static void pasteadd(Priv *p, const UBYTE *b, long n)
{
    if (p->pastelen + n > p->pastecap) {
        long cap = (p->pastelen + n) * 2 + 1024;
        UBYTE *q = AllocVec(cap, MEMF_ANY);
        if (!q) return;                 /* no memory: the rest is lost */
        if (p->pastebuf) {
            CopyMem(p->pastebuf, q, p->pastelen);
            FreeVec(p->pastebuf);
        }
        p->pastebuf = q;
        p->pastecap = cap;
    }
    CopyMem((APTR)b, p->pastebuf + p->pastelen, n);
    p->pastelen += n;
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
static void csi(Priv *p, const int *a, int na, int priv, int inter, int f, int fromamiga)
{
    int a0 = na > 0 ? a[0] : 0, m = na > 1 ? mods(a[1]) : 0;
    if (priv == '<' && (f == 'M' || f == 'm')) {        /* SGR mouse */
        int b = a0, x = na > 1 ? a[1] - 1 : 0, y = na > 2 ? a[2] - 1 : 0, mm = 0;
        if (b & 4) mm |= TM_SHIFT;
        if (b & 8) mm |= TM_ALT;
        if (b & 16) mm |= TM_CTRL;
        if (b & 64) pushmouse(p, (b & 1) ? TB_WHEELDOWN : TB_WHEELUP, x, y, mm);
        else if (f == 'm') pushmouse(p, TB_RELEASE, x, y, mm);
        else if (b & 32) pushmouse(p, TB_DRAG, x, y, mm);
        else if ((b & 3) == 0) pushmouse(p, TB_PRESS, x, y, mm);
        return;
    }
    if (inter == ' ') {
        if (f == 'r' && na >= 4) setsize(p, a[2], a[3]);   /* window bounds */
        else if (f == 'A') push(p, TE_KEY, K_LEFT | TM_SHIFT);   /* Amiga */
        else if (f == '@') push(p, TE_KEY, K_RIGHT | TM_SHIFT);
        return;
    }
    if (priv == '?') {
        if (f == '~') push(p, TE_KEY, K_HELP);
        return;
    }
    if (priv) return;
    switch (f) {
    case 'A': push(p, TE_KEY, K_UP | m); break;
    case 'B': push(p, TE_KEY, K_DOWN | m); break;
    case 'C': push(p, TE_KEY, K_RIGHT | m); break;
    case 'D': push(p, TE_KEY, K_LEFT | m); break;
    case 'H': push(p, TE_KEY, K_HOME | m); break;
    case 'F': push(p, TE_KEY, K_END | m); break;
    case 'T': push(p, TE_KEY, K_UP | TM_SHIFT); break;      /* Amiga Shift+Up */
    case 'S': push(p, TE_KEY, K_DOWN | TM_SHIFT); break;    /* Amiga Shift+Down */
    case 'Z': push(p, TE_KEY, K_TAB | TM_SHIFT); break;
    case '~':
        if (fromamiga) {
            if (na && a0 <= 9) push(p, TE_KEY, K_F1 + a0);
            else if (a0 >= 10 && a0 <= 19) push(p, TE_KEY, (K_F1 + a0 - 10) | TM_SHIFT);
            break;
        }
        switch (a0) {
        case 1: case 7: push(p, TE_KEY, K_HOME | m); break;
        case 2: push(p, TE_KEY, K_INS | m); break;
        case 3: push(p, TE_KEY, K_DEL | m); break;
        case 4: case 8: push(p, TE_KEY, K_END | m); break;
        case 5: push(p, TE_KEY, K_PGUP | m); break;
        case 6: push(p, TE_KEY, K_PGDN | m); break;
        case 11: case 12: case 13: case 14: case 15:
            push(p, TE_KEY, (K_F1 + a0 - 11) | m); break;
        case 17: case 18: case 19: case 20: case 21:
            push(p, TE_KEY, (K_F1 + 5 + a0 - 17) | m); break;
        case 23: case 24: push(p, TE_KEY, (K_F1 + 10 + a0 - 23) | m); break;
        case 200: p->inpaste = 1; p->pastelen = 0; break;
        }
        break;
    case '|':                       /* a raw input event: 12 is a resize */
        if (a0 == 12) {
            outs(p, "\x1b[0 q");
            oflush(p);
        }
        break;
    case 't':
        if (a0 == 8 && na >= 3) setsize(p, a[1], a[2]);
        else if (a0 == 6 && na >= 3) {
            p->pub.CellH = a[1];
            p->pub.CellW = a[2];
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
static void parse(Priv *p, int final)
{
    UBYTE *ib = p->ibuf;
    int i = 0, ilen = p->ilen;
    while (i < ilen) {
        UBYTE c = ib[i];
        int cs = -1, fromamiga = 0;
        if (p->inpaste) {
            /* everything up to ESC [ 201 ~ is the paste's */
            int j;
            for (j = i; j < ilen; j++)
                if (ib[j] == 0x1B) break;
            if (j > i) pasteadd(p, ib + i, j - i);
            i = j;
            if (i >= ilen) break;
            if (ilen - i < 6) {
                if (!final) break;
                pasteadd(p, ib + i, ilen - i);
                i = ilen;
                break;
            }
            if (!memcmp(ib + i, "\x1b[201~", 6)) {
                TuiEv *e;
                p->inpaste = 0;
                if (p->pastegive) FreeVec(p->pastegive);
                p->pastegive = p->pastebuf;
                p->pastebuf = NULL;
                if ((e = push(p, TE_PASTE, 0)) != NULL) {
                    e->text = p->pastegive;
                    e->len = p->pastelen;
                }
                p->pastelen = p->pastecap = 0;
                i += 6;
            } else {
                pasteadd(p, ib + i, 1);
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
                push(p, TE_KEY, K_ESC);
                i++;
                continue;
            }
            if (ib[i + 1] == '[') cs = i + 2;
            else if (ib[i + 1] == 'O') {    /* SS3 */
                int k = 0;
                if (i + 2 >= ilen) {
                    if (!final) break;
                    push(p, TE_KEY, K_ESC);
                    i += 2;
                    continue;
                }
                switch (ib[i + 2]) {
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
                if (k) push(p, TE_KEY, k);
                i += 3;
                continue;
            } else {
                /* ESC and a key: Alt+key as xterm sends it (meta) */
                UBYTE n = ib[i + 1];
                if (n >= 32 && n < 127) push(p, TE_KEY, TM_ALT | n);
                else if (n == 0x7F || n == 8) push(p, TE_KEY, TM_ALT | K_BS);
                else push(p, TE_KEY, K_ESC), i--;
                i += 2;
                continue;
            }
        }
        if (cs >= 0) {
            int a[8], na = 0, priv = 0, inter = 0, j = cs, have = 0, done = 0;
            memset(a, 0, sizeof(a));
            if (j < ilen && (ib[j] == '?' || ib[j] == '<' || ib[j] == '>' || ib[j] == '=')) priv = ib[j++];
            for (; j < ilen; j++) {
                UBYTE b = ib[j];
                if (b >= '0' && b <= '9') {
                    if (na < 8) a[na] = a[na] * 10 + (b - '0');
                    have = 1;
                } else if (b == ';') {
                    if (na < 8) na++;
                    have = 1;
                } else if (b >= 0x20 && b <= 0x2F) inter = b;
                else if (b >= 0x40 && b <= 0x7E) {
                    if (have && na < 8) na++;
                    csi(p, a, na, priv, inter, b, fromamiga);
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
                for (k = 1; k < n && i + k < ilen && (ib[i + k] & 0xC0) == 0x80; k++) ;
                if (k == n) {
                    ULONG u = c & (0x7F >> n);
                    for (k = 1; k < n; k++) u = (u << 6) | (ib[i + k] & 0x3F);
                    push(p, TE_KEY, (int)u);
                    i += n;
                    continue;
                }
            }
            push(p, TE_KEY, c);             /* Latin-1, as the keymap gives it */
            i++;
            continue;
        }
        switch (c) {
        case 8: push(p, TE_KEY, K_BS); break;
        case 9: push(p, TE_KEY, K_TAB); break;
        case 10: case 13: push(p, TE_KEY, K_ENTER); break;
        case 0x7F: push(p, TE_KEY, K_DEL); break;
        default:
            if (c < 32) push(p, TE_KEY, TM_CTRL | (c + 'a' - 1));
            else push(p, TE_KEY, c);
        }
        i++;
    }
    if (i > 0) {
        memmove(ib, ib + i, ilen - i);
        p->ilen = ilen - i;
    }
}

/* Read what the console has into ibuf; 0 bytes after "a key is there":
 * the input ended (CTerm's tab closing gives that for ever) */
static int readsome(Priv *p)
{
    LONG n = Read(p->tin, p->ibuf + p->ilen, sizeof(p->ibuf) - p->ilen);
    if (n <= 0) {
        p->ended = 1;
        return 0;
    }
    p->ilen += (int)n;
    return 1;
}

/* what the console has now (nothing waiting: nothing is read) */
static void readnow(Priv *p)
{
    int guard = 16;
    while (guard-- && WaitForChar(p->tin, 0)) {
        if (!readsome(p)) break;
        parse(p, 0);
        if (p->ilen >= (int)sizeof(p->ibuf) - 8) {  /* one long run (a paste): take it as it is */
            parse(p, 1);
            if (p->ilen >= (int)sizeof(p->ibuf) - 8) p->ilen = 0;
        }
    }
    if (p->ilen) {
        /* a lone Esc, or a sequence cut between reads: a moment for the rest */
        if (WaitForChar(p->tin, 40000)) {
            readsome(p);
            parse(p, 0);
        }
        if (p->ilen && !p->inpaste) parse(p, 1);
    }
}

static void arm(Priv *p, long us)
{
    if (p->wpending || p->tsusp) return;
    p->wpkt->dp_Type = ACTION_WAIT_CHAR;
    p->wpkt->dp_Arg1 = us > 0 && us < WAITUS ? us : WAITUS;
    SendPkt(p->wpkt, p->conport, p->rport);
    p->wpending = 1;
}

/* the wait packet back (if out): what it says is taken; 1 = it came back */
static int collect(Priv *p, int block)
{
    int back = 0;
    while (p->wpending) {
        if (!GetMsg(p->rport)) {
            if (!block) return back;
            WaitPort(p->rport);
            continue;
        }
        p->wpending = 0;
        back = 1;
        if (p->wpkt->dp_Res1) readnow(p);
    }
    return back;
}

static void breaks(Priv *p, ULONG sig)
{
    /* Ctrl-C..F come as signals: keys like the others */
    if (sig & SIGBREAKF_CTRL_C) push(p, TE_KEY, TM_CTRL | 'c');
    if (sig & SIGBREAKF_CTRL_D) push(p, TE_KEY, TM_CTRL | 'd');
    if (sig & SIGBREAKF_CTRL_E) push(p, TE_KEY, TM_CTRL | 'e');
    if (sig & SIGBREAKF_CTRL_F) push(p, TE_KEY, TM_CTRL | 'f');
}

/* the next event, waiting up to us microseconds (-1: for ever, 0: not at
 * all); extra: more signals to wake for (*got says which) */
LONG TuiWait(struct Tui *t, TuiEv *e, LONG us, ULONG extra, ULONG *got)
{
    Priv *p = P(t);
    ULONG waited = 0;
    if (got) *got = 0;
    if (!p) {
        e->type = TE_END;
        return 1;
    }
    for (;;) {
        ULONG sig;
        breaks(p, SetSignal(0, BREAKS) & BREAKS);
        if (p->evh != p->evt) {
            *e = p->evq[p->evh];
            p->evh = (p->evh + 1) % EVQ;
            return 1;
        }
        if (p->ended) {                 /* every wait from now on: the end */
            memset(e, 0, sizeof(*e));
            e->type = TE_END;
            return 1;
        }
        collect(p, 0);
        if (p->evh != p->evt) continue;
        if (us == 0) {
            e->type = TE_NONE;
            return 0;
        }
        TuiFlush(t);
        arm(p, us);
        sig = Wait((1UL << p->rport->mp_SigBit) | BREAKS | extra);
        if (got) *got |= sig & extra;
        breaks(p, sig & BREAKS);
        if (sig & extra) {
            collect(p, 0);
            if (p->evh == p->evt) {
                e->type = TE_NONE;
                return 0;
            }
            continue;
        }
        if (collect(p, 0) && us > 0) {
            waited += WAITUS;
            if (p->evh == p->evt && (long)waited >= us) {
                e->type = TE_NONE;
                return 0;
            }
        }
    }
}

/* ---- opening and closing ------------------------------------------------ */

static void modes_on(Priv *p)
{
    outs(p, "\x1b[?1049h");         /* its own screen (CTerm; others ignore it) */
    outs(p, "\x1b[?7l\x1b[>1l");    /* no wrap, no scroll at the last cell */
    outs(p, "\x1b[12{");            /* resizes reported (raw event class 12) */
    if (p->pub.CTerm) {
        outs(p, "\x1b%G");          /* UTF-8 both ways: every character shows */
        p->pub.UTF8 = 1;
        outs(p, "\x1b[?1000h\x1b[?1002h\x1b[?1006h");   /* the mouse, with drags */
        outs(p, "\x1b[?2004h");     /* pastes marked (and modern keys: CTerm) */
    } else
        p->pub.UTF8 = 0;
    forget(p);
}

static void modes_off(Priv *p)
{
    outs(p, "\x1b[12}\x1b[0m\x1b[?7h\x1b[>1h");
    if (p->pub.CTerm) {
        outs(p, "\x1b[?2004l\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?25h");
        outs(p, "\x1b%@");          /* the Shell's characters again */
    } else
        outs(p, "\x1b[ p\x1b[H\x1b[J");     /* cursor on; no second screen: cleared */
    outs(p, "\x1b[?1049l");
    oflush(p);
}

static void readreplies(Priv *p, int tries, int *flag)
{
    while (tries-- > 0 && !*flag && !p->ended) {
        if (!WaitForChar(p->tin, 100000)) continue;
        if (!readsome(p)) return;
        parse(p, 0);
    }
}

/* the size: the Amiga window bounds report, else xterm's, else 80 x 24;
 * CTerm's cell in pixels too */
static void asksize(Priv *p)
{
    int r0 = p->pub.Rows, c0 = p->pub.Cols, h0 = p->pub.CellH;
    p->pub.Rows = p->pub.Cols = 0;
    outs(p, "\x1b[0 q");
    oflush(p);
    readreplies(p, 10, &p->pub.Rows);
    if (!p->pub.Rows) {
        outs(p, "\x1b[18t");
        oflush(p);
        readreplies(p, 5, &p->pub.Rows);
    }
    if (!p->pub.Rows) {
        p->pub.Rows = r0 > 0 ? r0 : 24;
        p->pub.Cols = c0 > 0 ? c0 : 80;
    }
    if (p->pub.CTerm) {
        outs(p, "\x1b[16t");
        oflush(p);
        p->pub.CellH = 0;
        readreplies(p, 5, &p->pub.CellH);
        if (!p->pub.CellH) p->pub.CellH = h0;
    }
    {   /* the size events pushed on the way are not news */
        int i, j = p->evh;
        for (i = p->evh; i != p->evt; i = (i + 1) % EVQ)
            if (p->evq[i].type != TE_RESIZE) {
                p->evq[j] = p->evq[i];
                j = (j + 1) % EVQ;
            }
        p->evt = j;
    }
}

/* an interactive console under in (0: Input()) and Output() */
LONG TuiPossible(BPTR in)
{
    struct FileHandle *fh;
    BPTR o = Output();
    if (!in) in = Input();
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

/* raw mode, own screen, size; keys from in (0: Input(); a program whose
 * Input() is a pipe gives the console opened as "*"). NULL: no terminal */
struct Tui *TuiOpen(BPTR in)
{
    Priv *p;
    if (!TuiPossible(in)) return NULL;
    if (!(p = AllocVec(sizeof(*p), MEMF_ANY | MEMF_CLEAR))) return NULL;
    p->tin = in ? in : Input();
    p->tout = Output();
    p->conport = ((struct FileHandle *)BADDR(p->tin))->fh_Type;
    p->pub.CTerm = iscterm(p->conport);
    p->pub.Cols = 80;
    p->pub.Rows = 24;
    forget(p);
    if (!(p->rport = CreateMsgPort())) goto fail;
    if (!(p->wpkt = AllocDosObject(DOS_STDPKT, NULL))) goto fail;
    SetSignal(0, BREAKS);
    SetMode(p->tin, 1);
    modes_on(p);
    asksize(p);
    if (!gridalloc(p)) {
        modes_off(p);
        SetMode(p->tin, 0);
        goto fail;
    }
    TuiRedraw(&p->pub);
    return &p->pub;
fail:
    if (p->wpkt) FreeDosObject(DOS_STDPKT, p->wpkt);
    if (p->rport) DeleteMsgPort(p->rport);
    FreeVec(p);
    return NULL;
}

/* everything back as it was */
void TuiClose(struct Tui *t)
{
    Priv *p = P(t);
    if (!p) return;
    collect(p, 1);                      /* answered within WAITUS */
    if (!p->tsusp) {
        modes_off(p);
        SetMode(p->tin, 0);
    }
    FreeDosObject(DOS_STDPKT, p->wpkt);
    DeleteMsgPort(p->rport);
    gridfree(p);
    if (p->pastebuf) FreeVec(p->pastebuf);
    if (p->pastegive) FreeVec(p->pastegive);
    FreeVec(p);
}

/* the terminal to a command for a while ... */
void TuiSuspend(struct Tui *t)
{
    Priv *p = P(t);
    if (!p || p->tsusp) return;
    collect(p, 1);
    p->tsusp = 1;
    modes_off(p);
    SetMode(p->tin, 0);
}

/* (suspended) after a command in the terminal: its output stays until a
 * key; msg says so (NULL: " a key goes back ") */
void TuiKeyAfter(struct Tui *t, CONST_STRPTR msg)
{
    Priv *p = P(t);
    char c;
    if (!p) return;
    if (!msg) msg = (CONST_STRPTR)" a key goes back ";
    Write(p->tout, "\n\x1b[7m", 5);
    Write(p->tout, (APTR)msg, strlen((const char *)msg));
    Write(p->tout, "\x1b[0m", 4);
    SetMode(p->tin, 1);
    if (Read(p->tin, &c, 1) <= 0) p->ended = 1;
    while (!p->ended && WaitForChar(p->tin, 20000))     /* the rest of a key's sequence */
        if (Read(p->tin, &c, 1) <= 0) p->ended = 1;
    Write(p->tout, "\r\x1b[K", 4);
    SetMode(p->tin, 0);
}

/* ... and back (all drawn again on the next flush) */
void TuiResume(struct Tui *t)
{
    Priv *p = P(t);
    if (!p || !p->tsusp) return;
    SetMode(p->tin, 1);
    p->tsusp = 0;
    modes_on(p);
    asksize(p);
    gridalloc(p);
    TuiRedraw(t);
}

/* after TE_RESIZE: the grid follows (the program draws everything
 * again); TRUE if it changed */
LONG TuiResized(struct Tui *t)
{
    Priv *p = P(t);
    if (!p || (p->gw == p->pub.Cols && p->gh == p->pub.Rows)) return 0;
    gridalloc(p);
    TuiRedraw(t);
    return 1;
}
