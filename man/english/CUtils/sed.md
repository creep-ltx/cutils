# sed - the stream editor: change text with a script, line by line

## SYNOPSIS
    sed [-nEsuz] [-i[SUFFIX]] [-l N] [-e SCRIPT]... [-f FILE]...
        [SCRIPT] [file ...]

## DESCRIPTION
Reads the files (or its input) one line at a time, runs a small script
on each line and prints the result. It is the tool for changing text
without opening an editor: replace a word in many files, delete lines,
pick lines out, put text before or after a line. This is GNU sed's
language, so the scripts found in books and on the net work as they are.

Without **-e** or **-f** the first word is the script. Several commands
go in one script separated by `;` or new lines. Each line goes into the
*pattern space*; the commands change it; at the end of the script it is
printed (unless **-n**) and the next line is read. A second buffer, the
*hold space*, keeps text from one line to the next.

A command may have an address in front of it, then it works only on the
lines the address picks:

    3         line 3
    $         the last line
    /re/      the lines that match the regular expression re
    \%re%     the same, with another character around it
    /re/I     upper and lower case the same
    /re/M     ^ and $ also at the newlines inside the pattern space
    1~3       line 1, then every 3rd line (1, 4, 7 ...)
    3,7       lines 3 to 7 (a range: from the first to the second)
    /a/,/b/   from a line matching a to the next matching b
    5,+2      line 5 and the 2 after it
    5,~4      from line 5 to the next line number divisible by 4
    0,/re/    from the start to the first match (also on line 1)
    addr!     the lines the address does NOT pick

## COMMANDS
- **s/re/new/flags** replace what re matches with new. In new, `&` is
  the whole match, `\1` to `\9` the parts in `\( \)`, `\n` a newline,
  `\t` a tab, `\&` a plain &, `\U` `\L` upper or lower case until `\E`,
  `\u` `\l` the next character only. Flags: `g` all matches, a number
  N only the Nth (with `g`: the Nth and after), `p` print if changed,
  `i` or `I` case blind, `m` or `M` multi-line, `w FILE` write if
  changed. Another character than `/` works too: `s|/usr|/opt|`.
- **y/abc/xyz/** change each a into x, b into y, c into z.
- **p** print the pattern space. **P** its first line.
- **d** delete it, start the next line. **D** delete its first line and
  run the script again on the rest (without reading a line).
- **n** print it (unless -n), read the next line in its place.
- **N** add a newline and the next line to it.
- **g** **G** the hold space into (G: after) the pattern space.
  **h** **H** the pattern space into (H: after) the hold space.
  **x** swap them.
- **a TEXT** put TEXT after the line, **i TEXT** before it, **c TEXT**
  instead of it (for a range: once, at its end). The classic form
  `a\` with the text on the next lines works too; end each line but the
  last with `\`.
- **r FILE** put the file after the line. **R FILE** one line of the
  file each time. **w FILE** write the pattern space to the file (made
  empty when sed starts). **W FILE** its first line. `/dev/stdout` and
  `/dev/stderr` are the output and the error output; `r /dev/stdin`
  reads the input.
- **=** print the line number. **l** show the pattern space with the
  unprintable characters as `\t` `\001` ..., wrapped (`l N`: at N).
  **F** print the file name.
- **q** [CODE] print, then stop (return code CODE). **Q** [CODE] stop
  without printing.
- **:label** a place to jump to. **b label** jump (no label: to the
  end). **t label** jump if an s has changed something since the last
  line was read or the last t. **T label** jump if not.
- **{ ... }** a block: the commands inside work on the lines the
  address of `{` picks. **z** empty the pattern space. **#** a comment
  (`#n` as the first line of a script: like -n).

## REGULAR EXPRESSIONS
The *basic* form, as GNU sed's default: `( ) { } | + ?` are plain
characters and `\( \) \{ \} \| \+ \?` are special. With **-E** it is the
other way round. Both:

    .  any character       [abc] [^a-z] [[:alpha:]]  one of these
    ^  start   $  end      *  any number of the thing before
    \w a word character    \W  not one   \s space   \S not space
    \b a word's edge       \< its start  \> its end
    \n a newline           \t a tab      \xHH \dNNN \oNNN a byte

An empty regular expression (`//`) is the last one used. The longest of
the leftmost matches is taken, as POSIX says.

## OPTIONS
- **-n** no automatic printing: only `p` and the like print.
- **-E** extended regular expressions. **-r** is the same.
- **-i**[SUFFIX]
  edit the files in place: the result replaces each file (protection
  bits and file note kept, the date is now). With SUFFIX the old one is
  kept as name+SUFFIX; a `*` in SUFFIX stands for the file name
  (`-ibak_*`). Implies **-s**.
- **-s** each file on its own: `$` and line numbers per file.
- **-u** write each line out at once (for a pipe that is watched).
- **-z** lines end with a 0 byte instead of a newline.
- **-l** N
  where `l` wraps its lines (70; 0: never).
- **-e** SCRIPT
  add SCRIPT to the script (several are joined by new lines).
- **-f** FILE
  add the script in FILE (`-` the input).
- **--expression=**SCRIPT, **--file=**FILE, **--quiet**, **--silent**,
  **--in-place**[=SUFFIX], **--regexp-extended**, **--separate**,
  **--null-data**, **--line-length=**N, **--unbuffered**, **--posix**,
  **--version**: the long names, as GNU sed's.

## EXAMPLES
Replace words (the first on each line; with g all of them):

    sed s/colour/color/ notes
    sed "s/colour/color/g" notes >notes.new

Edit files in place, keeping the old ones as .bak:

    sed -i.bak "s/Work:/Data:/g" S:User-Startup

Print only some lines (-n and p):

    sed -n 5p file              line 5
    sed -n "10,20p" file        lines 10 to 20
    sed -n "/Assign/p" S:Startup-Sequence
    sed -n "$p" file            the last line
    sed 5q file                 the first 5 lines (as head -5)

Delete lines:

    sed 1d file                 the first line
    sed "/^$/d" file            empty lines
    sed "/^;/d" S:User-Startup  comment lines
    sed "$d" file               the last line

From one marker to another:

    sed -n "/BEGIN/,/END/p" file
    sed "/BEGIN/,/END/d" file

Add, insert, change lines:

    sed "1i First line" file
    sed "$a Last line" file
    sed "/^Path /a Assign MAN: HELP:Man" S:User-Startup
    sed "3c This is line 3 now" file

Swap words with groups, upper case with \\U:

    sed -E "s/(\w+) (\w+)/\2 \1/" file
    sed "s/\w\+/\u&/g" file     each word capitalised

Change characters one for one (as tr):

    sed "y/abc/ABC/" file

The hold space: the lines in reverse order (as tac), all lines joined
with commas, every line numbered:

    sed "1!G;h;$!d" file
    sed "H;$!d;x;s/\n/,/g;s/^,//" file
    sed = file | sed "N;s/\n/ /"

A script in a file, for the longer jobs:

    sed -f fix.sed file

## RETURN CODES
0 fine, 10 a bad script or a file that could not be read or written
(sed goes on with the next file), 20 stopped with Ctrl-C or out of
memory. `q CODE` and `Q CODE` end with CODE.

## AMIGA NOTES
Put the script in double quotes when it has spaces, `;`, `<`, `>` or
`|` in it: in the ROM Shell an unquoted `;` starts a comment, so
`sed -n /a/p;/b/p file` runs `sed -n /a/p` alone, which then waits for
its input.

In the ROM Shell `*` is the escape character inside double quotes
(`*n` a new line, `*"` a quote): write `**` for each `*` there. So
`".*"` must be typed `".**"`. Outside quotes a `*` is plain, and csh
quotes as Unix shells do (`'s/.*/x/'`), with no such trap.

GNU's back-references inside a pattern (`\(a\)\1`, a repeated part) are
not supported: sed says so and stops. In the replacement `\1` works. The
`e` command and the `e` flag (run a shell command) are not there either.
A repeat count is at most 100 (`a\{100\}`), and a very long pattern may
be refused as too big.

Latin-1 letters change case with `\U`, `\L` and the `I` flag; `l` shows
bytes over 127 in octal, as GNU sed does.

GNU sed returns 1 for a bad script, 2 for a missing file and 4 for an
I/O error; here they are all 10 (ERROR), so `IF ERROR` catches them.

`sed ?` shows the usage line.

## SEE ALSO
`grep`, `tr`, `cut`, `head`, `tail`, `diff`
