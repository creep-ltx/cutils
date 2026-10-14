# man - manual pages on the Amiga (design, decided 2026-10-14)

Decided with the user before any code:

- **Format: simple marked-up text** (Markdown-like): `# name - one line` first, then `## SECTION`
  headings, `**bold**`, `` `code` ``, indented blocks shown as they are. Readable with Type,
  easy to write; man shows it in colour.
- **Shown in Maki** when the output is a terminal (scroll, search, colours); plain text when it
  goes to a file or a pipe (`man ls | grep -i sort`).
- **Where:** `MAN:` (assign, SYS:Man by default), one file per page (`MAN:ls`, maybe
  `MAN:ls.man`); more drawers can be added to MAN: with Assign ADD for your own pages.
- **`man -k word`**: search names and one-line summaries (an index file built on install,
  or scanned when missing).
- **Our own programs**: pages generated at build time from each source's header comment
  (CUtils now; csh's commands, Sumi, Maki, Ahiru, CTerm later).
- **No page**: look for the program's own NAME.doc / NAME.guide (next to it, in the usual
  drawers) and show that (AmigaGuide turned into text).
- **The Amiga's own commands** (List, Copy, Protect, Assign...): pages **written fresh in my own
  words** from how the commands really behave (tested on 3.2, the manuals only for facts), so
  they can be public. English.
- **Language: English.**
- **Library functions** (dos.library, exec.library: `man Open`): later, a second step.

Still to settle while building: the page file names (with or without an ending), sections
(Unix's 1/3/5... or none), and how the MAN: assign gets made (S:User-Startup line, the
user's go needed on the A1200).
