#!/usr/bin/env python3
"""httpd.py [HTTP_PORT HTTPS_PORT] - the test server for wget's cases
(tests/cases/wget.t, started by tests/wgettest.sh). http on 8471, https
on 8472 with a self-signed certificate made at start in /tmp/wgettest/
(cert.pem: subjectAltName DNS:localhost only, so https://127.0.0.1 fails
the name check even when the certificate is trusted).

Endpoints:
  /hello.txt          a short text, Last-Modified 2020-05-17 10:20:30 GMT
  /data.bin           tests/files/wget-data.bin (the cases diff it)
  /big.bin            3 MB, made from a fixed pattern; Range supported
  /chunked.txt        the hello text in uneven chunks (with an extension)
  /chunked-big.bin    big.bin, chunked (16 KB and odd sizes)
  /nolength.txt       no Content-Length, ends when the connection closes
  /redir/1            302 to "2" (relative), 301 to an absolute URL, 307 to /hello.txt
  /to-https           302 to https://127.0.0.1:HTTPS/hello.txt
  /loop               302 to itself, for ever
  /missing            404
  /gone500            503 every time
  /flaky/KEY/N        500 the first N times for this KEY, then a text
  /cut/KEY            big.bin, but the first answer stops after 1 MB
  /noresume.bin       big.bin, Range never honoured (200)
  /drip               says 100 bytes, sends 10 and goes quiet for 40 s
  /disp               Content-Disposition: attachment; filename="report 2026.txt"
  /space%20name.txt   a name with a space
  /query?x=1&y=2      the name before ? is "query"
  /dir/               index.html
  /headers            the request's headers, as the body
  /auth               needs Basic user:secret (401 otherwise)
  /slow.bin           1 MB of big.bin in 16 KB pieces, 0.15 s apart (to watch the
                      progress line); /slow-chunked.bin the same without a length
"""
import base64, os, socket, ssl, subprocess, sys, threading, time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HTTP = int(sys.argv[1]) if len(sys.argv) > 1 else 8471
HTTPS = int(sys.argv[2]) if len(sys.argv) > 2 else 8472
HERE = os.path.dirname(os.path.abspath(__file__))
TMP = '/tmp/wgettest'

HELLO = b'hello, world\nfrom the wget test server\n'
BIG = bytes((i * 7 + (i >> 9)) & 255 for i in range(3 * 1024 * 1024))
DATA = open(os.path.join(HERE, 'files', 'wget-data.bin'), 'rb').read()
counts = {}
lock = threading.Lock()


class H(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *a):
        pass

    def send(self, code, body=b'', headers=(), length=True, head=None):
        self.send_response(code)
        for k, v in headers:
            self.send_header(k, v)
        if length:
            self.send_header('Content-Length', str(len(body)))
        self.send_header('Connection', 'close')
        self.end_headers()
        if self.command != 'HEAD':
            self.wfile.write(body)

    def ranged(self, data, honour=True, ctype='application/octet-stream'):
        r = self.headers.get('Range')
        if honour and r and r.startswith('bytes=') and r.endswith('-'):
            start = int(r[6:-1])
            if start >= len(data):
                return self.send(416, b'', [('Content-Range', f'bytes */{len(data)}')])
            return self.send(206, data[start:], [('Content-Type', ctype),
                             ('Content-Range', f'bytes {start}-{len(data) - 1}/{len(data)}')])
        self.send(200, data, [('Content-Type', ctype)])

    def chunked(self, data, sizes):
        self.send_response(200)
        self.send_header('Transfer-Encoding', 'chunked')
        self.send_header('Connection', 'close')
        self.end_headers()
        i, k = 0, 0
        while i < len(data):
            n = sizes[k % len(sizes)]
            k += 1
            part = data[i:i + n]
            ext = ';x=1' if k == 2 else ''
            self.wfile.write(b'%x%s\r\n' % (len(part), ext.encode()) + part + b'\r\n')
            i += n
        self.wfile.write(b'0\r\nX-Trailer: yes\r\n\r\n')

    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        p = self.path
        tls = isinstance(self.connection, ssl.SSLSocket)
        if p == '/hello.txt':
            return self.send(200, HELLO, [('Content-Type', 'text/plain'),
                             ('Last-Modified', 'Sun, 17 May 2020 10:20:30 GMT')])
        if p == '/data.bin':
            return self.ranged(DATA)
        if p == '/big.bin':
            return self.ranged(BIG)
        if p == '/chunked.txt':
            return self.chunked(HELLO, [5, 1, 17, 100])
        if p == '/chunked-big.bin':
            return self.chunked(BIG, [16384, 1000, 77777])
        if p == '/nolength.txt':
            self.send_response(200)
            self.send_header('Connection', 'close')
            self.end_headers()
            self.wfile.write(HELLO * 3)
            self.close_connection = True
            return
        if p == '/redir/1':
            return self.send(302, b'', [('Location', '2')])
        if p == '/redir/2':
            return self.send(301, b'', [('Location', f'http://127.0.0.1:{HTTP}/redir/3')])
        if p == '/redir/3':
            return self.send(307, b'', [('Location', '/hello.txt')])
        if p == '/to-https':
            return self.send(302, b'', [('Location', f'https://127.0.0.1:{HTTPS}/hello.txt')])
        if p == '/loop':
            return self.send(302, b'', [('Location', '/loop')])
        if p == '/gone500':
            return self.send(503, b'busy\n')
        if p.startswith('/flaky/'):
            _, _, key, n = p.split('/')
            with lock:
                counts[p] = counts.get(p, 0) + 1
                c = counts[p]
            if c <= int(n):
                return self.send(500, b'oops\n')
            return self.send(200, b'third time lucky\n')
        if p.startswith('/cut/'):
            with lock:
                counts[p] = counts.get(p, 0) + 1
                c = counts[p]
            if c == 1 and not self.headers.get('Range'):
                self.send_response(200)
                self.send_header('Content-Length', str(len(BIG)))
                self.send_header('Connection', 'close')
                self.end_headers()
                self.wfile.write(BIG[:1024 * 1024])
                self.wfile.flush()
                self.close_connection = True
                try:
                    self.connection.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass
                return
            return self.ranged(BIG)
        if p in ('/slow.bin', '/slow-chunked.bin'):
            self.send_response(200)
            if p == '/slow.bin':
                self.send_header('Content-Length', str(1024 * 1024))
            self.send_header('Connection', 'close')
            self.end_headers()
            for i in range(0, 1024 * 1024, 16384):
                self.wfile.write(BIG[i:i + 16384])
                self.wfile.flush()
                time.sleep(0.15)
            self.close_connection = True
            return
        if p == '/noresume.bin':
            return self.ranged(BIG, honour=False)
        if p == '/drip':
            self.send_response(200)
            self.send_header('Content-Length', '100')
            self.send_header('Connection', 'close')
            self.end_headers()
            self.wfile.write(b'0123456789')
            self.wfile.flush()
            time.sleep(40)
            return
        if p == '/disp':
            return self.send(200, b'the report\n', [('Content-Disposition', 'attachment; filename="report 2026.txt"')])
        if p == '/space%20name.txt':
            return self.send(200, b'spaced\n')
        if p.startswith('/query?'):
            return self.send(200, b'asked: ' + p.split('?', 1)[1].encode() + b'\n')
        if p == '/dir/':
            return self.send(200, b'<html>index</html>\n', [('Content-Type', 'text/html')])
        if p == '/headers':
            lines = [l for l in str(self.headers).split('\n') if l.strip()]
            body = ('\n'.join(sorted(lines)) + '\n').encode()
            return self.send(200, body)
        if p == '/auth':
            if self.headers.get('Authorization') == 'Basic ' + base64.b64encode(b'user:secret').decode():
                return self.send(200, b'welcome\n')
            return self.send(401, b'who are you\n', [('WWW-Authenticate', 'Basic realm="t"')])
        self.send(404, b'not here\n', [('Content-Type', 'text/plain')])


def cert():
    os.makedirs(TMP, exist_ok=True)
    c, k = os.path.join(TMP, 'cert.pem'), os.path.join(TMP, 'key.pem')
    if not os.path.exists(c):
        subprocess.run(['openssl', 'req', '-x509', '-newkey', 'ec', '-pkeyopt', 'ec_paramgen_curve:prime256v1',
                        '-nodes', '-keyout', k, '-out', c, '-days', '3650', '-subj', '/CN=localhost',
                        '-addext', 'subjectAltName=DNS:localhost'], check=True, capture_output=True)
    return c, k


def main():
    c, k = cert()
    s1 = ThreadingHTTPServer(('127.0.0.1', HTTP), H)
    s2 = ThreadingHTTPServer(('127.0.0.1', HTTPS), H)
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.load_cert_chain(c, k)
    s2.socket = ctx.wrap_socket(s2.socket, server_side=True)
    s1.daemon_threads = s2.daemon_threads = True
    threading.Thread(target=s2.serve_forever, daemon=True).start()
    print(f'httpd.py: http {HTTP}, https {HTTPS}', flush=True)
    s1.serve_forever()


main()
