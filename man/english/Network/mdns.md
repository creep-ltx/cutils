# mdns - look up .local names, and announce services, by multicast DNS

## SYNOPSIS
    mdns NAME
    mdns LISTEN [SECS N]
    mdns STATUS
    mdns ADD _type._proto PORT N [INSTANCE name]
    mdns DEL SLOT

## DESCRIPTION
Multicast DNS (what Apple calls Bonjour and Linux Avahi) lets the
computers on a home network find each other by name, as `name.local`,
without any DNS server. The Amiga's network stack (lwip-amiga) answers for
its own name; mdns is the tool for the rest:

- **mdns NAME** finds the address of a `.local` name on the network.
- **LISTEN** shows what the other machines announce, for a while.
- **STATUS** shows what this Amiga announces about itself.
- **ADD** announces a service running on the Amiga (DNS-SD), so other
  computers see it, for example an FTP server; **DEL** withdraws one.

Services announced from the start are listed in ENVARC:netstack.prefs
(MDNS_SERVICE lines); ADD and DEL change the list while the stack runs,
which suits daemons that start after the network.

## ARGUMENTS
- **NAME** the name to look up, such as `pi.local`.
- **LISTEN** watch the announcements on the network.
- **SECS** N
  how long LISTEN watches, in seconds (10 by default).
- **STATUS** list what this machine announces, with slot numbers.
- **ADD** TYPE
  announce a service of TYPE, written `_service._protocol`, such as
  `_ftp._tcp` or `_http._tcp`. Needs PORT.
- **PORT** N
  the port the service listens on.
- **INSTANCE** NAME
  the name the service is shown under (optional).
- **DEL** SLOT
  withdraw the service in this slot (as STATUS shows it).

## EXAMPLES
    mdns pi.local
    mdns LISTEN SECS 30
    mdns STATUS
    mdns ADD _ftp._tcp PORT 21
    mdns ADD _http._tcp PORT 80 INSTANCE "A1200 web"
    mdns DEL 1

## NOTES
Part of lwip-amiga, the TCP/IP stack in the Emu68 driver stack 2.0 by
rondoval (github.com/rondoval/emu68-driver-stack); see its README. It only
works with that stack's bsdsocket.library. Setting `MDNS = no` in
ENVARC:netstack.prefs turns multicast DNS off altogether.

## SEE ALSO
`netinfo`, `ping`, `netdev-stats`
