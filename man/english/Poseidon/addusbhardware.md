# AddUSBHardware - start or stop a USB controller driver in Poseidon

## SYNOPSIS
    AddUSBHardware DEVICE [UNIT] [QUIET] [REMOVE] [ALL]
    AddUSBHardware REMOVE ALL

## DESCRIPTION
Hands a USB host controller driver (a file in DEVS:USBHardware, such as
xhci.device) to the Poseidon USB stack by hand, or takes it away again.
Once a controller has been added, Poseidon looks for class drivers for
whatever is plugged into it, so the devices start working.

Normally nothing of this is typed: the stack is started at boot by
`PsdStackLoader`, with the controllers chosen in Trident (the Poseidon
preferences). AddUSBHardware is for testing a driver, or for bringing a
controller up or down without a reboot.

## ARGUMENTS
- **DEVICE** the controller driver: its path and name, for example
  `DEVS:USBHardware/xhci.device`. Needed, except for `REMOVE ALL`.
- **UNIT** N
  the unit of the driver, when it serves several controllers. 0 if not
  given.
- **QUIET** print nothing.
- **REMOVE** take the controller out of the stack instead of adding it.
  Give the same path and name that was used to add it.
- **ALL** every unit of the driver. With REMOVE and no DEVICE: every
  controller, which takes the whole USB stack offline.

## EXAMPLES
    AddUSBHardware DEVS:USBHardware/xhci.device
    AddUSBHardware DEVS:USBHardware/xhci.device 1 REMOVE
    AddUSBHardware REMOVE ALL

## NOTES
Part of Poseidon, the USB stack by Chris Hodges; on this machine Poseidon
6.0 for AmigaOS 3.2 (2026), which ships with the Emu68 driver stack.
Poseidon's own manual is `HELP:Poseidon.guide`.

Taking away the controller that the keyboard or mouse hang on takes them
away too.

## SEE ALSO
`AddUSBClasses`, `PsdStackLoader`, `PsdDevLister`, `PsdErrorlog`
