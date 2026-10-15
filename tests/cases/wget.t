$ rm -rf wg wi SYS:T/cuw{S}
? 0
$ MakeDir wg wi SYS:T/cuw{S} SYS:T/cuw{S}/c SYS:T/cuw{S}/n
? 0
$ wget -V
wget 0.1b1 (15.10.2026)
? 0
$ wget
wget: missing URL (wget ? for the usage)
? 10
$ wget -nv -P wg http://127.0.0.1:8471/hello.txt
http://127.0.0.1:8471/hello.txt -> 'wg/hello.txt' [39]
? 0
$ cat wg/hello.txt
hello, world
from the wget test server
? 0
$ date -r wg/hello.txt "+%Y-%m-%d %H:%M:%S"
2020-05-17 10:20:30
? 0
$ wget -nv -P wg http://127.0.0.1:8471/hello.txt
http://127.0.0.1:8471/hello.txt -> 'wg/hello.txt.1' [39]
? 0
$ wget -nv -P wg/ 127.0.0.1:8471/hello.txt
http://127.0.0.1:8471/hello.txt -> 'wg/hello.txt.2' [39]
? 0
$ wget -nc -P wg http://127.0.0.1:8471/hello.txt
File 'wg/hello.txt' already there; not retrieving.
? 0
$ wget -q -P wg http://127.0.0.1:8471/data.bin
? 0
$ diff -q wg/data.bin wget-data.bin
? 0
$ wget -q -P SYS:T/cuw{S} http://127.0.0.1:8471/big.bin
? 0
$ wc -c SYS:T/cuw{S}/big.bin
3145728 SYS:T/cuw{S}/big.bin
? 0
$ wget -q -O SYS:T/cuw{S}/cb.bin http://127.0.0.1:8471/chunked-big.bin
? 0
$ wget -q -O - http://127.0.0.1:8471/chunked.txt
hello, world
from the wget test server
? 0
$ wget -q -O - http://127.0.0.1:8471/nolength.txt
hello, world
from the wget test server
hello, world
from the wget test server
hello, world
from the wget test server
? 0
$ wget -q -O wg/both.txt http://127.0.0.1:8471/chunked.txt http://127.0.0.1:8471/query?two
? 0
$ cat wg/both.txt
hello, world
from the wget test server
asked: two
? 0
$ wget -nv -P wg http://127.0.0.1:8471/redir/1
http://127.0.0.1:8471/hello.txt -> 'wg/1' [39]
? 0
$ cat wg/1
hello, world
from the wget test server
? 0
$ wget -nv http://127.0.0.1:8471/loop
wget: http://127.0.0.1:8471/loop: more than 20 redirections
? 10
$ wget -nv -P wg http://127.0.0.1:8471/missing
wget: http://127.0.0.1:8471/missing: 404 Not Found
? 10
$ find wg -name missing
? 0
$ wget --spider -nv http://127.0.0.1:8471/hello.txt
http://127.0.0.1:8471/hello.txt: exists
? 0
$ wget --spider -nv http://127.0.0.1:8471/missing
wget: http://127.0.0.1:8471/missing: does not exist (404 Not Found)
? 5
$ wget -nv -O - http://127.0.0.1:8471/flaky/a{S}/2
third time lucky
http://127.0.0.1:8471/flaky/a{S}/2 -> '-' [17]
? 0
$ wget -nv -t 1 http://127.0.0.1:8471/flaky/b{S}/1
wget: http://127.0.0.1:8471/flaky/b{S}/1: 500 Internal Server Error
? 10
$ wget -nv -t 2 http://127.0.0.1:8471/gone500
wget: http://127.0.0.1:8471/gone500: 503 Service Unavailable
wget: giving up after 2 tries
? 10
$ wget -nv -t 2 http://127.0.0.1:8479/nobody
wget: http://127.0.0.1:8479/nobody: connection refused
wget: giving up after 2 tries
? 10
$ wget -nv http://no.such.host.invalid/
wget: http://no.such.host.invalid/: unable to resolve host address 'no.such.host.invalid'
? 10
$ head -c 1000000 SYS:T/cuw{S}/big.bin >SYS:T/cuw{S}/c/big.bin
? 0
$ wget -c -nv -P SYS:T/cuw{S}/c http://127.0.0.1:8471/big.bin
http://127.0.0.1:8471/big.bin -> 'SYS:T/cuw{S}/c/big.bin' [3145728]
? 0
$ wget -c -P SYS:T/cuw{S}/c http://127.0.0.1:8471/big.bin *>wg/again.txt
? 0
$ grep -c "already fully retrieved" wg/again.txt
1
? 0
$ wc -c SYS:T/cuw{S}/c/big.bin
3145728 SYS:T/cuw{S}/c/big.bin
? 0
$ head -c 1000 SYS:T/cuw{S}/big.bin >SYS:T/cuw{S}/n/noresume.bin
? 0
$ wget -c -nv -P SYS:T/cuw{S}/n http://127.0.0.1:8471/noresume.bin
http://127.0.0.1:8471/noresume.bin -> 'SYS:T/cuw{S}/n/noresume.bin' [3145728]
? 0
$ wget -nv -P SYS:T/cuw{S} http://127.0.0.1:8471/cut/k1{S}
http://127.0.0.1:8471/cut/k1{S} -> 'SYS:T/cuw{S}/k1{S}' [3145728]
? 0
$ wget -nv -t 1 -T 2 -O wg/drip http://127.0.0.1:8471/drip
wget: http://127.0.0.1:8471/drip: read timed out
? 10
$ wc -c wg/drip
10 wg/drip
? 0
$ wget -nv --content-disposition -P wg http://127.0.0.1:8471/disp
http://127.0.0.1:8471/disp -> 'wg/report 2026.txt' [11]
? 0
$ wget -nv -P wg "http://127.0.0.1:8471/space%20name.txt"
http://127.0.0.1:8471/space%20name.txt -> 'wg/space name.txt' [7]
? 0
$ cat "wg/space name.txt"
spaced
? 0
$ wget -nv -P wg "http://127.0.0.1:8471/query?x=1&y=2"
http://127.0.0.1:8471/query?x=1&y=2 -> 'wg/query' [15]
? 0
$ cat wg/query
asked: x=1&y=2
? 0
$ wget -nv -P wg http://127.0.0.1:8471/dir/
http://127.0.0.1:8471/dir/ -> 'wg/index.html' [19]
? 0
$ wget -q -O - -U "Test/1.0" --header "X-One: 1" "--header=X-Two: two" http://127.0.0.1:8471/headers
Accept-Encoding: identity
Accept: */*
Connection: close
Host: 127.0.0.1:8471
User-Agent: Test/1.0
X-One: 1
X-Two: two
? 0
$ wget -q -O - http://127.0.0.1:8471/headers | grep Agent
User-Agent: wget/0.1b1 (AmigaOS; CUtils)
? 0
$ wget -q -O - --header "Host: other.example" http://127.0.0.1:8471/headers | grep Host
Host: other.example
? 0
$ wget -q -O - http://user:secret@127.0.0.1:8471/auth
welcome
? 0
$ wget -nv -O - http://127.0.0.1:8471/auth
wget: http://127.0.0.1:8471/auth: 401 Unauthorized
? 10
$ wget -S -nv -O NIL: http://127.0.0.1:8471/hello.txt *>wg/s.txt
? 0
$ grep -E "^  (HTTP|Content-|Last)" wg/s.txt
  HTTP/1.1 200 OK
  Content-Type: text/plain
  Last-Modified: Sun, 17 May 2020 10:20:30 GMT
  Content-Length: 39
? 0
$ wget -nv -o wg/log.txt http://127.0.0.1:8471/missing
? 10
$ cat wg/log.txt
wget: http://127.0.0.1:8471/missing: 404 Not Found
? 0
$ Echo >wi/urls "# two files*Nhttp://127.0.0.1:8471/chunked.txt*N*N  http://127.0.0.1:8471/data.bin  "
? 0
$ wget -nv -i wi/urls -P wi
http://127.0.0.1:8471/chunked.txt -> 'wi/chunked.txt' [39]
http://127.0.0.1:8471/data.bin -> 'wi/data.bin' [70000]
? 0
$ wget -q http://127.0.0.1:8471/missing -P wi http://127.0.0.1:8471/hello.txt
? 10
$ wc -c wi/hello.txt
39 wi/hello.txt
? 0
$ wget -r http://127.0.0.1:8471/
wget: -r: not supported
? 10
$ wget --mirror http://127.0.0.1:8471/
wget: --mirror: not supported
? 10
$ wget -np http://127.0.0.1:8471/
wget: -np: not supported
? 10
$ wget --bogus http://127.0.0.1:8471/
wget: --bogus: unknown option
? 10
$ wget ftp://127.0.0.1/file
wget: ftp://127.0.0.1/file: only http:// and https:// are supported
? 10
$ wget -q -t x http://127.0.0.1:8471/
wget: x: not a number
? 10
$ rm -rf wg wi
? 0
