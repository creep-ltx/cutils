# tr - change, delete or squeeze characters in text

## SYNOPSIS
    tr [-cCds] SET1 [SET2]

## DESCRIPTION
Reads its input and writes it to the output with characters changed,
left out, or runs of them squeezed into one. It only reads the input
and writes the output, so use it in a pipe or with `<` and `>`:
`tr a-z A-Z <in >out`.

- `tr SET1 SET2` each character in SET1 becomes the one at the same
  place in SET2. When SET2 is shorter, its last character is repeated.
- `tr -d SET1` the characters in SET1 are left out.
- `tr -s SET1` a run of the same character from SET1 becomes one.
- `tr -s SET1 SET2` changed first, then runs of characters in SET2
  squeezed.
- `tr -ds SET1 SET2` SET1 left out, runs of characters in SET2 squeezed.

What a SET may hold:
- **a-z** a range of characters.
- **\\n \\t \\r \\a \\b \\f \\v \\\\** new line, tab, carriage return,
  bell, backspace, form feed, vertical tab, backslash.
- **\\NNN** a character by its octal number.
- **[:alpha:]** a class: also [:upper:] [:lower:] [:digit:] [:alnum:]
  [:space:] [:blank:] [:punct:] [:cntrl:] [:print:] [:graph:]
  [:xdigit:].
- **[c\*N]** in SET2: the character c, N times.
- **[c\*]** in SET2: c as many times as needed to make SET2 as long as
  SET1.

tr works on bytes in Latin-1, the Amiga's own character set: upper and
lower case include the accented letters, so
`tr [:upper:] [:lower:]` works for Swedish, German or French text too.

## OPTIONS
- **-d** delete the characters in SET1.
- **-s** squeeze runs of the same character into one.
- **-c** complement: SET1 is every character that is *not* in it (in
  byte order).
- **-C** the same as **-c**.

## EXAMPLES
    tr a-z A-Z <Work:Docs/notes.txt
    type S:Startup-Sequence | tr [:upper:] [:lower:]
    tr -d \r <Work:dos.txt >Work:amiga.txt
    tr -s " " <Work:table.txt
    tr -cs [:alnum:] \n <Work:Docs/text.txt | sort | uniq -c

## RETURN CODES
0 fine, 10 bad arguments (or a read error), 20 stopped with Ctrl-C.

## NOTES
In the ROM Shell, `*` inside quotes is the escape character: there
write `[c**5]`, or leave the quotes off.

tr takes no file names; give it a file with `<`.

`tr ?` shows the usage line.

## SEE ALSO
`cut`, `sort`, `uniq`, `grep`, `cat`
