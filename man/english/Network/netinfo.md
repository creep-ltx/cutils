# netinfo - show the Amiga's network settings at a glance

## SYNOPSIS
    netinfo

## DESCRIPTION
Prints the state of the network: the host name, and for each network
interface its name, driver and unit, IP address, netmask, broadcast
address, hardware (MAC) address, MTU, whether the link is up, whether the
address came by DHCP or is static, and the DNS servers.

It only reads; it changes nothing. The settings themselves live in
ENVARC:netstack.prefs. It takes no arguments.

## EXAMPLES
    netinfo

## NOTES
Part of lwip-amiga, the TCP/IP stack in the Emu68 driver stack 2.0 by
rondoval (github.com/rondoval/emu68-driver-stack); see its README. It asks
bsdsocket.library through Roadshow's interface query calls, which
lwip-amiga provides; "cannot open bsdsocket.library" means no network
stack is running.

## SEE ALSO
`netdev-stats`, `mdns`, `ping`
