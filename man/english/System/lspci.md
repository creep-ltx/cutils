# lspci - list the PCI Express devices of a Raspberry Pi 4 / CM4

## SYNOPSIS
    lspci [QUIET]

## DESCRIPTION
On a PiStorm with a Raspberry Pi 4 or Compute Module 4 running Emu68, the
Pi's PCI Express bus carries devices such as the VL805 USB 3 controller
(or, on a CM4 board, an NVMe drive). lspci asks bcmpcie.library for every
device found on that bus and prints, for each, its bus position, vendor
and device numbers and class, then the details: configuration registers,
capabilities, bridge windows and BARs (the memory areas assigned to it).

It is a diagnostic tool. If no device appears, the PCIe link did not come
up; a BAR address of zero means the device was not configured.

## ARGUMENTS
- **QUIET** one line per device instead of the details: bus position,
  vendor, device, class, which driver owns it, and its BARs. Good for
  scripts.

## EXAMPLES
    lspci
    lspci QUIET

A QUIET line looks like:

    1:00.0 vendor=1106 device=3483 class=0C:03:30 owner=- bar0=...

## NOTES
lspci 2.3 (2026), part of emu68-pcie-library in the Emu68 driver stack 2.0
by rondoval (github.com/rondoval/emu68-driver-stack); see that library's
README. It needs bcmpcie.library (and gic400.library) in LIBS:. "Failed to
open bcmpcie.library" means it is not installed.

## SEE ALSO
`Emu68Info`, `PsdDevLister`
