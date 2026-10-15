# PdaTransfer - back up, restore and install files on a Palm PDA

## SYNOPSIS
    PdaTransfer BACKUP|RESTORE DIR DRAWER [DEVICE D] [UNIT N]
                [MAXBAUD B]
    PdaTransfer INSTALL|MERGE FILE NAME.prc|.pdb [...]
    PdaTransfer FETCH|DELETE NAME DATABASE [...]
    PdaTransfer LIST|PURGE [...]

## DESCRIPTION
Exchanges data with a Palm OS handheld through pdalink.library: backs up
all its databases into a drawer and puts them back, installs programs and
databases (.prc and .pdb files), fetches or deletes a single database, and
lists them.

After starting it, press the HotSync button on the PDA (the message
"Please press the HotSync button" asks for it). It talks over a serial
device, by default serial.device unit 0. A USB Palm works through
Poseidon's palmpda.class, which offers usbpalm.device: set the variable
PDALINK_DEVICE to usbpalm.device (see NOTES).

## ARGUMENTS
One of the first eight says what to do:
- **BACKUP** copy all databases into the drawer DIR. Databases that
  have not changed since the last backup are not read again, and files
  of databases deleted on the PDA are deleted from the drawer.
- **RESTORE** put all database files from DIR back on the PDA.
- **INSTALL** install the .prc or .pdb file FILE, replacing a database
  of the same name.
- **MERGE** install FILE, merging it into an existing database of the
  same name.
- **FETCH** read the database NAME into a .prc or .pdb file.
- **DELETE** delete the database NAME from the PDA.
- **LIST** list all databases, in RAM and in ROM.
- **PURGE** remove the records marked as deleted from the PDA's RAM.

The rest:
- **DIR** DRAWER
  the backup drawer, for BACKUP and RESTORE. **D** is the short form.
- **FILE** NAME
  the .prc or .pdb file, for INSTALL and MERGE. **F** for short.
- **NAME** DATABASE
  the database's name on the PDA (upper and lower case matter), for
  FETCH and DELETE. **N** for short.
- **DEVICE** D
  the serial device. **SD** for short.
- **UNIT** N
  the serial unit. **SU** for short.
- **MAXBAUD** B
  the fastest speed to try, 57600 by default. **SB** for short.

## EXAMPLES
    PdaTransfer BACKUP DIR Work:PalmBackup
    PdaTransfer RESTORE DIR Work:PalmBackup SD duart.device SU 1
    PdaTransfer INSTALL FILE RAM:game.prc
    PdaTransfer DELETE NAME "ExampleDB"
    PdaTransfer LIST

## NOTES
PdaTransfer 1.1 (2000) by Richard Koerber, part of PdaLink (Aminet
`util/libs/PdaLink`, whose guide documents it). An older Poseidon archive
included it together with a version of pdalink.library that can use
usbpalm.device; see `HELP:Poseidon.guide`, "palmpda.class". The default
device, unit and speed can be set with the variables PDALINK_DEVICE,
PDALINK_UNIT and PDALINK_RATE:

    SetEnv PDALINK_DEVICE usbpalm.device SAVE

## SEE ALSO
`PsdDevLister`
