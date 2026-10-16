# grep - find the lines that match a pattern (regular expression)

## SYNOPSIS
    grep [-EFivwxcLlnhHoqsr] [-e PAT] [-m N] [-A N] [-B N] [-C N]
         PATTERN [file ...]

## DESCRIPTION
Reads files, or its input, and prints the lines in which PATTERN is
found. It is the quickest way to look for a word in many files, or to
pick lines out of another command's output (`List | grep .info`).

When several files are searched, each line is shown after the name of
the file it is in. With no file, or `-`, grep reads its input. In a
window the matches are coloured.

PATTERN is a *regular expression*. The most useful parts:

    .         any one character
    [abc]     one of a, b, c       [^a-z]  anything but a to z
    [[:alpha:]] [[:digit:]] [[:space:]] ...  classes of characters
    ^         the start of the line
    $         the end of the line
    *         the thing before it, any number of times (also none)
    \w        a letter, digit or _   (\W anything else)
    \s        a space or tab         (\S anything else)
    \.        a backslash makes a special character plain: a dot

That is the *basic* form, as GNU grep's default: there `( ) | { } + ?`
are plain characters and `\( \) \| \{ \} \+ \?` are special. With
**-E** (extended) it is the other way round, which is easier to write:
`grep -E "cat|dog"`.

## OPTIONS
- **-E** extended regular expressions (`+ ? | ( ) {n,m}`).
- **-F** the pattern is plain text, nothing special in it.
- **-e** PAT
  the pattern is PAT; for a pattern that starts with `-`.
- **-i** upper and lower case are the same.
- **-v** the lines that do *not* match.
- **-w** only whole words match.
- **-x** only whole lines match.
- **-c** only a count of matching lines, per file.
- **-l** only the names of the files that match.
- **-L** only the names of the files that do not match.
- **-n** line numbers in front of the lines.
- **-h** never the file names in front.
- **-H** always the file names in front.
- **-o** only the matching parts, one a line.
- **-q** quiet: print nothing, only set the return code.
- **-s** no messages about files that cannot be read.
- **-m** N
  stop after N matching lines (in each file).
- **-A** N
  also N lines after each match.
- **-B** N
  also N lines before each match.
- **-C** N
  N lines before and after each match.
- **-r** search drawers too: every file inside them, at any depth.
  **-R** is the same.
- **--colour=**WHEN
  `always` colours also into a file or a pipe, `never` not at all,
  `auto` (the default) only in a window. `--colour` alone is `always`;
  `--color` works too.

## EXAMPLES
    grep -i assign S:Startup-Sequence
    grep -n -r LoadWB SYS:S
    grep -l Amiga Work:Docs/#?.txt
    List SYS:C | grep -i dir
    grep -c -v "^$" S:User-Startup
    grep -E "^(Assign|Path) " S:#?

## RETURN CODES
0 something matched, 5 nothing matched, 10 an error (a file that could
not be read, a bad pattern, bad arguments), 20 stopped with Ctrl-C.

## NOTES
Quoting: in csh, quote as in bash (`-d' '`, `-d" "`, `"a.*b"`). In the ROM
Shell and in AmigaDOS scripts a quote counts only at the start of a word,
and inside quotes `*` is the escape character (`*"`, `*N`): write
`-d " "` or `"-d "`, and `**` for one `*` (`"a.**b"`).

GNU grep returns 1 for "no match" and 2 for errors. On the Amiga 5 is
the WARN code, so a script can say `IF WARN` for "not found" and still
go on.

Without **-r** a drawer named as a file gives an error (rc 10). With
**-r**, links are not gone into. Very deep drawers may need a bigger
stack (`Stack 16000`).

The patterns for *file names* are the Amiga's: `#?.txt` or `*.txt`,
upper and lower case the same. The PATTERN for the *text* is a regular
expression, where `*` means "repeat". Put a PATTERN with spaces, `;`,
`>` or `|` in quotes.

There are no back-references (`\1`).

`grep ?` shows the usage line.

## SEE ALSO
`find`, `cut`, `sort`, `uniq`, `wc`, `diff`, Search
