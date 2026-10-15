# PsdLoadModule - make a library, device or program resident in memory

## SYNOPSIS
    PsdLoadModule NAME [NAME ...] [QUIET] [REBOOT] [REMOVE] [INIT]
                  [CHECK] [FAST] [LMPRI N] [AUTOINIT N] [CLASS N]
                  [VERSION N] [TYPE N] [PRI N]

## DESCRIPTION
Installs a module (a file with a resident tag: a library, a device, a
resource) so that it survives a reset and is started by Exec at boot, the
same way the modules in the Kickstart ROM are. That lets a newer driver be
in place before the disk boots, or before the system on disk would load
it.

It is Torbjorn Andersson's LoadModule 1.1, renamed by Poseidon so it does
not clash with the other LoadModule commands. Poseidon used it to install
itself before booting; on this A1200 the startup uses it for
`DEVS:input.device`, the input.device that came with Poseidon.

## ARGUMENTS
- **NAME** the module file or files; patterns work. With REMOVE, the
  name alone is enough; `""` means every module PsdLoadModule has
  installed.
- **QUIET** do not say that a module was installed.
- **REBOOT** reset the machine after installing, so the module starts.
  Without it, it starts at the next reset.
- **REMOVE** take an installed module off the list again. The memory
  it used is only given back by a reset.
- **INIT** also start the module now, not only after a reset.
- **CHECK** at every boot, check that the module's memory is unchanged,
  and show an alert if not. Not for modules that write to themselves.
- **FAST** load the module into the best memory there is (usually fast
  memory) instead of chip memory.
- **LMPRI** N
  priority of the small boot module that CHECK and FAST add. By default
  just below intuition.library, so alerts can be shown.
- **AUTOINIT** N
  0 clears, 1 sets the RTF_AUTOINIT flag of the module's resident tag.
- **CLASS** N
  other flags for the resident tag: 1 coldstart, 2 single task, 4 after
  DOS (add them up).
- **VERSION** N
  write N as the module's version.
- **TYPE** N
  write N as the module's type (3 device, 8 resource, 9 library).
- **PRI** N
  write N as the module's start priority.

The options after NAME have no effect on modules already installed (except
REMOVE). INIT and the last five can crash the machine if used wrongly.

## EXAMPLES
    PsdLoadModule DEVS:input.device QUIET
    PsdLoadModule LIBS:poseidon.library FAST CHECK
    PsdLoadModule "" REMOVE

## NOTES
LoadModule 1.1 (1997) by Torbjorn A. Andersson, freeware, Aminet
`util/boot/LoadModule11`; included with Poseidon by permission. Thomas
Richter's LoadModule (also on Aminet) works the other way round: it
reboots unless given NOREBOOT; PsdLoadModule reboots only with REBOOT.

## SEE ALSO
`Resident`, `PsdStackLoader`
