#!/usr/bin/env bash
# wgettest.sh - wget's cases. Starts tests/httpd.py (http 8471, https 8472
# on 127.0.0.1, which FS-UAE's emulated bsdsocket reaches as the PC), runs
# `S=${S:-g} tests/run.sh wget` on W (default ./w, the private FS-UAE),
# checks the 3 MB downloads byte for byte on the PC (the 8 MB machine has
# no room for diff on two of them), then the https cases (wget-https.t) on
# W2 (default ./w030, a 68030 with 32 MB: AmiSSL's 3.5 MB library does not
# fit in the stock machine's fast RAM beside other sessions' tests; start
# it with tests/uae030.sh start). Stops the server at the end.
cd "$(dirname "$0")"
S=${S:-g}; export S
W=${W:-./w}; W2=${W2:-./w030}
python3 httpd.py 8471 8472 >/tmp/wgettest-httpd.log 2>&1 &
pid=$!
trap 'kill $pid 2>/dev/null' EXIT
for i in $(seq 50); do
  (exec 3<>/dev/tcp/127.0.0.1/8471) 2>/dev/null && break
  sleep 0.2
done
bad=0
W=$W ./run.sh wget || bad=1
# the big downloads, byte for byte (the same pattern as httpd.py's BIG)
n=0; fails=0
for f in big.bin cb.bin c/big.bin n/noresume.bin "k1$S"; do
  n=$((n + 1))
  rm -f /tmp/wgettest/got.bin
  $W get "SYS:T/cuw$S/$f" /tmp/wgettest/got.bin >/dev/null 2>&1
  if ! python3 -c '
import sys
big = bytes((i * 7 + (i >> 9)) & 255 for i in range(3 * 1024 * 1024))
sys.exit(open(sys.argv[1], "rb").read() != big)' /tmp/wgettest/got.bin 2>/dev/null; then
    echo "FAIL big file SYS:T/cuw$S/$f differs from the server's"; fails=1
  fi
done
$W run "Delete >NIL: SYS:T/cuw$S ALL QUIET" >/dev/null 2>&1
[ $fails = 0 ] && echo "ok   big files ($n compared byte for byte)" || bad=1
if $W2 ping >/dev/null 2>&1; then
  $W2 run "MakeDir >NIL: RAM:cut$S" >/dev/null 2>&1
  $W2 put /tmp/wgettest/cert.pem "RAM:cut$S/wget-cert.pem" >/dev/null || exit 2
  W=$W2 ./run.sh wget-https || bad=1
else
  echo "skip cases/wget-https.t: no 32 MB emulator at $W2 (tests/uae030.sh start)"
fi
exit $bad
