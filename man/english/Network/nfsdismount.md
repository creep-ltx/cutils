# NFSDismount - cleanly stop an AmiNFSv3 network drive

## SYNOPSIS
    NFSDismount DEVICE

## DESCRIPTION
Stops the handler of an NFS drive mounted with AmiNFSv3 (for example
NFS0:), cleanly: the volume disappears from the Workbench and the handler
ends. The device itself stays known, so the next time something uses NFS0:
a fresh handler is started, loaded anew from L:nfs-handler. That is also
how a new version of the handler is put to use without a reboot.

It refuses while any file or lock on the drive is open: close the files
and windows on it first.

A restarted handler uses the same Startup line the device was first
mounted with; changed options in the DOSDriver file only take effect after
a reboot (see the AmiNFSv3 documentation).

## ARGUMENTS
- **DEVICE** the NFS device, with or without the colon (`NFS0:`).
  Needed.

## EXAMPLES
    NFSDismount NFS0:

## RETURN CODES
0 dismounted; 5 the device was not mounted (no handler running); 10 no
such device, the drive is in use, or another error (said).

## NOTES
Part of AmiNFSv3, an NFS version 3 file system for AmigaOS
(github.com/creep-ltx/AmiNFSv3); version 0.1 (2026). It sends the handler
ACTION_DIE, which AmiNFSv3's handler supports. For file systems built on
filesysbox.library use `FbxDismount`; the system's `Assign DISMOUNT` only
removes the name and leaves the handler running.

## SEE ALSO
`FbxDismount`, `Assign`, `Mount`
