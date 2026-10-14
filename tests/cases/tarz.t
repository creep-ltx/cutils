$ rm -rf tz
$ MakeDir tz tz/o tz/t
$ tar tf tar-small.tgz
s/one
s/two
s/three
? 0
$ tar tvf tar-small.tar.bz2
-rw-r--r-- 0/0             700 2020-01-02 03:04 s/one
-rw-r--r-- 0/0             300 2020-01-02 03:04 s/two
-rw-r--r-- 0/0               2 2020-01-02 03:04 s/three
? 0
$ tar tf tar-small.tar.xz
s/one
s/two
s/three
? 0
$ tar tf tar-small.tar.zst
s/one
s/two
s/three
? 0
$ tar tzf - <tar-small.tgz
s/one
s/two
s/three
? 0
$ tar xzvf tar-small.tgz -C tz/o
s/one
s/two
s/three
? 0
$ Type tz/o/s/three
3
$ tar xOf tar-small.tar.xz s/three
3
? 0
$ tar xjf tar-small.tar.zst -C tz/o s/two
? 0
$ wc -c tz/o/s/two
300 tz/o/s/two
$ Echo >tz/t/f "squeezed"
$ SetDate tz/t/f 01-Jan-20 10:00:00
$ tar czf tz/t.tgz tz/t
? 0
$ tar tf tz/t.tgz
tz/t/
tz/t/f
? 0
$ tar cjf tz/t.tbz tz/t
? 0
$ tar tvf tz/t.tbz tz/t/f
-rwxrwxrwx 0/0               9 2020-01-01 10:00 tz/t/f
? 0
$ tar caf tz/a.tar.gz tz/t
? 0
$ tar xOf tz/a.tar.gz tz/t/f
squeezed
? 0
$ tar caf tz/a.tar.bz2 tz/t
? 0
$ tar tf tz/a.tar.bz2
tz/t/
tz/t/f
? 0
$ tar cJf tz/x.txz tz/t
tar: tz/x.txz: this tar cannot pack xz yet
? 10
$ tar c --zstd -f tz/x.tzst tz/t
tar: tz/x.tzst: this tar cannot pack zstd yet
? 10
$ tar czf - tz/t >tz/s.tgz
? 0
$ tar tf - <tz/s.tgz
tz/t/
tz/t/f
? 0
$ rm -rf tz
