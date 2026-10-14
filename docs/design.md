# CUtils in C - design

Unix-style commands for AmigaOS 3.2, in C: the core utils your fingers know,
for csh pipes (`List | grep foo | wc -l`) and for the ROM Shell alike. Started
2026-10-14 (the user: "a C version of CUtils plus cat, grep, head, tail, wc,
find"). The Amiga E versions (ls cp mv mkdir rm) stay in AmigaTools/cutils;
this replaces them.

## Start here (a later session)

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
  bytes, locked part by part: cu_walk, tar), `lib/cz.c` (the squeezing streams: tar;
  docs/tar.md).
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
