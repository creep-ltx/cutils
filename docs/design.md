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
- State: see the table in README.md.

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
- **Names that clash with 3.2's C:** (Sort, Which, Search, Type) are not used.
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
  from CShell's ls; exec and dos only, so Ahiru and Maki can copy it as it is).
- `src/NAME.c` - one command each, `$VER: NAME 0.1bN (date)`.
