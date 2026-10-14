# CUtils

Unix-style commands for AmigaOS 3.2, in C: the core utils your fingers already
know, one small pure program each, made for `C:` and for csh pipes.

| Command | What it does | State |
|---|---|---|
| cat  | files one after another; -n -b -s -E -v | 0.1b1, tested in FS-UAE |
| head | the first lines or bytes; -n -N -c -q -v | 0.1b1, tested in FS-UAE |
| wc   | lines, words, bytes; -l -w -c -L | 0.1b1, tested in FS-UAE |
| tail | the last lines or bytes; -n -N -n +N -c -f -q -v; big files read from the end | 0.1b1, tested in FS-UAE |
| grep | regular expressions (basic, -E, -F); -i -v -w -x -c -l -L -n -h -H -o -q -s -m -A -B -C -r; matches in colour in a window | 0.1b1, tested in FS-UAE |
| tee  | the input to the output and to files; -a | 0.1b1, tested in FS-UAE |

Every command answers `?` with its usage; its source's first comment is its
manual. Design and rules: [docs/design.md](docs/design.md).

    make              builds bin/*
    tests/run.sh      the cases in tests/cases/ on the private FS-UAE
