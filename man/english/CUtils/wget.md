# wget - fetch files from the web, over http and https

## SYNOPSIS
    wget [-cqSV] [-nv] [-nc] [-O FILE] [-P DIR] [-t N] [-T SECS]
         [-U AGENT] [--header "Name: value"] [-i FILE] [-o LOGFILE]
         [--spider] [--content-disposition] [--no-check-certificate]
         URL ...

## DESCRIPTION
wget downloads what each URL points to and saves it as a file in the
current drawer, named after the last part of the URL: `wget
https://aminet.net/util/arc/untgz.lha` makes `untgz.lha`. A URL that
ends in `/` gives `index.html`; `%20` and the like in the name are turned
back into the characters they stand for. When a file of that name is
already there, the new one is called `name.1`, then `name.2` and so on,
as GNU wget does (not with -O, -c or -nc).

It speaks HTTP/1.1, plain (`http://`) and encrypted (`https://`, with
AmiSSL 5). A URL without `http://` in front is taken as http. It follows
redirections (up to 20, also to another server or from http to https),
takes chunked answers and answers without a length, and sends a
`user:password@` in the URL as a login (Basic). The file gets the date
the server gives for it (Last-Modified), turned from GMT into the
Amiga's local time (the zone as for date).

Something that goes wrong on the way - a lost connection, a server that
stays silent for -T seconds, an answer 500 and up - is tried again (3
tries in all), going on from where it stopped when the server allows.
An answer in the 400s (404 Not Found, 401 Unauthorized ...) is said and
not tried again. A failed URL never stops the others.

While a file comes in and the messages go to a window, one line is
redrawn in place: the name, how far it is in percent, a bar, the bytes
so far, the speed and the time left. When the server does not say how
big the file is, a marker moves to and fro instead. The line fits the
window (`$COLUMNS` when set, else the console's own width). When the
messages go to a file, a line per MB instead.

**Ctrl-C** stops at once. The part already fetched is kept, so `wget -c`
with the same URL goes on from there later.

Options may come before or after the URLs. `wget ?` shows the usage.

## OPTIONS
- **-O** FILE
  everything into FILE (several URLs one after another). `-O -` writes
  it to the output, for a pipe.
- **-P** DIR
  save the files in DIR, which is made when it is missing.
- **-c** continue: a file that is partly there is finished, asking the
  server only for the rest (Range). A server that cannot do that sends
  it all again; a file that is already whole is left alone.
- **-nc** never fetch a URL whose file is already there.
- **-q** quiet: nothing at all is said, only the return code tells.
- **-nv** less: one line per file (`URL -> 'name' [bytes]`) and the
  errors.
- **-S** show the headers of the server's answers.
- **-t** N
  how many tries (3; 0 = no limit).
- **-T** SECS
  how long a connection may stay silent before it counts as lost (30;
  0 = wait for ever).
- **-U** AGENT
  what wget calls itself to the server (User-Agent). Normally
  `wget/0.1b1 (AmigaOS; CUtils)`.
- **--header** LINE
  an extra line for the request, `"Name: value"`, as often as needed
  (`"--header=Name: value"` with the quotes round it all). A Host or
  User-Agent given this way replaces wget's own.
- **-i** FILE
  the URLs from FILE, one a line; `#` starts a comment. `-` is the
  input.
- **-o** LOGFILE
  the messages into LOGFILE instead of the window.
- **--spider** only ask whether the URL is there (HEAD); nothing is
  saved. Return code 5 when it is not.
- **--content-disposition** use the name the server suggests for the
  file, when it suggests one.
- **--no-check-certificate** https without checking the server's
  certificate.
- **--ca-certificate**=FILE
  trust the certificates in FILE (PEM) instead of AmiSSL's.
- **--ca-directory**=DIR
  trust the certificates in DIR (hashed names, as AmiSSL:Certs).
- **-V** the version.

Long forms: --output-document --directory-prefix --continue --quiet
--no-verbose --no-clobber --tries --timeout --user-agent
--server-response --input-file --output-file --version;
`--name=value` or `--name value`. The rarer GNU wget options (-r, -m,
-N, -k, -p, cookies, --post-data, --limit-rate ...) answer "not
supported".

## EXAMPLES
    wget https://aminet.net/util/arc/untgz.lha
    wget -P RAM: https://aminet.net/comm/tcp/curl-8.22.0.lha
    wget -O Work:readme.txt http://example.com/README
    wget -c https://aminet.net/comm/tcp/curl-8.22.0.lha
    wget -q -O - http://example.com/list.txt | grep lha
    wget -nv -i Work:downloads.txt -P Work:Downloads
    wget --spider https://aminet.net/util/arc/untgz.lha

## RETURN CODES
0 fine, 5 --spider found nothing there (an answer in the 400s), 10 a
failure (no such host, refused, 404, a certificate not trusted, a write
error, bad arguments; with several URLs the worst one), 20 stopped with
Ctrl-C.

## NOTES
**https** needs AmiSSL 5 (amisslmaster.library and AmiSSL's library in
LIBS:), opened only when an https URL comes up: plain http works without
it. AmiSSL wants about 4 MB of fast RAM. Without it wget says "https
needs AmiSSL 5".

**Certificates**: the server's certificate is checked against AmiSSL's
own store, the drawer `AmiSSL:Certs` that the AmiSSL installer fills
(and keeps up to date with each new AmiSSL), plus `AmiSSL:UserCerts`
for certificates you add yourself. The name of the server is checked
too. A certificate that does not pass stops the download with the
reason ("certificate has expired", "hostname mismatch", "self-signed
certificate" ...). The Amiga's clock must be right, or every
certificate looks expired or not yet valid. Without an `AmiSSL:` assign
wget says so; `--no-check-certificate` skips the check (the connection
is still encrypted, but anyone in between could read it).

On a slow Amiga (a 68030 at 50 MHz) setting up an https connection takes
some seconds; a few big servers hang up when it takes too long.

The name of the file comes from the URL as given, also after a
redirection (as GNU wget does without --trust-server-names).

## SEE ALSO
`tar`, `gunzip`, `lha`
