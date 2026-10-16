# CUtils in C - design

Unix-style commands for AmigaOS 3.2, in C: the core utils your fingers know,
for csh pipes (`List | grep foo | wc -l`) and for the ROM Shell alike. Started
2026-10-14 (the user: "a C version of CUtils plus cat, grep, head, tail, wc,
find"). The Amiga E versions (ls cp mv mkdir rm) stay in AmigaTools/cutils;
this replaces them.

## Start here (a later session)

**Parallel sessions (set up 2026-10-15, the user's wish):** CTerm, csh and CUtils may each have a
session running at the same time. Stay in your own repo: CTerm's session owns `~/Projects/CTerm`,
csh's owns `~/Projects/CShell`, CUtils' owns `~/Projects/CUtils`; a change another one needs is a
note in its design.md or a message, never an edit there. Shared things:
- **FS-UAE on port 1240** (`~/Projects/CTerm/tests/files/uae.sh`) is used by all three: work in
  your own RAM: drawer (CUtils: `S=x tests/run.sh`), never `uae.sh stop`/restart it without
  asking the others (`ListAgents`, then a message); a crash there - tell the others before restarting.
- **The A1200** (Wasabi): message the others before installing, rebooting or restarting wasabid;
  quit test programs and say when the screen is free. Installing in C:/LIBS: needs the user's go.
- **Ahiru and Maki's FileColours** is on both lists: the csh session does it (in those repos), not CUtils.
- **Next for CUtils, in this order:** (1) the stale docs (design.md "not yet on the A1200" for
  the second batch, README.md: "tested in FS-UAE" without "installed", tar's "waits for
  lib/cz.c"); (2) the small mismatch list below; (3) xz and zstd packing for tar; (4) an installer
  and the Aminet release (ask the user first: the readme and the version); later btop, awk, an
  archive manager. sed, top and wget (2026-10-15) were never tried by hand on the A1200: worth a
  look over Wasabi (wget https needs ~4 MB free fast RAM).

**State 2026-10-15: 45 commands, all installed on the A1200** (C: and HELP:Man/english/CUtils,
every binary refreshed for the errout() fix); 45 test files green on FS-UAE port 1240 plus
wget-https.t on the 32 MB one (tests/uae030.sh). Next ideas (the user's list, 2026-10-15): use it a
while, then an installer + Aminet release, man pages for the rest of the family, the small
mismatch list below, xz/zstd packing, FileColours in Ahiru/Maki; later btop, awk, an archive
manager.

- Build: `make` (bebbo's gcc in `~/opt/amiga`), binaries in `bin/`.
- Test: start the private FS-UAE (`~/Projects/CTerm/tests/files/uae.sh start`, port
  1240), then `tests/run.sh [name...]`. Cases are `tests/cases/NAME.t` (format in
  run.sh's head), test files in `tests/files/`. `S=x tests/run.sh` uses RAM:cux /
  RAM:cutx, so several sessions can test at once. After a crash: `tests/uae-restart.sh`.
- State: see the table in README.md. **Installed on the A1200 2026-10-14** (the user asked):
  all 15 in C: with the p bit; the old ones kept as C:cp-e.bak, mv-e.bak, rm-e.bak,
  mkdir-e.bak (Amiga E) and C:ls-0.2.bak (CShell's ls). `wasabi put` needs `--force` for C:.
  C: is before SYS:System on its path, so `find` is ours, not the Workbench Find tool.
  **Second batch installed 2026-10-14** (sort uniq cut tr xargs diff which date basename
  dirname seq file stat ln chmod, touch 0.1b2): the Amiga's own kept as C:Sort-amiga,
  C:Which-amiga, C:Date-amiga. All 30 in C: now; 955 test cases green in FS-UAE.
  **man installed 2026-10-14** (the user's go): C:man, 31 pages in HELP:Man/english/CUtils/,
  and in S:User-Startup a `;BEGIN CUtils` block after CCon's: `Assign MAN: HELP:Man`
  (S:User-Startup.bak-man kept). tee and tail 0.1b2 there too. 970 test cases green.
  man 0.1b2 installed the same evening: pages now NAME.md (the .man ones deleted), roff read.
  ps, kill, free, uptime 0.1b1 installed 2026-10-14 with their pages; csh 0.1b8 passes non-%job
  kill targets to C:kill. 35 commands, 1004 test cases green.
  tar, gzip, gunzip, bzip2, bunzip2, unxz, unzstd 0.1b1 (2026-10-14): 42 commands, 1319
  cases green, tests/cz-pc 881 of 881 PC/68020 runs agree. bzip2's default level steps
  down to what fits in memory.
  sed 0.1b1 (2026-10-15, FS-UAE only, not on the A1200): GNU sed's language; tests/sed-pc
  compares it with GNU sed on Linux (417 listed cases + random s///, all agree) and the
  68020 build under vamos with the Linux build. lib/regex.c grew for it: RE_LONGEST (POSIX
  leftmost-longest), RE_MULTILINE, \b \B \< \> \` \'; fixed there: a search gave up when
  nothing could start at one place (`$` alone, `\bfoo`), {n,m} over 100 is an error (was a
  plain {), and the {n,m} copy is on the heap (nested groups overflowed a 4 KB stack).
  top 0.1b3 (2026-10-16): the classic top in the terminal (lib/tui.c); CPU% and TIME+ exact
  through exec's tc_Launch/tc_Switch hooks on every task (an EClock time per switch; probe
  experiments/exectap.c, Knowledge amiga/task-cpu-time.md). The hook code (74 bytes) is copied
  once per boot into a block that is never freed, behind the semaphore "CUtils top hooks 2",
  so a stale hook left in a dead task's memory can never call into an unloaded top; holding
  that semaphore = owning the hooks (a second top samples). The vertical-blank sampler (asm,
  ring in is_Data) paces the refreshes and covers tasks with hooks of their own.
  Tested: top.t 28 cases on 3.2.1; stress (experiments: ~/tmp/exectap/stress3.sh, one wasabi
  command at a time) on 3.1 and 3.2.1; experiments/hookscan lists tasks still hooked.
  -resident32 trap: a .text label declared in C as `extern char x[]` is reached through a4
  (DREL32) and is garbage; ask an asm helper for the address (top_stubat).
  wget 0.1b1 (2026-10-15, FS-UAE only): 43 commands, 1405 cases green (wget.t 67 on both
  emulators, wget-https.t 19 on the 030 one: `tests/wgettest.sh`).
- Added 2026-10-14, tested in FS-UAE only (not yet on the A1200): diff, which, date,
  basename, dirname, seq, file, stat, ln, chmod. `which` and `date` replace C:Which and
  C:Date (see the rule below); installing them means keeping the originals aside.
  diff's algorithm was also fuzzed on Linux against GNU diff (a stand-in for cu.h and
  the dos calls): the same unified output in 400 of 400 random pairs, valid patches and
  minimal edit counts everywhere; -i -w -b -B agree with GNU in 2800 runs.
- Backed up to the NAS: remote `nas` = /mnt/nas/home/Code/CUtils.git.

## The rules every command keeps

- **Unix flags, bundled**, parsed by `cu_opt()`: `ls -la`, `head -n5`, `head -5`,
  `--` ends flags, `-` alone is the input. `cmd ?` prints the usage line (as an
  AmigaDOS command should). No ReadArgs: muscle memory is the point.
- **Names and patterns**: every file argument goes through `cu_each()`: a name
  as typed is a name (even with `(` `[` in it), else an Amiga pattern; `*` is
  taken as `#?` (the ROM Shell hands `*` on as typed; csh expands `*` itself, or
  hands `#?` on when the line would be too long). Matches sorted naturally,
  case blind (file9 before file10).
- **Output**: through `cu_put*()`, gathered in 4 KB; messages through `cu_err()` /
  `cu_fault()`: `name: what: why` on the error stream (`pr_CES`, else Output), so
  `2>` / `*>` in csh separates them. Wording like the E family: lower case,
  the Amiga's own fault text for DOS errors.
- **Return codes**: 0 fine, 5 something skipped or not found (and said), 10
  bad arguments or a failure, 20 Ctrl-C. The worst one wins; one bad file
  never stops the rest. Ctrl-C checked between files and every few hundred lines.
- **Text**: `cu_open()`/`cu_line()` read 16 KB blocks; lines without copying
  in the common case, long lines grown on the heap; no line limit.
- **Pure**: built with `-resident32` (each start its own data), so `Resident
  C:grep PURE` works and csh pipes skip the disk. Link with `-m68020` (else the
  68000 libnix and a crash at start). `--gc-sections` also crashed them: the
  library is an archive instead, so a command links only the parts it uses.
- **Small**: dos.library and libnix's malloc only; no stdio, no printf.
- **Safe** (from the E family, cp mv rm): non-destructive by default (`-f` to
  replace), metadata (protection, date, filenote) kept like `Copy CLONE`, roots
  refused, links never followed into, trees walked with explicit lists on the
  heap (the Shell's stack is often 4 KB; `cu_stackleft()` for a check).
- **Names that clash with 3.2's C:** (Search, Type) are not used. Three are
  taken over on purpose (the user's decision, 2026-10-14): `sort`, `which` and
  `date` replace C:Sort, C:Which and C:Date, so each also takes the Amiga's
  ReadArgs template and behaves and prints as the Amiga's does (checked side by
  side with Sort 46.4, Which 45.2 and Date 47.2 in FS-UAE; details in their
  header comments). sort's Amiga mode: any of its keywords, or exactly two names
  and no -flags. Return codes differ only for bad arguments (10). Keep the
  originals as C:Sort-amiga, C:Which-amiga, C:Date-amiga when installing.
  (`find` meets `SYS:System/Find`, a Workbench tool: C: comes before System in
  the standard path, so find in C: wins; the tests put RAM:cu first with
  `Path ... HEAD`, also ahead of an older ls in C:.)
- **The ROM Shell's own characters**: `;` starts a comment, `>` `<` redirect
  anywhere on the line, a `+` at the end means "more lines follow" (they go to
  the command's input). find's -exec takes `";"` and `"+"` quoted, or the end
  of the line.
- Every command has a header comment that is its manual: usage, flags, return codes.

## Files

- `lib/cu.h` - the shared parts, each described there. `lib/cu.c` (output,
  messages, the command line, paths), `lib/cu_each.c` (patterns), `lib/cu_in.c`
  (reading), `lib/regex.c` (POSIX ERE as a Pike VM, from CShell; compiled once),
  `lib/cu_tree.c` (cp mv rm: lstat without following links, a drawer read whole,
  the file copier with metadata, the tree walkers on heap lists, the "not
  copied:" list),
  `lib/cu_walk.c` (a tree walked with a list on the heap, no locks held between
  calls: find, du), `lib/cu_colours.c` (ENV:FileColours and the kinds of files,
  from CShell's ls; exec and dos only, so Ahiru and Maki can copy it as it is),
  `lib/cu_lines.c` (sort: lines kept in 64 KB chunks with a pointer array, and
  a stable bottom-up merge sort),
  `lib/cu_date.c` (the calendar by hand, touch's date parser, ISO dates, the
  after-2046 DateStamp fix: touch, date, stat, diff), `lib/cu_prot.c` (protection
  bits as "----rwed": stat, chmod), `lib/cu_long.c` (paths longer than dos.library's 255
  bytes, locked part by part: cu_walk, tar).
- `lib/cz*.c` - the squeezers (cz.h; docs/tar.md has the decisions): cz.c the kind from
  the first bytes and the input buffer, one file per kind (cz_gz/cz_bz/cz_xz/cz_zst read,
  cz_gzw/cz_bzw write), cz_w.c creating; cz_int.h says how a command names its kinds
  (CZ_READERS/CZ_PACKERS) so the others are not linked; a program naming none (tar) gets
  all from cz_all.c/cz_allw.c. `lib/czc.c` (+ czc_pack.c) is the command shared by gzip,
  gunzip, bzip2, bunzip2, unxz, unzstd. The libraries are in third_party/ (its README).
  **Before trusting a change there**: `tests/cz-pc/run.sh` (the same code for Linux and
  for the 68020 under vamos, compared on ~860 runs; bebbo's gcc and structs by value).
- `tests/sed-pc/` - sed.c built for Linux (cu.h/shim.c stand in for lib/ and dos), run.sh
  compares it with GNU sed and the real bin/sed under vamos with it; stack.sh runs deep
  patterns on a 4 KB stack; one.sh shows one run three ways.
- `src/wget.c` + `src/wget_tls.c` (https on AmiSSL 5, in its own file so the SDK's headers
  stay out of the rest; AmiSSL is opened at run time, only for https; no link library). The
  SDK's headers: `third_party/fetch-amissl.sh` downloads them into third_party/AmiSSL (ignored
  by git, make runs it). Tests: `tests/wgettest.sh` starts `tests/httpd.py` (http 8471, https
  8472), runs wget.t, checks the 3 MB downloads byte for byte on the PC, then wget-https.t on a
  second emulator: AmiSSL needs ~4 MB of free fast RAM, more than the shared 8 MB machine has
  beside the other sessions' tests: `tests/uae030.sh start` (68030, 32 MB, port 1247,
  `tests/w030`; `FAST=1` for the emulator at full speed).
- **A miscompile in lib/cu.c, fixed 2026-10-15**: with errout() inlined, gcc called Write()
  with SysBase in a6 whenever pr_CES was set, so every cu_err message to a redirected error
  stream (`*>` in the Shell, `2>` in csh, wget -o) vanished. errout() is noinline now; check
  with `m68k-amigaos-objdump -dr build/cu.o` that errput loads _DOSBase before Write.
- `src/NAME.c` - one command each, `$VER: NAME 0.1bN (date)`.

## Small mismatches to tidy (found writing the man pages, 2026-10-14)

The pages in man/english/CUtils describe what the code does; these header comments or
usage strings say less or differ (no behaviour is wrong): grep -r recurses on the stack
(stops with "too deep", not "any depth"), a drawer without -r gives rc 10, only the last -e
counts, -R undocumented; find's usage leaves out -iname -mmin -print0 -true -false -depth
-not/-and/-or and `-exec {} +`, -size takes b and G too; cut's usage lacks
--output-delimiter, -n ignored; chmod X undocumented, a named soft link gives rc 5; date
never checks Ctrl-C (header says 20), VER= works like TO=; wc -m, xargs -x -l -i, cp/rm -R
not in usage strings; sort's Amiga form gives rc 10 for an unreadable file (header 5); du's
total uses a tab; stat's labels indent 3. Fixed: tee named the wrong file on a write error
(0.1b2), tail +N (0.1b2).
