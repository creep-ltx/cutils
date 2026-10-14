# CUtils

Unix-style commands for AmigaOS 3.2, in C: the core utils your fingers already
know, one small pure program each, made for `C:` and for csh pipes.

| Command | What it does | State |
|---|---|---|
| cat  | files one after another; -n -b -s -E -v | 0.1b1, tested in FS-UAE |
| cp   | copy, never over a file unless -f, metadata kept like Copy CLONE; -f -r | 0.1b1, tested in FS-UAE |
| df   | how full the volumes are: size, used, free, use%; -h | 0.1b1, tested in FS-UAE |
| du   | sizes of trees in KB, drawer by drawer; -a -s -c -h -d N | 0.1b1, tested in FS-UAE |
| find | walk trees: -name -type -size -mtime -mmin -newer -empty, ! -o ( ), -maxdepth -mindepth -prune -print -print0 -delete -exec | 0.1b1, tested in FS-UAE |
| grep | regular expressions (basic, -E, -F); -i -v -w -x -c -l -L -n -h -H -o -q -s -m -A -B -C -r; matches in colour in a window | 0.1b1, tested in FS-UAE |
| head | the first lines or bytes; -n -N -c -q -v | 0.1b1, tested in FS-UAE |
| ls   | drawers in columns, coloured by kind (ENV:FileColours); -1 -a -A -d -F -h -l -r -R -S -t | 0.3b1 (CShell's ls 0.2 on lib/), tested in FS-UAE |
| mkdir | make drawers; -p | 0.1b1, tested in FS-UAE |
| mv   | move: Rename() on one volume, copy+delete across, trees all or nothing; -f -b | 0.1b1, tested in FS-UAE |
| rm   | delete: drawers only with -r, roots never, links as links; -r -f -v | 0.1b1, tested in FS-UAE |
| tail | the last lines or bytes; -n -N -n +N -c -f -q -v; big files read from the end | 0.1b1, tested in FS-UAE |
| tee  | the input to the output and to files; -a | 0.1b1, tested in FS-UAE |
| touch | date a file now (or -d / -r), make it when missing; -c -d -r | 0.1b1, tested in FS-UAE |
| wc   | lines, words, bytes; -l -w -c -L | 0.1b1, tested in FS-UAE |

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
