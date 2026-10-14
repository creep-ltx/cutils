# CUtils

Unix-style commands for AmigaOS 3.2, in C: the core utils your fingers already
know, one small pure program each, made for `C:` and for csh pipes.

| Command | What it does | State |
|---|---|---|
| basename | the last part of a path, Amiga style (Work:dir/file gives file); suffix, -a -s | 0.1b1, tested in FS-UAE |
| bunzip2 | unpack bzip2 (bzip2 -d): joined streams, every CRC checked, small mode by itself when memory is short; -c -k -f -t -r -v -q -S | 0.1b1, tested in FS-UAE |
| bzip2 | pack (-1..-9) and unpack bzip2, files replaced with their date, bits and note kept; -c -d -k -f -t -r -v -q -S | 0.1b1, tested in FS-UAE |
| cat  | files one after another; -n -b -s -E -v | 0.1b1, tested in FS-UAE |
| chmod | protection bits: +x -w u+rwx =rwed 755, and the Amiga's s p a h; -R -v -c | 0.1b1, tested in FS-UAE |
| cp   | copy, never over a file unless -f, metadata kept like Copy CLONE; -f -r | 0.1b1, tested in FS-UAE |
| cut  | parts of lines: fields, characters, bytes; -f -d -s -c -b --complement --output-delimiter | 0.1b1, tested in FS-UAE |
| date | replaces C:Date: the same output, template and clock setting (DAY DATE TIME TO); and Unix +FORMAT -r -d | 0.1b1, tested in FS-UAE |
| df   | how full the volumes are: size, used, free, use%; -h | 0.1b1, tested in FS-UAE |
| diff | two files or trees: normal or -u/-U N unified (coloured like git in a window), Myers with GNU's heuristics; -q -s -i -w -b -B -r -N -a | 0.1b1, tested in FS-UAE |
| dirname | a path without its last part (Work:a/b gives Work:a, "file" gives "") | 0.1b1, tested in FS-UAE |
| du   | sizes of trees in KB, drawer by drawer; -a -s -c -h -d N | 0.1b1, tested in FS-UAE |
| file | what a file is: hunk executables and libraries, icons, IFF pictures and sounds with sizes, archives, ADF, MODs, PNG/GIF/JPEG, text kinds; -b | 0.1b1, tested in FS-UAE |
| find | walk trees: -name -type -size -mtime -mmin -newer -empty, ! -o ( ), -maxdepth -mindepth -prune -print -print0 -delete -exec | 0.1b1, tested in FS-UAE |
| free | chip, fast and total memory: size, used, free, largest block; -h -k -m -b | 0.1b1, tested in FS-UAE |
| grep | regular expressions (basic, -E, -F); -i -v -w -x -c -l -L -n -h -H -o -q -s -m -A -B -C -r; matches in colour in a window | 0.1b1, tested in FS-UAE |
| gunzip | unpack gzip (gzip -d): multi-member files, every CRC and length checked; -c -k -f -t -l -r -v -q -S | 0.1b1, tested in FS-UAE |
| gzip | pack (-1..-9) and unpack gzip, files replaced with their date, bits and note kept; -c -d -k -f -n -t -l -r -v -q -S | 0.1b1, tested in FS-UAE |
| head | the first lines or bytes; -n -N -c -q -v | 0.1b1, tested in FS-UAE |
| kill | stop a program with a break: by Shell number, 0x address or name (as pkill); -C -D -E -F | 0.1b1, tested in FS-UAE |
| ln | hard (default) and soft (-s) links with MakeLink; -f -v; never across volumes or to roots | 0.1b1, tested in FS-UAE |
| ls   | drawers in columns, coloured by kind (ENV:FileColours); -1 -a -A -d -F -h -l -r -R -S -t | 0.3b1 (CShell's ls 0.2 on lib/), tested in FS-UAE |
| man  | manual pages: Markdown (ours) and real Unix man pages (roff) in MAN:, the system's own in HELP:, a program's .guide/.doc; own viewer (or ENV:PAGER); -k -w -a -L -l | 0.1b2, tested in FS-UAE |
| mkdir | make drawers; -p | 0.1b1, tested in FS-UAE |
| mv   | move: Rename() on one volume, copy+delete across, trees all or nothing; -f -b | 0.1b1, tested in FS-UAE |
| ps   | the Shells and what they run (as Status), or every task (-e); stack headroom (-l) | 0.1b1, tested in FS-UAE |
| rm   | delete: drawers only with -r, roots never, links as links; -r -f -v | 0.1b1, tested in FS-UAE |
| seq | numbers from FIRST by INC to LAST, also decimals and negatives; -w -s -f | 0.1b1, tested in FS-UAE |
| sort | lines in order: -n -h -V -M -f -d -i -b -r -u -s -c -C -k POS1,POS2 -t -o; also takes C:Sort's FROM TO COLSTART CASE NUMERIC LEXICOGRAPHIC DESCENDING SKIPBLANKS (replaces it) | 0.1b1, tested in FS-UAE side by side with C:Sort |
| stat | size, blocks, protection bits, date, comment, type, link target; -L -t -c FORMAT | 0.1b1, tested in FS-UAE |
| tail | the last lines or bytes; -n -N -n +N -c -f -q -v; big files read from the end | 0.1b1, tested in FS-UAE |
| tar  | GNU tar: create, list, unpack v7/ustar/GNU/pax archives (squeezed ones found by their first bytes); Amiga protection bits, file notes, soft and hard links kept (pax AMIGA.protect/AMIGA.comment); safe unpacking; -c -x -t -f -v -z -j -J -a -C -k -O -m -T --strip-components --exclude | 0.1b1 (plain archives; squeezing waits for lib/cz.c), tested in FS-UAE and with GNU tar on Linux |
| tee  | the input to the output and to files; -a | 0.1b1, tested in FS-UAE |
| touch | date a file now (or -d / -r), make it when missing; -c -d -r | 0.1b2, tested in FS-UAE |
| tr   | characters changed, deleted, squeezed: ranges, classes, \\NNN, [c*N]; -c -d -s; Latin-1 case | 0.1b1, tested in FS-UAE |
| uniq | runs of equal lines once; -c -d -u -i -f -s -w, IN and OUT files | 0.1b1, tested in FS-UAE |
| unxz | unpack xz: joined streams, CRC32/CRC64/SHA-256 checked, BCJ filters, the window grown as needed; -c -k -f -t -l -r -v -q -S | 0.1b1, tested in FS-UAE |
| unzstd | unpack zstd: frames, skippable frames, checksums; keeps the .zst unless --rm; -c -k -f -t -l -r -v -q -S | 0.1b1, tested in FS-UAE |
| uptime | time since the last start (the E-clock), Shells open; -p -s | 0.1b1, tested in FS-UAE |
| wc   | lines, words, bytes; -l -w -c -L | 0.1b1, tested in FS-UAE |
| which | replaces C:Which: the same output and template (NORES RES ALL), and Unix which [-a] | 0.1b1, tested in FS-UAE |
| xargs | commands built from the input's words; -n -L -I -s -t -0, never runs with no words | 0.1b1, tested in FS-UAE |

Every command answers `?` with its usage, and has a manual page: `man NAME`
(Markdown pages in man/english/CUtils, installed into MAN:english/CUtils; docs/man.md). Design and rules: [docs/design.md](docs/design.md).

    make              builds bin/*
    tests/run.sh      the cases in tests/cases/ on the private FS-UAE

## The Amiga E versions

`e/` holds the earlier CUtils in Amiga E (ls cp mv mkdir rm, and vinstall), moved here
from [AmigaTools](https://github.com/creep-ltx/AmigaTools) with their history. Their
releases (ls 0.3.4, cp 0.1.2, mv 0.5, mkdir 0.1.1, rm 0.1) stay on AmigaTools' release
page. The C versions above replace them.

## License

MIT, see [LICENSE](LICENSE).
