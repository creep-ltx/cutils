$ rm -rf wh
? 0
$ MakeDir wh wh/certs
? 0
$ wget -nv -P wh https://127.0.0.1:8472/hello.txt
wget: https://127.0.0.1:8472/hello.txt: no certificates to check with: AmiSSL: is not assigned (its Certs drawer holds them); --no-check-certificate skips the check
? 10
$ wget -nv --no-check-certificate -P wh https://127.0.0.1:8472/hello.txt
https://127.0.0.1:8472/hello.txt -> 'wh/hello.txt' [39]
? 0
$ cat wh/hello.txt
hello, world
from the wget test server
? 0
$ wget -nv --ca-certificate=wget-cert.pem -P wh https://127.0.0.1:8472/hello.txt
wget: https://127.0.0.1:8472/hello.txt: certificate not trusted: IP address mismatch (--no-check-certificate skips the check)
? 10
$ wget -nv --ca-certificate=wget-cert.pem -P wh https://localhost:8472/chunked.txt
https://localhost:8472/chunked.txt -> 'wh/chunked.txt' [39]
? 0
$ wget -nv --ca-directory=wh/certs https://localhost:8472/hello.txt
wget: https://localhost:8472/hello.txt: certificate not trusted: self-signed certificate (--no-check-certificate skips the check)
? 10
$ wget -nv --ca-certificate=nothing.pem https://localhost:8472/hello.txt
wget: https://localhost:8472/hello.txt: cannot read the certificates of --ca-certificate
? 10
$ wget -q --no-check-certificate -P wh https://127.0.0.1:8472/data.bin
? 0
$ diff -q wh/data.bin wget-data.bin
? 0
$ head -c 30000 wget-data.bin >wh/c.bin
? 0
$ wget -c -nv --no-check-certificate -O wh/c.bin https://127.0.0.1:8472/data.bin
https://127.0.0.1:8472/data.bin -> 'wh/c.bin' [70000]
? 0
$ diff -q wh/c.bin wget-data.bin
? 0
$ wget -nv --no-check-certificate -O - http://127.0.0.1:8471/to-https
hello, world
from the wget test server
https://127.0.0.1:8472/hello.txt -> '-' [39]
? 0
$ wget -q --no-check-certificate -O - https://127.0.0.1:8472/chunked.txt https://127.0.0.1:8472/nolength.txt
hello, world
from the wget test server
hello, world
from the wget test server
hello, world
from the wget test server
hello, world
from the wget test server
? 0
$ wget -nv --no-check-certificate https://127.0.0.1:8472/missing
wget: https://127.0.0.1:8472/missing: 404 Not Found
? 10
$ wget -nv -t 1 --no-check-certificate https://127.0.0.1:8471/hello.txt
wget: https://127.0.0.1:8471/hello.txt: TLS handshake failed: wrong version number
? 10
$ rm -rf wh
? 0
