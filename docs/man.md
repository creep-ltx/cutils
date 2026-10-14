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

## The page format

Simple marked-up text (Markdown-like) in `NAME.man`: `# name - one line` first, `## SECTION`
headings, `**bold**`, `` `code` ``, indented blocks as they are. Readable with Type and in
MultiView as plain text; man shows it in colour. man also reads 3.2's plain manual-form pages
(NAME/FORMAT/TEMPLATE/FUNCTION headings coloured) and AmigaGuide (@node, @{b}, links shown).

Pages for our own programs are generated at build time from each source's header comment
(CUtils now; csh's commands, Sumi, Maki, Ahiru, CTerm later).

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
