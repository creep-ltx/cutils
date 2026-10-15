# ping - check that another computer answers over the network

## SYNOPSIS
    ping [COUNT N] [INTERVAL S] [TIMEOUT S] [SIZE N] [LOAD N]
         [NUMERIC] [ONEREPLY] [QUIET] [VERBOSE] [RECORDROUTE]
         [DONTROUTE] [DEBUG] [BELL] HOST

## DESCRIPTION
Sends small ICMP "echo request" packets to HOST, a name or an IP address,
and prints a line for each answer: its size, its sequence number and how
long the round trip took. Stop it with Ctrl-C (or give COUNT or TIMEOUT);
it then prints a summary: packets sent and received, the percentage lost,
and the shortest, average and longest round trip.

It is the first test when the network does not work: ping the Amiga's own
address, then the router, then a computer further away.

Every keyword also has a Unix-style short form, so `ping -c 4 host` works
as well as `ping COUNT 4 host`.

## ARGUMENTS
- **COUNT** N
  stop after N answers. **-c** N
- **INTERVAL** S
  wait S seconds between packets (1 by default). **-i** S
- **TIMEOUT** S
  stop after S seconds, however many answers came. **-t** S
- **SIZE** N
  send N bytes of data in each packet (56 by default, which makes 64
  bytes with the ICMP header). **-s** N
- **LOAD** N
  first send N packets as fast as possible. **-l** N
- **NUMERIC** print addresses only, never look up names. **-n**;
  **NUMERICONLY** is another name.
- **ONEREPLY** stop at the first answer. **-o**
- **QUIET** only the first line and the summary. **-q**
- **VERBOSE** also show other ICMP packets that arrive. **-v**
- **RECORDROUTE** ask the routers on the way to note themselves in the
  packet, and show that route (many ignore it; room for nine). **-R**
- **DONTROUTE** do not use the routing table: send straight to a host
  on a network the Amiga is directly connected to.
- **DEBUG** set the SO_DEBUG option on the socket. **-d**
- **BELL** print a bell character for each answer (the screen flashes
  or a sound plays).
- **HOST** the computer to ping: a name or an address. Needed.

## EXAMPLES
    ping 192.168.1.1
    ping COUNT 4 www.aminet.net
    ping -c 3 -n 192.168.1.1
    ping ONEREPLY TIMEOUT 5 192.168.1.10

## NOTES
ping 4.13 (2017) from Roadshow, the TCP/IP stack by Olaf Barthel; its
reference is Roadshow's ping.doc. On this A1200 it runs on top of the
lwip-amiga bsdsocket.library from the Emu68 driver stack, not on Roadshow
itself; RECORDROUTE and DONTROUTE have not been checked there.

## SEE ALSO
`netinfo`, `mdns`, `netdev-stats`
