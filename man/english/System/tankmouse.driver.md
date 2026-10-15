# TankMouse.driver - make the TankMouse's scroll wheel work

## SYNOPSIS
    TankMouse.driver [MMB] [VREV] [HREV]

## DESCRIPTION
The TankMouse is a modern mouse for the Amiga's mouse port with a scroll
wheel. Its wheel movements arrive as extra signals on the port, which the
system does not understand by itself; TankMouse.driver reads them and
turns them into the wheel events that programs know (NewMouse style), for
the wheel's forward and backward turns. It speaks the same signals as the
Cocolino adapter's driver, a subset of them.

It runs until stopped with Ctrl-C; on exit it prints some statistics. Run
only one copy at a time.

## ARGUMENTS
- **MMB** use the middle mouse button. **M** for short. (Not in the
  driver's documentation; the meaning is taken from its messages.)
- **VREV** turn the wheel's direction round (up and down). **V**.
- **HREV** turn the sideways direction round. **H**. (Not documented
  either; the TankMouse itself scrolls only up and down.)

## EXAMPLES
    Run <>NIL: TankMouse.driver
    TankMouse.driver VREV

On this A1200 S:User-Startup starts it as `RUN <>NIL: C:TankMouse.driver`.

## NOTES
TankMouse.driver 0.3 (2024) by Szymon Bieganski, Aminet
`driver/input/TankMouse`; source at github.com/sq7bti/AmigaMouse (branch
tankmouse). Its installer adds it to S:User-Startup; remove a Cocolino
driver there first if there is one. FreeWheel (Aminet
`util/mouse/FreeWheel`) can fine-tune the wheel, and RawKeys shows whether
its events arrive.

## SEE ALSO
`Run`, `Break`
