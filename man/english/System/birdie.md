# Birdie - put patterns into the window borders on Workbench

## SYNOPSIS
    Birdie PATTERN [PATTERN ...] [ACTIVEPATTERN] [ACTIVECOLOR RRGGBB]
           [IMPOSELEVEL N] [PUBSCREENS] [EXCLUDE pattern] [PRECISION N]
           [NOBITMAPS] [NOICONBORDER] [FLOODMASKMWB] [NOLIGHTING]
           [NODRAW] [LRA n] [LGA n] [LBA n] [DRA n] [DGA n] [DBA n]

## DESCRIPTION
A system patch (a *hack*) that fills the borders and title bars of the
windows on the Workbench screen with pictures (patterns) or colour
gradients. Each window gets one of the patterns at random; the active
window can get a pattern or a tint of its own. On high and true colour
screens the border edges get a light and shadow version of the pattern.

Pictures are loaded with datatypes, so any picture format with a datatype
works. Instead of a file, a gradient may be given as `gx.RRGGBB.RRGGBB`
(hexadecimal start and end colour) or `gd.R-G-B.R-G-B` (decimal);
gradients only work on high or true colour screens.

Start it after IPrefs, with `Run <>NIL: Birdie ...`. Switch off other
patches that redraw window borders (such as MCP's QuickDraw).

## ARGUMENTS
- **PATTERN** 1 to 20 pictures or gradients. Without any that can be
  loaded, Birdie does nothing.
- **ACTIVEPATTERN** the first pattern is for the active window only;
  the others go to inactive windows.
- **ACTIVECOLOR** RRGGBB
  tint the active window's pattern with this colour instead (high or
  true colour; uses twice the memory).
- **IMPOSELEVEL** N
  how strong the tint is, in percent (50 by default).
- **PUBSCREENS** work on all public screens, not only Workbench.
- **EXCLUDE** PATTERN
  leave the public screens whose names match alone
  (`EXCLUDE (Workbench|MyScreen)`).
- **PRECISION** N
  colour matching precision on AGA, from 32 (rough, saves pens) to -1
  (exact, the default).
- **NOBITMAPS** leave the images drawn into borders (usually the system
  gadgets) alone.
- **NOICONBORDER** draw Workbench icons without a border and with
  colour 0 transparent. Not together with NODRAW or NOBITMAPS.
- **FLOODMASKMWB** with NOICONBORDER: for icons of 16 colours or fewer,
  make colour 0 transparent only where it touches the edge.
- **NOLIGHTING** plain white and black edges instead of light and
  shadow (high or true colour; saves memory).
- **NODRAW** do not patch graphics' Draw(); for debugging.
- **LRA** N
  how much to add to red for the light edges (60 by default); **LGA**
  and **LBA** the same for green and blue.
- **DRA** N
  the same for the shadow edges (-60 by default); **DGA** and **DBA**
  for green and blue.

## EXAMPLES
    Run <>NIL: Birdie Work:Patterns/marble.iff
    Run <>NIL: Birdie act.png inact1.png inact2.png ACTIVEPATTERN
    Run <>NIL: Birdie gx.303040.101018 ACTIVECOLOR 4060A0
    Run <>NIL: Birdie stone.iff NOICONBORDER FLOODMASKMWB

## NOTES
Birdie 1.5 (1998) by Trond Werner Hansen (Tronan). This page follows the
documentation of the later Birdie 2000 (Aminet `util/wb/birdie2000`,
Birdie.doc), whose options are a superset: REDRAWLINE, REDRAWBITMAPS and
MINDEPTH there do not exist in 1.5. If VisualPrefs is used with Birdie's
NOBITMAPS, set VisualPrefs/BirdieMode (see `VisualPrefs`).

On this A1200 the Birdie line in S:User-Startup is switched off (a
comment).

## SEE ALSO
`VisualPrefs`
