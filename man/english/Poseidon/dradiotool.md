# DRadioTool - control a D-Link or GemTek USB FM radio

## SYNOPSIS
    DRadioTool [ON] [OFF] [FREQ KHZ] [SCAN] [AUTO] [SIGNAL] [UNIT N]

## DESCRIPTION
Switches a USB FM radio made by D-Link or GemTek on and off, tunes it, and
searches for stations. Only radios with USB vendor 0x04B4 and product
0x1002 are supported.

## ARGUMENTS
- **ON** switch the radio on.
- **OFF** switch the radio off.
- **FREQ** KHZ
  tune to this frequency, in kHz: from 87000 to 108000 (87 to 108 MHz).
- **SCAN** search for stations, from FREQ (or 87 MHz) up to 107 MHz.
  Each station found is printed; the last one found stays tuned.
  Ctrl-C stops the search.
- **AUTO** with SCAN: stop for three seconds at each station found;
  press Ctrl-C then to keep it.
- **SIGNAL** set the return code: 0 if a stereo station is heard on the
  current frequency, 5 if none. For a scan of one's own in a script.
- **UNIT** N
  which radio, when several are connected (0 is the first).

## EXAMPLES
    DRadioTool ON SCAN AUTO
    DRadioTool FREQ 104000
    DRadioTool OFF

## RETURN CODES
With SIGNAL: 0 a stereo station is heard, 5 no station. The other codes
are not documented.

## NOTES
Part of Poseidon, the USB stack by Chris Hodges (version 1.1 from 2003).
Described in `HELP:Poseidon.guide`.

## SEE ALSO
`PsdDevLister`
