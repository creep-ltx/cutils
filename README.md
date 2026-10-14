# CUtils

Unix-style commands for AmigaOS 3.2, in C: the core utils your fingers already
know, one small pure program each, made for `C:` and for csh pipes.

| Command | What it does | State |
|---|---|---|
| basename | the last part of a path, Amiga style (Work:dir/file gives file); suffix, -a -s | 0.1b1, tested in FS-UAE |
| cat  | files one after another; -n -b -s -E -v | 0.1b1, tested in FS-UAE |
| chmod | protection bits: +x -w u+rwx =rwed 755, and the Amiga's s p a h; -R -v -c | 0.1b1, tested in FS-UAE |
| cp   | copy, never over a file unless -f, metadata kept like Copy CLONE; -f -r | 0.1b1, tested in FS-UAE |
| date | replaces C:Date: the same output, template and clock setting (DAY DATE TIME TO); and Unix +FORMAT -r -d | 0.1b1, tested in FS-UAE |
| df   | how full the volumes are: size, used, free, use%; -h | 0.1b1, tested in FS-UAE |
| diff | two files or trees: normal or -u/-U N unified (coloured like git in a window), Myers with GNU's heuristics; -q -s -i -w -b -B -r -N -a | 0.1b1, tested in FS-UAE |
| dirname | a path without its last part (Work:a/b gives Work:a, "file" gives "") | 0.1b1, tested in FS-UAE |
| du   | sizes of trees in KB, drawer by drawer; -a -s -c -h -d N | 0.1b1, tested in FS-UAE |
| file | what a file is: hunk executables and libraries, icons, IFF pictures and sounds with sizes, archives, ADF, MODs, PNG/GIF/JPEG, text kinds; -b | 0.1b1, tested in FS-UAE |
| find | walk trees: -name -type -size -mtime -mmin -newer -empty, ! -o ( ), -maxdepth -mindepth -prune -print -print0 -delete -exec | 0.1b1, tested in FS-UAE |
| grep | regular expressions (basic, -E, -F); -i -v -w -x -c -l -L -n -h -H -o -q -s -m -A -B -C -r; matches in colour in a window | 0.1b1, tested in FS-UAE |
| head | the first lines or bytes; -n -N -c -q -v | 0.1b1, tested in FS-UAE |
| ln | hard (default) and soft (-s) links with MakeLink; -f -v; never across volumes or to roots | 0.1b1, tested in FS-UAE |
| ls   | drawers in columns, coloured by kind (ENV:FileColours); -1 -a -A -d -F -h -l -r -R -S -t | 0.3b1 (CShell's ls 0.2 on lib/), tested in FS-UAE |
| mkdir | make drawers; -p | 0.1b1, tested in FS-UAE |
| mv   | move: Rename() on one volume, copy+delete across, trees all or nothing; -f -b | 0.1b1, tested in FS-UAE |
| rm   | delete: drawers only with -r, roots never, links as links; -r -f -v | 0.1b1, tested in FS-UAE |
| seq | numbers from FIRST by INC to LAST, also decimals and negatives; -w -s -f | 0.1b1, tested in FS-UAE |
| stat | size, blocks, protection bits, date, comment, type, link target; -L -t -c FORMAT | 0.1b1, tested in FS-UAE |
| tail | the last lines or bytes; -n -N -n +N -c -f -q -v; big files read from the end | 0.1b1, tested in FS-UAE |
| tee  | the input to the output and to files; -a | 0.1b1, tested in FS-UAE |
| touch | date a file now (or -d / -r), make it when missing; -c -d -r | 0.1b2, tested in FS-UAE |
| wc   | lines, words, bytes; -l -w -c -L | 0.1b1, tested in FS-UAE |
| which | replaces C:Which: the same output and template (NORES RES ALL), and Unix which [-a] | 0.1b1, tested in FS-UAE |

Every command answers `?` with its usage; its source's first comment is its
manual. Design and rules: [docs/design.md](docs/design.md).

    make              builds bin/*
    tests/run.sh      the cases in tests/cases/ on the private FS-UAE

## The Amiga E versions

`e/` holds the earlier CUtils in Amiga E (ls cp mv mkdir rm, and vinstall), moved here
from [AmigaTools](https://github.com/creep-ltx/AmigaTools) with their history. Their
releases (ls 0.3.4, cp 0.1.2, mv 0.5, mkdir 0.1.1, rm 0.1) stay on AmigaTools' release
page. The C versions above replace them.

## License

MIT, see [LICENSE](LICENSE).
