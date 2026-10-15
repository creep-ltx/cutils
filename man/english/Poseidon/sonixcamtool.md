# SonixcamTool - take pictures with a Sonix USB webcam

## SYNOPSIS
    SonixcamTool TO [INTERVAL] [UPTO N] [GAMMA G] [SHARPEN] [TEXT T]
                 [FONT F] [FONTSIZE N] [UNIT N]

## DESCRIPTION
Takes a picture with a USB webcam built on the Sonix Technologies chip set
(USB vendor 0x0C45) with an OV7630 sensor - the cheap cameras on a
bendable metal neck - and saves it as a true colour PPM file (Portable
Anymap; the NetPBM tools on Aminet convert it).

It can keep taking pictures at an interval. White balance, gamma
correction and a sharpening filter can be applied, and a line of text can
be written into the bottom of the picture.

These cameras need isochronous (real time) USB transfers. When the tool
was written, only the Deneb controller board could do them; with other
controllers it may not work.

## ARGUMENTS
- **TO** the file to write. Needed. It may contain a number format such
  as `%ld` or `%04ld` (the `l` is needed) for the picture number.
- **INTERVAL** N
  keep taking pictures, one every N ticks (50 ticks are a second).
  Ctrl-C stops.
- **UPTO** N
  with INTERVAL: stop after N pictures. Put a number format in TO.
- **GAMMA** G
  apply white balance and gamma correction with gamma G (a decimal
  number; 0.45 is good, 1.0 gives white balance only).
- **SHARPEN** apply a 5x5 sharpening filter.
- **TEXT** T
  write T along the bottom of the picture (cut off if too long).
- **FONT** F
  the font for TEXT; the system font if not given.
- **FONTSIZE** N
  the font's size in pixels.
- **UNIT** N
  which camera, when several are connected (0 is the first).

## EXAMPLES
    SonixcamTool RAM:snap.ppm
    SonixcamTool RAM:snap.ppm GAMMA 0.45 SHARPEN
    SonixcamTool RAM:shot%03ld.ppm INTERVAL 0 UPTO 79 GAMMA 0.45

## NOTES
Part of Poseidon, the USB stack by Chris Hodges (version 1.3 from 2008).
Described in `HELP:Poseidon.guide`.

## SEE ALSO
`PencamTool`, `PsdDevLister`
