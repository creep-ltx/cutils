# PencamTool - take pictures with an STV680 USB webcam (Aiptek Pencam)

## SYNOPSIS
    PencamTool TO [PICNUM] [INTERVAL] [UPTO N] [NOBEEP] [GAMMA G]
               [SHARPEN] [TEXT T] [FONT F] [FONTSIZE N] [UNIT N]

## DESCRIPTION
Reads a picture from a USB webcam built on the STV680 chip (USB vendor
0x0553, product 0x0202: the Aiptek Pencam family and some other cheap
cameras) and saves it as a true colour PPM file (Portable Anymap; the
NetPBM tools on Aminet convert it to other formats).

It can take a new snapshot, fetch a picture stored in the camera's memory,
or keep taking pictures at an interval. White balance, gamma correction
and a sharpening filter can be applied, and a line of text can be written
into the bottom of the picture.

## ARGUMENTS
- **TO** the file to write. Needed. It may contain a number format
  such as `%ld` or `%04ld` (the `l` is needed), which becomes the
  picture's number, for INTERVAL and UPTO.
- **PICNUM** N
  fetch picture N from the camera's memory (0 is the first). Without
  it, a new snapshot is taken. A number with no picture gives garbage.
- **INTERVAL** N
  keep taking pictures, one every N ticks (50 ticks are a second).
  Only sensible without PICNUM. Ctrl-C stops.
- **UPTO** N
  several pictures in one go: from PICNUM up to picture N; or with
  INTERVAL, stop at picture N. Put a number format in TO, or every
  picture overwrites the last.
- **NOBEEP** no beep when a picture has been downloaded.
- **GAMMA** G
  apply white balance and gamma correction with the gamma value G, a
  decimal number; 0.45 is a good value. 1.0 gives white balance only.
- **SHARPEN** apply a 5x5 sharpening filter.
- **TEXT** T
  write the text T along the bottom of the picture (cut off if too
  long).
- **FONT** F
  the font for TEXT, such as `xen.font`; the system font if not given.
- **FONTSIZE** N
  the font's size in pixels.
- **UNIT** N
  which camera, when several are connected (0 is the first).

## EXAMPLES
    PencamTool RAM:snap.ppm
    PencamTool RAM:snap.ppm 0 GAMMA 0.45 SHARPEN
    PencamTool RAM:movie%04ld.ppm INTERVAL 0 GAMMA 0.5
    PencamTool RAM:shot%03ld.ppm 0 UPTO 79 GAMMA 0.45 SHARPEN
    PencamTool RAM:cam.ppm GAMMA 0.45 TEXT "My cam" FONT xen.font
               FONTSIZE 8 NOBEEP

## NOTES
Part of Poseidon, the USB stack by Chris Hodges (this tool is version 1.7
from 2008). The manual `HELP:Poseidon.guide` also describes a VHI Studio
driver for the same cameras.

## SEE ALSO
`SonixcamTool`, `PsdDevLister`
