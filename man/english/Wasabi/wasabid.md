# wasabid - the Wasabi daemon: remote control of this Amiga

## SYNOPSIS
    wasabid [PORT] [name NAME] [allow RANGE ...] [allow any] [plain]

## DESCRIPTION
wasabid is the Amiga half of Wasabi. It waits on the network for the
`wasabi` client on a PC (Linux, macOS, Windows) and does what it asks:
copy files both ways, run commands and send back their output and exit
code, take screenshots, move the mouse, type, click gadgets and menus,
send ARexx, and stream the debug output and DOS calls while your
programs run.

It runs on AmigaOS 2.04 and later, on any 68000 or better, and needs a
TCP/IP stack with `bsdsocket.library` (Roadshow, AmiTCP, or the lwIP
stack on a PiStorm). It takes about 350 KB of memory. `wasabid` runs on
any 68k; `wasabid.020` is the same for a 68020 or better.

To install it, copy it to C:, choose a key and start it:

    Copy wasabid.020 C:wasabid
    SetEnv SAVE wasabi.key choose-a-key
    Run >NIL: C:wasabid

Put the last two lines in S:User-Startup to have it at every boot. On
OS 2.x `SetEnv` has no SAVE: use `SetEnv wasabi.key choose-a-key` and
`Copy ENV:wasabi.key ENVARC:` instead, and put only the Run line in
S:User-Startup.

Arguments are plain words and may come in any order.

## OPTIONS
- **PORT** listen on another port (1 to 65535). The default is 1234.
- **name** NAME
  the name this Amiga gives when the client looks for Amigas on the
  network, so two Amigas can be told apart. Without it the name is
  taken from ENV:HOSTNAME, else it is `amiga`. Spaces become dashes.
- **allow** RANGE
  also answer this range of addresses, written as an address and a
  number of bits, for example `100.64.0.0/10` for Tailscale. May be
  given several times.
- **allow any** answer every address, even from the open internet.
  wasabid warns loudly at startup. Do not leave it like this.
- **plain** let in clients without encryption too, even when
  encryption is ready.

Two more words are used by `wasabi update` when it tests a new daemon:
`--selftest` and `trial`. They are not for everyday use.

## THE KEY
The key is the variable `wasabi.key` (ENV:wasabi.key). Every
connection must give the same key, or it is refused ("bad key"). The
key is read once, when wasabid starts. If no key is set, the startup
line says `NO KEY SET`.

## WHO MAY CONNECT
wasabid answers only addresses that cannot be reached from the
internet: 10.x.x.x, 172.16.x.x to 172.31.x.x, 192.168.x.x, this machine
(127.x.x.x) and link-local addresses, plus the ranges given with
**allow**. Anyone else is closed before a word is read.

Refused addresses are counted per address in `L:wasabid.refused`. The
count survives a restart, and the client shows it when it has risen.

## ENCRYPTION
Encryption (TLS 1.3) is used when this Amiga has **AmiSSL 5** and its
keys. The keys are made on the PC with `wasabi keys init` and put on
the Amiga with `wasabi keys amiga NAME`. The Amiga keeps them in
`ENVARC:wasabi/` (`ca.crt`, `amiga.crt`, `amiga.key`). Each end accepts
only certificates signed by your own keys. AmiSSL takes about 3.5 MB
more memory while encryption is on.

- **When encryption is ready**, wasabid accepts only encrypted
  clients. A client without keys is told why and refused, unless
  wasabid was started with **plain**.
- **When it is not ready** (no AmiSSL, its assign not made yet at boot,
  or no keys), wasabid still runs and lets plain clients in. It says
  why at startup and in its answer to `wasabi discover`, and tries
  again every 30 seconds.
- **From this machine itself** (127.x, as with FS-UAE or an SSH
  tunnel) a client may always connect without encryption, so an
  emulator needs neither AmiSSL nor keys.

The key in ENV:wasabi.key is checked either way.

## STOPPING IT
- **Break C**: `Break NUMBER C`, with the number of its process (see
  `Status`). It says "wasabid: stopping". A command it is still running
  is waited for first.
- From the PC: `wasabi quit --yes`. After that someone must be at the
  Amiga to start it again.
- `wasabi restart` from the PC reloads it in place.
  `wasabi update FILE` replaces it with a new version, checked first
  (the old one is kept as C:wasabid.bak).

## FILES
- **ENV:wasabi.key** the key (keep it in ENVARC: too).
- **ENVARC:wasabi/** the encryption keys: ca.crt, amiga.crt, amiga.key.
- **L:wasabid.refused** the count of refused addresses.
- **T:wasabi-run-N** the output of a running command, while it runs.

## EXAMPLES
    Run >NIL: C:wasabid
    Run >NIL: C:wasabid name A1200
    Run >NIL: C:wasabid allow 100.64.0.0/10
    Run >NIL: C:wasabid 1235
    Run >NIL: C:wasabid plain

## RETURN CODES
0 when it stops. 20 when it cannot start: no `bsdsocket.library` (the
TCP/IP stack is not up), or not enough memory. When the port is taken
it says "cannot listen on port N" and stops.

## NOTES
wasabid runs any command it is sent, as whoever started it, on a
machine with no memory protection. Keep the key secret, and never
forward its port from your router to the internet.

Discovery: wasabid also answers the client's search for Amigas, on the
same port number (UDP). It answers anyone on the local network, but
gives away no more than a port scan would.

It opens an ARexx port named `WASABI`, so Amiga scripts can type,
click and pick menus too.

## SEE ALSO
`wasabi` (the client on the PC), the Wasabi README and docs/security.md
