# netdev-stats - show the network driver's counters and link state

## SYNOPSIS
    netdev-stats [INTERVAL S] [DEVICE name] [UNIT N] [COUNTERS]
    netdev-stats RXUSECS US RXFRAMES N TXFRAMES N

## DESCRIPTION
Asks the network driver (by default genet.device unit 0, the Raspberry Pi
4 / CM4's own Ethernet under Emu68) for its statistics: packets and bytes
sent and received, errors and dropped packets, packets lost inside the
driver, and the link: up or down, its speed in Mb/s and duplex.

With INTERVAL it keeps printing, every S seconds, until Ctrl-C. With
COUNTERS it shows the driver's own full list of counters instead; for
genet.device that is every statistic the network chip keeps, the same set
Linux shows with `ethtool -S`.

It can also tune *interrupt coalescing* while the network runs: how long,
and for how many packets, the hardware waits before interrupting the
Amiga. Fewer interrupts cost less CPU time, at the price of a little
delay.

## ARGUMENTS
- **INTERVAL** S
  print again every S seconds (1 or more). Not with the coalescing
  values.
- **DEVICE** NAME
  the driver to ask (genet.device if not given).
- **UNIT** N
  its unit (0 if not given).
- **COUNTERS** show the driver's full counter list.
- **RXUSECS** US
  receive: interrupt at the latest after US microseconds.
- **RXFRAMES** N
  receive: interrupt after N packets.
- **TXFRAMES** N
  send: interrupt after N packets sent.

RXUSECS, RXFRAMES and TXFRAMES must be given together. genet.device's
defaults are 500 microseconds, 64 and 32 frames.

## EXAMPLES
    netdev-stats
    netdev-stats INTERVAL 1
    netdev-stats COUNTERS
    netdev-stats RXUSECS 500 RXFRAMES 64 TXFRAMES 32

## NOTES
Part of lwip-amiga, the TCP/IP stack in the Emu68 driver stack 2.0 by
rondoval (github.com/rondoval/emu68-driver-stack); see the READMEs of
lwip-amiga and emu68-genet-driver-netdev. It works with drivers that speak
lwip-amiga's "netdev" interface, which today means genet.device 4.x only.
The coalescing values at start come from the driver's settings
(RX_COALESCE_USECS, RX_COALESCE_FRAMES, TX_COALESCE_FRAMES); whether a
change made here outlives a restart is not documented.

## SEE ALSO
`netinfo`, `mdns`, `ping`
