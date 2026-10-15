# VisualPrefs - change the look of windows, gadgets and title bars

## SYNOPSIS
    Run >NIL: VisualPrefs

## DESCRIPTION
A system patch that lets the look of the Amiga's user interface be set in
great detail: the colours of active and inactive window edges, the
thickness of each window border, the height of title bars whatever the
font, the style of the system gadgets (also with your own pictures), the
3D frames of BOOPSI and GadTools gadgets, proportional gadgets, centred
window titles, pop-up cycle gadgets, and more - for all screens at once or
for each screen on its own.

VisualPrefs itself is only the patch; it takes no arguments. The settings
are made with the GUI preferences editor (SYS:Prefs/GUI).

It must be started in S:Startup-Sequence right before IPrefs
(S:User-Startup is too late), so it is in place before the Workbench
screen opens; a reboot is needed after installing it.

## ENVIRONMENT
A few special settings are read at boot from variables in ENV:VisualPrefs
(set them with `SetEnv`; its SAVE switch keeps them):
- **WBOpenDelay** 50ths of a second to wait before reopening Workbench
  after a change of look; against occasional lock-ups.
- **DOpusDelay** the same against display damage with Directory Opus
  and CyberGraphX.
- **BirdieMode** set to NOBITMAPS when Birdie runs with NOBITMAPS.
- **CopyGTText** 1: copy GadTools button labels, for programs that
  reuse one string for several gadgets.
- **SafetyLevel** 0, 1 or 2: how far to go against stack overflows in
  its patches (0 none, the default; 1 skip its own code when the stack
  is low; 2 switch to a larger stack).
- **MinStackSize** the stack size, 300 to 4000 bytes (default 2000),
  below which SafetyLevel acts.
- **Special** a number of flags added together, for odd cases (thicker
  XEN frames, fast memory with RTG, narrow close gadget, ...); see the
  guide.

## EXAMPLES
    Run >NIL: C:VisualPrefs
    SetEnv VisualPrefs/SafetyLevel 2 SAVE

On this A1200 S:Startup-Sequence starts it as `run >nil: c:visualprefs`.

## NOTES
VisualPrefs 41.52 (2021), 68040 build, by Massimo Tantignone. Its manual
is Work:System/VisualPrefs/VisualPrefs.guide (release 1.5k of 2001; Aminet
`util/wb/VisualPrefs`), which calls it shareware and describes what an
unregistered copy cannot do.

## SEE ALSO
`Birdie`, `StackAttack`
