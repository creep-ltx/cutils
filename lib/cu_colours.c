/* cu_colours.c - kinds of files and their colours (cu.h), from CShell's ls.
 *
 * ENV:FileColours (else ENVARC:FileColours; both written with the defaults
 * when neither is there) is the family's file: ls, Ahiru and Maki read it.
 *   KIND CTERM,PLAIN   SGR numbers in CTerm and in a plain Shell window, - none
 *   .EXT KIND          a name ending in .ext is of that kind
 * Kinds: text drawer link program script archive picture sound source icon
 * hidden. A program is a name with no ending and the e bit (Ahiru's rule),
 * a script has the s bit, hidden the h bit, an icon ends in .info.
 *
 * Only exec and dos, no other part of CUtils: copy it (and the CU_K_ enum
 * and the four prototypes from cu.h) into another program as it is. */
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "cu.h"

static const char *const kindnames[CU_K_COUNT] = {
    "text", "drawer", "link", "program", "script", "archive", "picture", "sound",
    "source", "icon", "hidden"
};

#define MAXEXT 160
static char extname[MAXEXT][12];
static char extkind[MAXEXT];
static int next_;
static char sgr[2][CU_K_COUNT][20];

static int lc(int c)
{
    c &= 0xff;
    if ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7)) c += 32;
    return c;
}

static int ieq(const char *a, const char *b)
{
    while (*a && lc(*a) == lc(*b)) a++, b++;
    return !*a && !*b;
}

static const char deffile[] =
    "; File kinds and their colours, for csh's ls (and Ahiru and Maki, later).\n"
    "; KIND CTERM,PLAIN: SGR numbers in CTerm and in a plain Shell window; '-' = none.\n"
    "; In a CTerm Shell tab 30-37 are the Amiga's pens: its theme's colours are 90 grey\n"
    "; 91 red 92 green 93 yellow 94 blue 95 magenta 96 cyan 97 white. A plain window on\n"
    "; a stock Workbench has pens 30 grey 31 black 32 white 33 blue. 1 bold, 3 italic.\n"
    "; .EXT KIND puts a name ending in .ext into a kind (after the built-in list).\n"
    "drawer 94,33\n"
    "link 96,3\n"
    "program 92,1\n"
    "script 92,1\n"
    "archive 91,-\n"
    "picture 93,-\n"
    "sound 95,-\n"
    "source 96,-\n"
    "text -,-\n"
    "icon 90,32\n"
    "hidden 90,32\n";

static const struct { const char *exts; int kind; } builtin[] = {
    { "|lha|lzh|lzx|zip|dms|adf|hdf|iso|gz|tgz|tar|7z|rar|xz|bz2|lz|z|", CU_K_ARCHIVE },
    { "|iff|ilbm|lbm|pic|gif|jpg|jpeg|png|bmp|pcx|tga|ham|brush|br|anim|anm|ico|webp|", CU_K_PICTURE },
    { "|mod|med|mmd0|mmd1|mmd2|xm|s3m|it|8svx|svx|wav|mp3|aiff|aif|sid|ahx|thx|ogg|flac|bp|tfmx|okt|sng|", CU_K_SOUND },
    { "|e|c|h|s|asm|i|py|rexx|rx|cpp|hpp|bas|sh|m|amos|csh|", CU_K_SOURCE },
    { NULL, 0 }
};

static void addext(const char *e, int kind)
{
    int i;
    if (strlen(e) > 11) return;
    for (i = 0; i < next_; i++) if (ieq(extname[i], e)) { extkind[i] = kind; return; }
    if (next_ == MAXEXT) return;
    strcpy(extname[next_], e);
    extkind[next_++] = kind;
}

static void setsgr(char *d, const char *v, int n)
{
    if (n <= 0 || n > 14 || (n == 1 && *v == '-')) { d[0] = 0; return; }
    d[0] = 0x1b; d[1] = '[';
    memcpy(d + 2, v, n);
    d[n + 2] = 'm'; d[n + 3] = 0;
}

static void parsefile(char *t)
{
    char *line = t;
    while (line && *line) {
        char *nl = strchr(line, '\n'), *k, *v, *e;
        int i;
        if (nl) *nl = 0;
        k = line;
        while (*k == ' ' || *k == '\t') k++;
        if (*k && *k != ';' && *k != '#') {
            for (e = k; *e && *e != ' ' && *e != '\t'; e++) ;
            v = e;
            while (*v == ' ' || *v == '\t') v++;
            *e = 0;
            for (e = v; *e && *e != ' ' && *e != '\t' && *e != '\r'; e++) ;
            *e = 0;
            for (i = 0; i < CU_K_COUNT && !ieq(k[0] == '.' ? v : k, kindnames[i]); i++) ;
            if (i < CU_K_COUNT) {
                if (k[0] == '.') addext(k + 1, i);
                else {
                    char *comma = strchr(v, ',');
                    setsgr(sgr[0][i], v, comma ? comma - v : (int)strlen(v));
                    if (comma) setsgr(sgr[1][i], comma + 1, strlen(comma + 1));
                    else strcpy(sgr[1][i], sgr[0][i]);
                }
            }
        }
        line = nl ? nl + 1 : NULL;
    }
}

static char *readall(const char *path)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    char *t;
    LONG n;
    if (!fh) return NULL;
    Seek(fh, 0, OFFSET_END);
    n = Seek(fh, 0, OFFSET_BEGINNING);
    if (n < 0 || n > 32768 || !(t = AllocVec(n + 1, MEMF_ANY))) { Close(fh); return NULL; }
    n = Read(fh, t, n);
    Close(fh);
    t[n > 0 ? n : 0] = 0;
    return t;
}

void cu_colours_load(void)
{
    static char d[sizeof deffile];
    char *t;
    int i;
    for (i = 0; builtin[i].exts; i++) {
        const char *p = builtin[i].exts + 1;
        while (*p) {
            char e[12];
            int n = 0;
            while (*p && *p != '|' && n < 11) e[n++] = *p++;
            e[n] = 0;
            if (*p == '|') p++;
            addext(e, builtin[i].kind);
        }
    }
    memcpy(d, deffile, sizeof deffile);
    parsefile(d);
    if (!(t = readall("ENV:FileColours")) && !(t = readall("ENVARC:FileColours"))) {
        /* written with the defaults; no requester on a write-protected disk */
        struct Process *me = (struct Process *)FindTask(NULL);
        APTR win = me->pr_WindowPtr;
        BPTR fh;
        me->pr_WindowPtr = (APTR)-1;
        if ((fh = Open("ENVARC:FileColours", MODE_NEWFILE))) { Write(fh, (APTR)deffile, sizeof deffile - 1); Close(fh); }
        if ((fh = Open("ENV:FileColours", MODE_NEWFILE))) { Write(fh, (APTR)deffile, sizeof deffile - 1); Close(fh); }
        me->pr_WindowPtr = win;
        return;
    }
    parsefile(t);
    FreeVec(t);
}

int cu_kind(const char *name, LONG type, LONG prot)
{
    const char *dot = NULL, *p;
    int i;
    if (type == ST_SOFTLINK || type == ST_LINKDIR || type == ST_LINKFILE) return CU_K_LINK;
    if (type > 0) return CU_K_DRAWER;
    for (p = name; *p; p++) if (*p == '.') dot = p;
    if (dot && ieq(dot, ".info")) return CU_K_ICON;
    if (prot & FIBF_HIDDEN) return CU_K_HIDDEN;
    if (prot & FIBF_SCRIPT) return CU_K_SCRIPT;
    if (!strncmp(name, "mod.", 4) || !strncmp(name, "MOD.", 4)) return CU_K_SOUND;
    if (dot && dot[1]) {
        for (i = 0; i < next_; i++) if (ieq(extname[i], dot + 1)) return extkind[i];
        return CU_K_TEXT;
    }
    /* no ending and the e bit: a program, Ahiru's rule too. Reading each
     * file's first long word (a hunk header) was exact but tripled the time
     * of a drawer like C: (100 opens), slower than C:Dir */
    if (!(prot & FIBF_EXECUTE)) return CU_K_PROGRAM;
    return CU_K_TEXT;
}

const char *cu_sgr(int kind, int cterm)
{
    return kind >= 0 && kind < CU_K_COUNT ? sgr[cterm ? 0 : 1][kind] : "";
}

/* CTerm answers ACTION_DISK_INFO with the disk type 'CTRM' */
int cu_iscterm(BPTR fh)
{
    struct FileHandle *f = BADDR(fh);
    struct InfoData *id;
    int yes = 0;
    if (!f || !f->fh_Type) return 0;
    if ((id = AllocVec(sizeof *id, MEMF_PUBLIC | MEMF_CLEAR))) {
        if (DoPkt(f->fh_Type, ACTION_DISK_INFO, MKBADDR(id), 0, 0, 0, 0)) yes = id->id_DiskType == 0x4354524DUL;
        FreeVec(id);
    }
    return yes;
}
