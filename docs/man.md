# man - manual pages on the Amiga (design, decided 2026-10-14)

Decided with the user before any code.

## Where the pages are

AmigaOS 3.2 already has the place: **`HELP:`** (= SYS:Locale/Help, more drawers added with
`Assign HELP: ... ADD`, as MUI does with its Docs). It is sorted by language (`english/`,
`svenska/`), and 3.2 ships **110 pages for its own commands** in
`HELP:<language>/Sys/Commands/NAME.help` (plain text in manual form, or AmigaGuide), plus
`Sys/Utilities`, `Sys/Tools`... for the programs. So:

- **MAN: is an assign** (the user, 2026-10-14), and man only ever reads
  **`MAN:<language>/<program>/<page>`**, e.g. `MAN:english/CUtils/ls.man`,
  `MAN:english/CShell/csh.man`, `MAN:english/Sumi/sumi.man`. One drawer per program or
  package: tidy, easy to remove. Where MAN: points is the machine's choice:
  - with HELP: (3.2 sets it in S:Startup-Sequence; 3.0/3.1 most likely too, not checked):
    `Assign MAN: HELP:Man`, so the pages sit with the system's help;
  - without HELP: (2.x): `Assign MAN: SYS:Man`;
  - your own pages added on top: `Assign MAN: Work:MyPages ADD` (DOS looks in each drawer
    of a multi-assign in turn, so man needs no list of its own).
  The installer writes the one line into S:User-Startup (the user's go first). With no MAN:,
  man tries HELP:Man, then SYS:Man, and says how to make MAN:.
- **The Amiga's own commands**: man shows 3.2's own `HELP:<language>/Sys/Commands/NAME.help`
  (on every 3.2 machine; nothing of Hyperion's is copied or published). Fresh pages, in my own
  words, only where 3.2 has none, and for our replacements (sort, which, date) which take both
  the Amiga and the Unix form.
- **Programs with no page**: their own NAME.doc / NAME.guide (next to the program, or in
  HELP:), AmigaGuide shown as text.
- **Language**: the one set in Locale prefs, falling back to English (no Locale, as on 2.x:
  english). Our own pages: English.
- man itself needs only AmigaDOS 2.0 calls, so it can run on 2.04 and up (CUtils is built for
  3.2).

Search order for `man NAME`: MAN:<lang>/*/NAME.man, then (where HELP: exists) HELP:<lang>/Sys/*/NAME.help,
then english for both, then NAME.doc/.guide found near the program (which-style path search)
or anywhere in HELP:.

## The page format (.man)

Plain text with a few marks, readable as it is (Type, MultiView). An example is the spec:

    # ls - list drawers in columns, coloured by kind

    ## SYNOPSIS
        ls [-1aAdFhlrRSt] [path | pattern ...]

    ## DESCRIPTION
    Shows what is in a drawer, in columns that fit the window. Text in a
    paragraph is joined and wrapped by man to the window's width; a blank line
    ends a paragraph. **bold**, `code` (shown bold too), *word* (underlined).

    ## OPTIONS
    - **-1** one name a line.
    - **-l** long: protection bits, size, date, name, the filenote below.
    - **-k** POS1[,POS2]
      a continued item is indented two spaces; it joins the item.

    ## EXAMPLES
        ls -la Work:
        ls #?.c

    ## RETURN CODES
    0 fine, 5 a path could not be read, 10 bad arguments, 20 Ctrl-C.

    ## SEE ALSO
    `find`, `du`, `stat`

The rules:
- Line 1: `# NAME - summary` (the summary is what `man -k` searches and shows).
- `## HEADING`: a section; the usual ones, in this order: SYNOPSIS, DESCRIPTION, OPTIONS,
  then any of the program's own (AMIGA FORM, FILES, ...), EXAMPLES, RETURN CODES, NOTES,
  SEE ALSO.
- A line indented 4 spaces (or a tab): shown exactly as it is (synopsis, examples, tables).
- `- ` at a line's start: a list item; lines after it indented 2 spaces belong to it. An item
  starting with `**term**` is shown with the term on the left and the text in a column after
  it (as Unix man's options), or under it when the term is long.
- Inline: `**bold**`, `` `code` ``, `*underline*`. No other markup; a literal `*` or `` ` `` is
  written `\*`, `` \` ``.
- Latin-1 or plain ASCII; lines up to 76 characters so Type shows them unwrapped.
- `\*` is a plain star everywhere outside verbatim blocks, inside `` `code` `` too (man.c
  takes the escape first); in a verbatim block a star is just `*`.
- An option's argument goes after the term in capitals (`- **-k** POS1[,POS2] text`): man
  puts the first capitals word after the term into the term column, underlined.

## How it shows

- **man is the program that opens the page, always.** It has its own small viewer built in
  (scroll, page, search, q), so it works with nothing else installed.
- **ENV:PAGER** (as on Unix) may name another viewer, e.g. Maki, for its extras; man then hands
  the formatted page to it.
- Output to a file or a pipe (`man ls | grep -i sort`): plain text, no viewer.
- **`man -k word`**: search names and one-line summaries of every page.

## Later

- Library functions (dos.library, exec.library: `man Open`): a second step.
- Still to settle while building: sections (Unix's 1/3/5 or none), and installing (copying the
  pages into HELP:Man, the user's go needed on the A1200).
