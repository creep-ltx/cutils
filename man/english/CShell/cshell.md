# cshell - csh as the Shell of every Shell window

## SYNOPSIS
    Protect C:cshell +p
    Resident shell C:cshell REPLACE

## DESCRIPTION
cshell is csh built as the system's own Shell. Once it is made the
resident "shell", every new Shell is csh: NewShell, the Shell icon,
`Run`, and the command lines other programs run with System(). Each
one speaks csh's language at the prompt and runs Amiga scripts by the
Amiga's rules, as the ROM Shell does.

It is the same program as C:csh, from the same source, made pure: one
copy in memory serves every Shell window at once.

The boot is not changed. The Startup-Sequence and S:User-Startup still
run in the ROM Shell, and the ROM Shell stays in Kickstart.

## OPTIONS
cshell takes no arguments: it is started by AmigaDOS, never typed as a
command. To start csh by hand, type `csh` (see `csh`).

## INSTALLING
Copy cshell to C:, give it the p (pure) bit once, and make it the
resident shell:

    Copy cshell C:
    Protect C:cshell +p
    Resident shell C:cshell REPLACE

From then on every new Shell is csh, until the next reboot. Shells that
are already open keep the shell they have.

Install C:csh of the same version with it. When csh's own commands (a
loop, a function) run in the background with `&`, cshell starts C:csh
to run them, and an older C:csh would not understand what it is given.

## AT EVERY BOOT
To make cshell the Shell after every boot, put these lines at the start
of S:User-Startup:

    ;BEGIN CShell
    If EXISTS C:cshell
       Resident shell C:cshell REPLACE
    EndIf
    ;END CShell

The If keeps the boot working when C:cshell has been removed.

## WHAT A NEW SHELL DOES
A new Shell window first runs S:Shell-Startup by the Amiga's rules, as
the ROM Shell does. At its first prompt it runs S:CShell-Startup in the
csh language: your aliases, functions and PROMPT go there. The keys,
settings and history are csh's (see `csh`).

A Shell started in the background (Run, System()) never shows a prompt;
it runs its commands and ends.

`exit` or `EndShell` closes the Shell window.

## GOING BACK
To have the ROM Shell back in new windows:

- **by hand** if you typed the Resident line yourself, reboot.
- **at every boot** if the lines are in S:User-Startup, take them
  out (or rename C:cshell, which the If then skips) and reboot.

## EXAMPLES
Try it until the next reboot, from RAM:

    Copy cshell RAM:
    Protect RAM:cshell +p
    Resident shell RAM:cshell REPLACE
    NewShell

## RETURN CODES
A Shell that a program runs with System() and waits for gives back the
return code of the last command it ran, as the ROM Shell does.

## NOTES
cshell is also made as a ROM module, `cshell-ext.rom`: a second ROM for
the FS-UAE emulator. With it, every Shell after the boot is csh without
a Resident line; the boot itself still runs in the ROM Shell, and the
Kickstart file is not changed. It is meant for the emulator; on a real
Amiga use `Resident` as above.

A program that starts a Shell and gives it lines (for example a remote
control daemon) also gets cshell. Those lines are read as an Amiga
script, so they behave as they did in the ROM Shell.

## SEE ALSO
`csh`, `csh-builtins`, Resident, NewShell
