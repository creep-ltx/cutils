# csh - CShell, a modern shell for the Amiga

## SYNOPSIS
    csh
    csh [FROM] script [ARGS ...]
    csh COMMAND "text"

## DESCRIPTION
csh is a shell: it shows a prompt, reads what you type and runs it. It
speaks two languages. At the prompt and in csh scripts it speaks a
language like bash's (pipes, variables, loops, functions, `*` wildcards).
In Amiga scripts it speaks the Amiga's own script language, so every
script you already have still runs, exactly as in the ROM Shell.

At the prompt it gives what fish and zsh are loved for: the line is
coloured as you type, a grey suggestion from your history appears, and
Tab completes commands, files, drawers, assigns and volumes. Every key
can be changed.

Type `csh` in a Shell window or a CTerm tab to start it. It takes over
that Shell: its Path, current drawer and local variables are the same
ones. `exit` goes back to the ROM Shell under it. `EndCLI` or `EndShell`
ends the whole Shell.

csh runs each command the way the ROM Shell does, on its own process,
so Amiga programs see a normal Shell. Commands are found in this order:
aliases, functions, csh's built-in commands (see `csh-builtins`), the
resident list, the current drawer, the Path, then C:. A drawer typed
alone is a CD. A file with the s bit is a script: a csh script if its
first line is `#!csh`, an ARexx script if it starts with `/*`, else an
Amiga script.

To make csh the Shell of every new Shell window, see `cshell`.

## OPTIONS
- **FROM** script
  runs the script, then ends. A script whose first line is `#!csh` is
  read in the csh language; any other script is an Amiga script. The
  word FROM may be left out.
- **ARGS** the words after the script are given to a `#!csh` script as
  `$1`, `$2` and so on.
- **COMMAND** text
  runs the text in the csh language, then ends.

With none of these csh shows its prompt. When its input is not a
window (a file, NIL:) it reads lines plainly, without the line editor.

## TWO LANGUAGES
A line typed at the prompt is in the csh language. Amiga commands still
work there, because they are commands: `CD`, `Path`, `Assign`, `Set`,
`Alias`, `Echo` and the rest. Only the Amiga script words (`If`,
`Else`, `EndIf`, `Skip`, `Lab`, `.KEY`) belong to Amiga scripts.

Where the two disagree, csh does this:

    ;          ends a command (in Amiga scripts: starts a comment)
    #          a comment at a word's start, but #? #( #[ ## #~
               stay Amiga patterns
    * ? [..]   match file names, as in bash: echo *.txt
    #?         never matched by csh; the program gets it as it is
    $name      unknown is empty (in Amiga scripts: left as written)
    Set x 1    sets x, as x=1 does
    alias      alias x=text is csh's; alias x text the ROM's
    true/false 0 is true; any other return code is false

When `*` matches files, an Amiga command gets the file names. A command
that takes only one name (Dir, CD) gets the pattern as `#?` instead, and
so does a pattern that matches nothing. `set -f` turns matching off.

csh's variables are AmigaDOS local variables, so `Get`, `$name` in an
Amiga script and every program see the same ones. `export name` makes a
variable global (ENV:).

## THE PROMPT LINE
csh edits the line itself, the same way in CTerm and in a plain Shell
window. While you type:

- **colours** a command that exists is green, an unknown one red, a
  built-in cyan; strings, variables, comments and redirections have
  colours of their own (see csh.prefs).
- **suggestion** the newest history line starting with what you typed
  is shown in grey after the cursor. Right or End at the end of the
  line takes it, Alt+Right (or Shift+Right) one word of it, F1 all of
  it.
- **history** Up and Down go through the lines that start with what was
  typed before the first Up. Ctrl+R searches the history for text
  anywhere in a line; Ctrl+R again finds an older one, Return runs it,
  Esc gives back the line you had.
- **completion** Tab completes the word before the cursor: a command
  in a command's place, a `$variable`, else a file or drawer, an assign,
  a volume, or one of the command's own keywords (read from its
  template). One match is put in; with several, as much as they share,
  or a list under the line.
- **paste** a paste of several lines runs them one by one. Ctrl+C
  drops the lines not yet run.

A line that does not end a command (`if ...; then`, an open quote)
asks for the next line with PS2 (`> `).

## KEYS
These are the default keys. C- is Ctrl, A- is Alt, S- is Shift.

    Left C-b              one character left
    Right C-f             one character right; at the end: the
                          suggestion
    A-Left A-b S-Left     one word left
    A-Right A-f S-Right   one word right; at the end: a word of
                          the suggestion
    Home C-a C-Left       the start of the line
    End C-e C-Right       the end; at the end: the suggestion
    BS                    delete the character before the cursor
    Del                   delete the character under the cursor
    A-BS C-w              delete the word before the cursor
    A-d A-Del             delete the word after the cursor
    C-k                   delete to the end of the line
    C-u                   delete to the start of the line
    C-y                   put back what was deleted last
    C-t                   swap two characters
    Up C-p                an older line (starting as typed)
    Down C-n              a newer line
    C-Up S-Up PgUp        the oldest line
    C-Down S-Down PgDn    back to the line being typed
    A-.                   put in the last word of the last line
    C-r                   search the history
    Tab                   complete
    Enter C-j             run the line
    C-l                   clear the window
    C-c Esc               throw the line away
    C-d                   on an empty line: end csh; else delete
                          the character under the cursor
    C-\                   the same as C-d
    F1                    take the whole suggestion

Right Amiga keys belong to CTerm's menus and Left Amiga keys to its
panes; csh never sees them. An A1200 has no Home or End key, so the
Ctrl keys do that.

## CSH.KEYS
The keys are in `ENVARC:CShell/csh.keys`, one a line: the key, then
what it does. A line starting with `;` is a comment. When the file is
missing, csh writes it with the defaults, in ENVARC: and ENV:.

    <C-r>     search
    <A-Left>  wordleft
    <F2>      none

A key is a character, or a name in angle brackets with C- (Ctrl), A-
or M- (Alt) and S- (Shift) in front: `<Up> <Down> <Left> <Right>
<Home> <End> <PgUp> <PgDn> <Ins> <Del> <BS> <Tab> <Enter> <Return>
<Esc> <Help> <Space> <F1>` to `<F12>`, or `<C-x>` for a letter.

The actions: left right wordleft wordright start end backspace delete
delwordleft delwordright killend killstart yank prev next search
complete accept suggest suggestword clear cancel eof transpose first
last lastarg, and **none**, which takes a key away.

The defaults are read first and the file over them, so a line in the
file changes only that key. csh reads `ENV:CShell/csh.keys` first and
`ENVARC:` only when ENV: has none: after changing the ENVARC: copy,
copy it to ENV: as well (or reboot). A new csh reads the keys when it
starts. The first line it does not understand is shown when it
starts.

## CSH.PREFS
Settings are in `ENVARC:CShell/csh.prefs`, one a line: a name, then
its value; `;` starts a comment. It is written with the defaults when
missing and read in the same way as csh.keys (ENV: first).

    history 500       how many lines the history keeps
    suggest on        the grey suggestion (on or off)
    colour on         colours in the line (on or off)

The colours: each kind gets two SGR codes, the first for CTerm, the
second for a plain Shell window (its pens), `-` for none. Codes may be
joined with `;` (1;92). In CTerm 90-97 are its theme's colours: 90
grey, 91 red, 92 green, 93 yellow, 94 blue, 95 magenta, 96 cyan, 97
white. 1 is bold, 3 italic, 4 underline. The defaults:

    command 92,33       a command that exists
    unknown 91,3        a command not found
    builtin 96,33       a built-in command
    string 93,-         'quoted' and "quoted" text
    variable 95,-       $name
    option -,-          -x words
    comment 90,32       # a comment
    redirection 1,1     > < |
    suggestion 90,32    the grey suggestion
    promptrc 91,1       a failed return code in the prompt

## HISTORY
The history keeps the last 500 lines (the history setting). It is
shared between all csh windows and tabs: each line is written to
`ENV:CShell/history` as it is typed, so another csh sees it at its next
prompt. It is saved to `ENVARC:CShell/history` every 20 lines and when
csh ends, so it is kept across boots. A line starting with a space is
not kept. `history` lists it, numbered.

At the prompt csh also understands bash's history words. The line they
make is shown before it runs.

    !!        the last line
    !n        line n (as history numbers it)
    !-n       the line n lines back
    !str      the last line starting with str
    !$        the last word of the last line
    !^        its first argument
    !*        all its arguments
    ^old^new  the last line, with old made new

They are not used inside '...', or when `!` is followed by a space, `=`
or `(`. `set +H` turns them off.

## THE PROMPT
Without settings csh shows the Amiga's prompt, set with the Prompt
command (`%N` the Shell's number, `%S` the current drawer, `%R` the
return code). With the default prompt, a failed return code is shown
before the `>`.

The variable **PROMPT** (or **PS1**) wins over that. `$variables` and
`$(commands)` in it are worked out first, then the escapes below.
**RPROMPT** is shown at the right edge of the line, and goes away when
the typed line reaches it. **PS2** is the prompt of a line that goes on.

    %N            the Shell's number
    %S %/         the current drawer
    %~            the current drawer, with HOME shown as ~
    %c            the last part of the current drawer
    %V            the volume
    %R %?         the last return code
    %T            the time, 12:34
    %*            the time, 12:34:56
    %D            the date, 2026-10-14
    %j            how many jobs are running
    %M            free memory (K, M or G)
    %E            how long the last command took (2 s or more)
    %h            the number of the next history line
    %F{red} %f    text colour on, off
    %K{blue} %k   background colour on, off
    %B %b         bold on, off
    %U %u         underline on, off
    %(?.yes.no)   yes when the last return code was 0, else no
    %(j.yes.no)   yes when jobs are running, else no
    %%            a %

The colours are black, red, green, yellow, blue, magenta, cyan, white,
or a number 0 to 15. Put the prompt in single quotes so that it is
worked out anew each time.

## JOBS
A command with `&` after it runs in the background; csh says its job
number and the Shell numbers of its processes (`[1] 5`) and gives the
prompt back. Its output still comes to the window. When it ends, csh
says so at the next prompt (`[1] Done`).

- **jobs** lists the jobs.
- **fg** [%n]
  waits for a job (the newest without %n).
- **kill** [%n]
  sends Ctrl+C to every process of a job.
- **wait** [%n]
  waits for one job, or for all of them.

csh's own commands (a loop, a function) can run with `&` too: a second
copy of csh runs them, so what they change stays there. A job cannot be
stopped (there is no Ctrl+Z), and a program that ignores Ctrl+C cannot
be ended by force on the Amiga. Leaving csh with jobs running warns
once; a second `exit` leaves them running.

## FILES
- **C:csh** the shell.
- **S:CShell-Startup** run once, at the first prompt, in the csh
  language: the place for your aliases, functions and PROMPT.
- **ENVARC:CShell/csh.keys** the keys.
- **ENVARC:CShell/csh.prefs** the settings.
- **ENVARC:CShell/history** the history (ENV:CShell/history while
  running).
- **ENVARC:FileColours** the colours of csh's `ls`, shared with
  CTerm's family of programs.

## EXAMPLES
    csh
    csh FROM S:backup.csh Work:
    csh COMMAND "for f in *.txt; do wc -l $f; done"

A S:CShell-Startup:

    alias ll='ls -l'
    PROMPT='%F{green}%~%f%(?.. %F{red}%?%f)> '
    RPROMPT='%T'

## RETURN CODES
csh gives back the return code of the last command it ran, or the
number given to `exit`. 20 when its arguments are wrong or the FROM
file cannot be opened.

## NOTES
Ctrl+C stops the running command. Ctrl+D stops a loop of csh's own even
when a `trap ... INT` takes Ctrl+C, as it stops an Amiga script.

csh keeps `$COLUMNS` set to the window's width for programs like ls,
and `$CMD_DURATION` to how long the last typed line took, in
milliseconds.

csh is made for AmigaOS 3.2 and a 68020 or better.

## SEE ALSO
`csh-builtins`, `cshell`, `ls`, CTerm
