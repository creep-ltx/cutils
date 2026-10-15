# csh-builtins - csh's own commands and its bash-style language

## SYNOPSIS
    name=value  $name  ${name}  $(command)  $((1 + 2))
    cmd1 | cmd2   cmd &   cmd1 && cmd2   cmd1 || cmd2   cmd1; cmd2
    if ...; then ...; elif ...; then ...; else ...; fi
    for x in words; do ...; done    while ...; do ...; done
    case word in pattern) ...;; esac    name() { ...; }

## DESCRIPTION
This page is the reference for the language csh speaks at the prompt,
in `#!csh` scripts and in S:CShell-Startup. It is the common part of
bash's language, on Amiga terms: Amiga paths (`Work:`, `DH0:`, `/` is
the parent drawer), Amiga commands, Amiga variables. A script written
for bash often runs unchanged.

How to start csh, the keys and the prompt are on the `csh` page.

## OPTIONS
csh's own commands are listed under BUILT-IN COMMANDS. The options of
csh itself are on the `csh` page.

## WORDS AND QUOTES
- **'text'** exactly as written; nothing is special inside.
- **"text"** `$name`, `$(...)` and backquotes work inside; the
  rest is plain. The text stays one word.
- **\\x** the character x, plain.
- **$'text'** C escapes: `\n` `\t` `\e` `\x41` `\101` `\\` `\'`.
- **#** a comment, at a word's start. Amiga patterns (`#?`, `#(`,
  `#[`, `##`, `#~`) are not comments.
- **~** at a word's start: HOME, or SYS: when HOME is not set.
- **{a,b,c}** makes three words: `cp x.{c,h} RAM:`. `{1..5}` counts,
  `{a..e}` goes through letters.
- **\* ? [abc]** match file names (`echo *.txt`), as in bash. A
  pattern nothing matches is left as it is. `#?` is never matched by
  csh: it goes to the program.

A word given to an Amiga command is quoted the Amiga way when it had
quotes. `KEY=value` stays as it is, for the program's template.

## VARIABLES
csh's variables are AmigaDOS local variables: `Get`, `$name` in Amiga
scripts and every program see them. `export` makes one global (ENV:).

    name=value       set (no spaces around =)
    name+=more       add to the end
    X=1 cmd          X is 1 only while cmd runs
    $name ${name}    the value; unknown is empty
    ${name:-word}    word when name is empty or unset
    ${name:=word}    the same, and name is set to word
    ${name:+word}    word when name is set and not empty
    ${name:?text}    stop with text when name is empty
    ${#name}         the length
    ${name#pat}      the shortest start matching pat taken off
    ${name##pat}     the longest start taken off
    ${name%pat}      the shortest end taken off
    ${name%%pat}     the longest end taken off
    ${name/pat/new}  the first match made new
    ${name//pat/new} every match made new
    ${name/#pat/new} ${name/%pat/new}   only at the start, the end
    ${name:2}        from character 2 (counted from 0)
    ${name:2:3}      3 characters from character 2
    ${name^} ${name^^}   the first, all letters in capitals
    ${name,} ${name,,}   the first, all letters small
    ${!name}         the variable whose name is in name

Without the colon (`${name-word}`) only an unset name counts as empty.

Special variables:

    $1 $2 ...        the arguments of a script or function
    $# $@ $*         how many; all of them ("$@" one word each)
    $0               the script's or function's name
    $?               the return code of the last command
    $$               the Shell's number
    $-               the options on (e f u x, i at the prompt)
    $RC $Result2     as in the ROM Shell
    $RANDOM          a random number, 0 to 32767
    $SECONDS         seconds since csh started
    $PIPESTATUS      an array: each command's code in the last pipe
    $BASH_REMATCH    an array: what [[ x =~ re ]] matched
    $REPLY           what read or select read
    $PWD $OLDPWD     the current and the last drawer
    $CMD_DURATION    how long the last typed line took, in ms
    $COLUMNS         the window's width

## ARRAYS
    a=(one two three)     an array
    a[5]=six              one element
    a+=(seven)            add to the end
    ${a[0]} $a            the first element
    ${a[@]} ${a[*]}       all elements ("${a[@]}" one word each)
    ${#a[@]}              how many
    ${!a[@]}              the indexes
    ${a[@]:1:2}           two elements from element 1
    declare -A m          an array with names: m[red]=1, ${m[red]}

## ARITHMETIC
`$((...))` gives a number, `((...))` is a command (true when the result
is not 0), and `let` is the same as `((...))`. Inside, variables need
no `$`. Whole numbers only (32 bits).

    + - * / % **          sums, remainder, power
    = ++ -- += -= *= /=   change a variable (also %= &= |=
                          ^= <<= >>=)
    < <= > >= == !=       compare (1 or 0)
    && || !               and, or, not
    & | ^ ~ << >>         bits
    a ? b : c             b when a is not 0, else c
    ,                     one after another
    16#ff 2#101 010       hex, binary, octal (a leading 0)

## COMMANDS
    cmd1; cmd2            one, then the other
    cmd1 && cmd2          cmd2 only when cmd1 succeeded (code 0)
    cmd1 || cmd2          cmd2 only when cmd1 failed
    ! cmd                 true and false swapped
    cmd1 | cmd2           cmd1's output into cmd2's input
    cmd1 |& cmd2          the error output into the pipe too
    cmd &                 in the background (see JOBS in csh)
    { cmd1; cmd2; }       a group: one redirection for all
    ( cmd1; cmd2 )        a subshell: variables, aliases, functions,
                          the current drawer and options it changes
                          are put back after it
    $(cmd) `cmd`          the output of cmd, as words
    <(cmd)                the name of a T: file with cmd's output

A code of 0 is true, any other is false, so an Amiga command's 5 (WARN)
is false too. Each command of a pipe runs at the same time, joined by
PIPE:. Ctrl+C reaches every command of the pipe.

## REDIRECTION
    < file       input from file
    > file       output to file (>| too)
    >> file      output added to the end of file
    <> file      file for input and output
    2> file      the error output to file (2>> to add)
    *> *>>       the same, the Amiga's way
    &> file      output and errors to file (&>> to add)
    2>&1         errors where the output goes
    >&2          output to the error output (the window)
    <<END        a here-document: the lines up to END as input
    <<-END       the same, tabs at the line starts taken off
    <<'END'      the same, with no $ worked out
    <<<word      word as the input

## FLOW
- **if** list; **then** list; [**elif** list; **then** list;]
  [**else** list;] **fi**
  runs the first list whose test succeeded.
- **while** list; **do** list; **done**
  repeats while the test succeeds; **until** while it fails.
- **for** name [**in** words]; **do** list; **done**
  once for each word (the arguments without `in`). A pattern in the
  words gives the files it matches. `{ list; }` may stand for `do list;
  done`.
- **for ((** a; b; c **))**; **do** list; **done**
  a once, then list while b is not 0, with c after each round.
- **case** word **in** pattern [| pattern] **)** list **;;** ... **esac**
  runs the list of the first pattern that matches word.
- **select** name [**in** words]; **do** list; **done**
  shows the words numbered, asks with PS3 (`#? `), sets name to the
  word chosen and REPLY to what was typed. An empty line shows the
  menu again; `break` leaves.
- **[[** test **]]**
  a test without word splitting: `==` and `!=` compare with a pattern
  (`[[ $f == *.txt ]]`), `=~` with a regular expression, `<` `>`
  compare text; `-eq` and the others compare numbers; `-nt` `-ot`
  newer, older file; `-v name` set; `&&` `||` `!` `( )` join tests;
  and the file tests of `test`.
- **name()** { list; }
  defines a function (also `function name { list; }`). It is called
  like a command; its arguments are `$1`, `$2` ... inside, and `local`
  makes variables of its own.

## BUILT-IN COMMANDS
Where a name is also an Amiga command, the Amiga one keeps its place
for its own forms, as noted below.

- **echo** [-neE] [word ...]
  prints the words. -n no new line at the end, -e escapes
  (`\n` `\t` `\e` `\0nnn` `\xHH`, `\c` stops), -E none. With NOLINE,
  FIRST, LEN or TO it is the Amiga's Echo.
- **printf** [-v NAME] FORMAT [arg ...]
  prints the arguments in FORMAT: `%s %b %c %d %i %u %x %X %o %f %e
  %g %%`, with flags `- 0 + #`, a width and a precision. The format is
  used again while arguments are left. -v puts the result in NAME.
- **read** [-r] [-a NAME] [-p PROMPT] [name ...]
  reads a line: one word into each name, the rest into the last; no
  name: into REPLY. -r keeps backslashes, -a puts each word in the
  array NAME, -p shows PROMPT first. False at the end of the input.
- **mapfile** [-t] [NAME]
  reads every line of the input into the array NAME (MAPFILE). -t
  takes the line ends off. **readarray** is the same.
- **test** expr, **\[** expr **]**
  true or false: `-z s` empty, `-n s` not empty, `a = b`, `a == b`,
  `a != b` text, `-eq -ne -lt -le -gt -ge` numbers, `-e` exists, `-f`
  a file, `-d` a drawer, `-r -w -x` may be read, written, run (the
  protection bits), `-s` a file that is not empty; `!` not, `-a` and,
  `-o` or, `( )`.
- **true**, **:** do nothing, successfully. **false** returns 1.
- **cd** [DRAWER | - | ~]
  changes the current drawer. Alone: HOME (else SYS:). `-` goes back
  to the last one and prints it. `..` is the parent (`/`). Only in
  small letters: `CD` is the Amiga's.
- **pwd** prints the current drawer.
- **pushd** [DRAWER]
  goes to DRAWER and remembers the drawer it left; alone it swaps the
  two. Up to 16 are remembered.
- **popd** goes back to the drawer pushd remembered last.
- **dirs** prints the current drawer and the remembered ones.
- **export** name[=value] ...
  makes the variables global (ENV:), so other Shells see them.
- **unset** [-f] [-v] name ...
  removes variables; with -f functions.
- **local** name[=value] ...
  inside a function: variables of its own, gone when it returns.
- **declare** [-aAfFgprx] [name[=value] ...]
  sets variables: -a an array, -A an array with names, -r read-only,
  -x export, -g global even inside a function. -p prints variables, -f
  functions, -F function names. Alone it prints the arrays and
  read-only variables. **typeset** is the same.
- **readonly** name[=value] ...
  variables that can no longer be changed.
- **set** [-efuxH] [+efuxH] [-o name] [--] [word ...]
  options: -e stop a script when a command fails, -x show each command
  before it runs, -u an unset variable is an error, -f no file name
  matching, -H the `!` history words (on). + turns one off. -o takes
  the long names errexit, xtrace, nounset, noglob, pipefail (a pipe
  fails when any command in it fails), histexpand; `set -o` alone
  shows them. Words after the options become `$1`, `$2` ... `set name
  value` sets a variable, as the Amiga's Set; `set` alone is the
  Amiga's (it lists the variables).
- **alias** name=text ...
  defines aliases (Amiga aliases: the ROM Shell sees them too). `[]`
  in the text is where the arguments go. Without `=` it is the
  Amiga's Alias.
- **source** file [arg ...], **.** file [arg ...]
  runs a csh file in this csh, so its variables and functions stay.
- **eval** word ...
  runs the words as a csh line. Only in small letters: `Eval` is the
  Amiga's sum command.
- **exit** [n]
  ends csh, or the script, with code n (the last code without n).
- **return** [n]
  ends a function or a sourced file.
- **break** [n], **continue** [n]
  leave a loop, or go on with its next round; n loops out. Outside a
  loop, `break` is the Amiga's Break.
- **shift** [n]
  drops the first n arguments ($2 becomes $1).
- **getopts** optstring name [arg ...]
  reads a script's options one at a time, as bash's: the letter into
  name, its value into OPTARG, OPTIND counts.
- **let** expr ...
  arithmetic, as `((...))`.
- **trap** [command] [EXIT|INT|ERR|RETURN]
  runs command when csh or the script ends (EXIT), on Ctrl+C (INT),
  when a command fails (ERR), when a function returns (RETURN). `trap
  - INT` or `trap INT` puts it back; `trap` alone lists them. HUP,
  TERM and other Unix names are taken but never happen.
- **history** lists the history, numbered.
- **time** command
  runs the command, then prints how long it took (`real 1.24s`) in the
  window. Only in small letters.
- **jobs** lists the background jobs: Running, or Done with its code.
- **fg** [%n]
  waits for job n (the newest without it) and gives its code.
- **kill** [%n ...]
  sends Ctrl+C to every process of the jobs (the newest without %n).
  Any other argument (a Shell's number, a name) goes to the kill
  command.
- **wait** [%n]
  waits for job n, or for all jobs. `wait 5` (a number) is the
  Amiga's Wait.

## EXAMPLES
    for f in *.c; do echo "$f"; done
    n=0; while [ $n -lt 3 ]; do echo $n; n=$((n + 1)); done
    if [[ $name == *.lha ]]; then echo "an archive"; fi
    List RAM: | grep -i info | sort
    greet() { echo "Hello, ${1:-you}"; }
    case $answer in y|yes) echo ok;; *) echo no;; esac
    files=(*.txt); echo "${#files[@]} files"
    Copy Work:big RAM: >NIL: &

A script, with the s bit set (`Protect myscript +s`):

    #!csh
    for d in "$@"; do
        [ -d "$d" ] || { echo "not a drawer: $d"; continue; }
        echo "$d: $(ls "$d" | wc -l) entries"
    done

## RETURN CODES
A built-in gives 0 for success and 1 for false (`false`, a test that
does not hold). A wrong use (a missing argument, a bad option) gives 10,
the Amiga's ERROR; 20 when memory runs out.

## NOTES
csh's keywords (`if`, `for`, `done` ...) are in small letters. Amiga
commands may be typed in any case.

What csh does not have yet: Ctrl+Z, `bg`, `$!`, `disown`, `exec`,
`command`, `type` (Which says where a command is), `printf %q`,
`case ;&`, `read -n -t -s`, `declare -i` (taken but not kept), `**/`
and extglob patterns.

## SEE ALSO
`csh`, `cshell`, `grep`, `ls`
