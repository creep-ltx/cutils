$ tar ?
tar -c|-x|-t [-vzjJakmOp] [-f ARCHIVE] [-C DRAWER] [--strip-components N] [--exclude PAT] [-T FILE] [name ...]
? 0
$ rm -rf tz
? 0
$ MakeDir tz tz/e tz/s tz/m tz/o
? 0
$ tar tf tar-gnu.tar
g/
g/big
g/empty/
g/fér
g/soft
g/sub/
g/sub/dddddddddddddddddddddddddddddddddddddddd/
g/sub/dddddddddddddddddddddddddddddddddddddddd/ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
g/sub/hard
g/sub/lsoft
g/sub/up
g/text
g/zero
? 0
$ tar tvf tar-gnu.tar
drwxr-xr-x 0/0               0 2020-01-02 03:04 g/
-rw-r--r-- 0/0         1019480 2020-01-02 03:04 g/big
drwxr-xr-x 0/0               0 2020-01-02 03:04 g/empty/
-rw-r--r-- 0/0               7 2020-01-02 03:04 g/fér
lrwxrwxrwx 0/0               0 2020-01-02 03:04 g/soft -> text
drwxr-xr-x 0/0               0 2020-01-02 03:04 g/sub/
drwxr-xr-x 0/0               0 2020-01-02 03:04 g/sub/dddddddddddddddddddddddddddddddddddddddd/
-rw-r--r-- 0/0               5 2020-01-02 03:04 g/sub/dddddddddddddddddddddddddddddddddddddddd/ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
-rw-r--r-- 0/0               6 2020-01-02 03:04 g/sub/hard
lrwxrwxrwx 0/0               0 2020-01-02 03:04 g/sub/lsoft -> target/tttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt
lrwxrwxrwx 0/0               0 2020-01-02 03:04 g/sub/up -> ../text
hrw-r--r-- 0/0               0 2020-01-02 03:04 g/text link to g/sub/hard
-rw-r--r-- 0/0               0 2020-01-02 03:04 g/zero
? 0
$ tar xf tar-gnu.tar -C tz
? 0
$ find tz/g -exec stat -c "%N|%F|%s|%A|%y" {} "+"
tz/g|drawer|0|----rwed|2020-01-02 03:04:05
tz/g/big|file|1019480|----rw-d|2020-01-02 03:04:05
tz/g/empty|drawer|0|----rwed|2020-01-02 03:04:05
tz/g/fér|file|7|----rw-d|2020-01-02 03:04:05
tz/g/soft -> text|soft link|0|----rwed|1978-01-01 00:00:00
tz/g/sub|drawer|0|----rwed|2020-01-02 03:04:05
tz/g/sub/dddddddddddddddddddddddddddddddddddddddd|drawer|0|----rwed|2020-01-02 03:04:05
tz/g/sub/dddddddddddddddddddddddddddddddddddddddd/ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff|file|5|----rw-d|2020-01-02 03:04:05
tz/g/sub/hard|file|6|----rw-d|2020-01-02 03:04:05
tz/g/sub/lsoft -> target/tttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt|soft link|0|----rwed|1978-01-01 00:00:00
tz/g/sub/up -> /text|soft link|0|----rwed|1978-01-01 00:00:00
tz/g/text|file|6|----rw-d|2020-01-02 03:04:05
tz/g/zero|file|0|----rw-d|2020-01-02 03:04:05
? 0
$ wc -c tz/g/big
1019480 tz/g/big
? 0
$ tail -n1 tz/g/big
0029127 abcdefghijklmnopqrstuvwxyz
? 0
$ Echo >>tz/g/sub/hard "more"
? 0
$ Type tz/g/text
hello
more
? 0
$ tar -xvf tar-ustar.tar -C tz
u/
u/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/
u/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb/
u/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb/cccccccccccccccccccccccccccccccccccccccccccccccccc
u/hard
u/ro
u/rw
u/rwx
u/soft
? 0
$ stat -c "%n %A %y" tz/u/rw tz/u/rwx tz/u/ro tz/u/hard
tz/u/rw ----rw-d 2020-01-02 03:04:05
tz/u/rwx ----rwed 2020-01-02 03:04:05
tz/u/ro ----r--- 2020-01-02 03:04:05
tz/u/hard ----rw-d 2020-01-02 03:04:05
? 0
$ stat -c %N tz/u/soft
tz/u/soft -> rw
? 0
$ Type tz/u/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb/cccccccccccccccccccccccccccccccccccccccccccccccccc
split by the prefix
? 0
$ tar tvf tar-pax.tar
drwxr-xr-x 0/0               0 2020-01-02 03:04 p/
-rw-r--r-- 0/0              10 2020-01-02 03:04 p/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
-rw-r--r-- 0/0               5 2020-01-02 03:04 p/amiga
-rwxr-xr-x 0/0               5 2020-01-02 03:04 p/nodel
-rw-r--r-- 0/0               9 2001-09-09 01:46 p/mtime
lrw-r--r-- 0/0               0 2020-01-02 03:04 p/lsoft -> ../zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
? 0
$ tar xvf tar-pax.tar -C tz
p/
p/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
p/amiga
p/nodel
p/mtime
p/lsoft
? 0
$ stat -c "%n|%A|%y|%C" tz/p tz/p/amiga tz/p/nodel tz/p/mtime
tz/p|----rwed|2020-01-02 03:04:05|a drawer note
tz/p/amiga|hsparwed|2020-01-02 03:04:05|Ein Kommentar ü
tz/p/nodel|----rwe-|2020-01-02 03:04:05|
tz/p/mtime|----rw-d|2001-09-09 01:46:40|
? 0
$ stat -c %N tz/p/lsoft
tz/p/lsoft -> /zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
? 0
$ find tz/p -name y#? -exec wc -c {} "+"
10 tz/p/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
? 0
$ tar xf tar-v7.tar -C tz
? 0
$ find tz/v -exec stat -c "%n|%F|%s|%A" {} "+"
tz/v|drawer|0|----rwed
tz/v/a|file|4|----rw-d
tz/v/dir|drawer|0|----rwed
tz/v/dir/b|file|8|----rw-d
? 0
$ tar xvf tar-evil.tar -C tz/e
tar: removing leading `/' from member names
abs.f
tar: removing leading `../' from member names
up.f
tar: removing leading `a/../../' from member names
x.f
tar: removing leading `Work:' from member names
vol.f
ok/dot.f
ok/twice.f
ok/co_lon.f
esc
esc/passwd
tar: esc: is a drawer, not replaced by a link
? 10
$ find tz/e -exec stat -c "%N|%F" {} "+"
tz/e|drawer
tz/e/abs.f|file
tz/e/esc|drawer
tz/e/esc/passwd|file
tz/e/ok|drawer
tz/e/ok/co_lon.f|file
tz/e/ok/dot.f|file
tz/e/ok/twice.f|file
tz/e/up.f|file
tz/e/vol.f|file
tz/e/x.f|file
? 0
$ Type tz/e/esc/passwd
not through the link
? 0
$ tar tf tar-bad.tar
s/one
tar: tar-bad.tar: damaged: a wrong header at block 3, skipping to the next one
s/three
? 10
$ tar tf tar-short.tar
s/one
s/two
tar: tar-short.tar: unexpected end of the archive
? 10
$ tar xf tar-short.tar -C tz/s
tar: tar-short.tar: unexpected end of the archive
? 10
$ find tz/s
tz/s
tz/s/s
tz/s/s/one
? 0
$ rm -r tz/s/s
? 0
$ tar tf tar-noend.tar
s/one
s/two
s/three
? 0
$ tar tvf tar-huge.tar
tar: tar-huge.tar: a member too big for this tar (over 4 GB), stopped
? 10
$ tar tf a.txt
tar: a.txt: is not a tar archive
? 10
$ tar tf empty.txt
tar: empty.txt: is empty
? 10
$ tar tf nofile
tar: nofile: object not found
? 10
$ tar xf tar-gnu.tar -C tz/s --strip-components 2
? 0
$ find tz/s
tz/s
tz/s/dddddddddddddddddddddddddddddddddddddddd
tz/s/dddddddddddddddddddddddddddddddddddddddd/ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
tz/s/hard
tz/s/lsoft
tz/s/up
? 0
$ tar xf tar-small.tar -C tz --strip-components=1 s/two
? 0
$ Type tz/two
222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
? 0
$ tar xOf tar-small.tar s/three
3
? 0
$ tar -x -O -f tar-small.tar s/three s/none
3
tar: s/none: not found in the archive
? 5
$ tar tf tar-gnu.tar g/s#?
g/soft
g/sub/
g/sub/dddddddddddddddddddddddddddddddddddddddd/
g/sub/dddddddddddddddddddddddddddddddddddddddd/ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
g/sub/hard
g/sub/lsoft
g/sub/up
? 0
$ tar tf tar-gnu.tar --exclude sub --exclude=b*
g/
g/empty/
g/fér
g/soft
g/text
g/zero
? 0
$ tar t --file=tar-small.tar --exclude=t#?
s/one
? 0
$ tar xf tar-small.tar -C tz/m
? 0
$ tar xkf tar-small.tar -C tz/m
tar: s/one: is already there, kept (-k)
tar: s/two: is already there, kept (-k)
tar: s/three: is already there, kept (-k)
? 5
$ tar xf tar-small.tar -C tz/m --skip-old-files
? 0
$ Protect tz/m/s/one d SUB
? 0
$ tar xf tar-small.tar -C tz/m
tar: s/one: is delete-protected, not replaced
? 10
$ Protect tz/m/s/one d ADD
? 0
$ Echo >tz/m/mark "mark"
? 0
$ SetDate tz/m/mark 01-Jan-21
? 0
$ tar xmf tar-small.tar -C tz/m s/two
? 0
$ find tz/m/s -newer tz/m/mark
tz/m/s
tz/m/s/two
? 0
$ tar -q
tar: -q: unknown option
? 10
$ tar
tar: needs one of -c, -x, -t (tar ? for usage)
? 10
$ tar cf tz/none.tar
tar: will not make an empty archive (no names given)
? 10
$ tar -c -x
tar: only one of -c, -x, -t
? 10
$ tar xf
tar: -f: needs a value
? 10
$ tar --nosuch
tar: --nosuch: unknown option
? 10
$ tar tf tar-small.tar --strip-components x
tar: x: not a number of parts
? 10
$ MakeDir tz/t tz/t/empty tz/t/sub
? 0
$ Echo >tz/t/plain "plain"
? 0
$ Echo >tz/t/spah "spah"
? 0
$ Protect tz/t/spah spah ADD
? 0
$ Echo >tz/t/nod "nod"
? 0
$ Protect tz/t/nod d SUB
? 0
$ Echo >tz/t/ro "ro"
? 0
$ Protect tz/t/ro wed SUB
? 0
$ FileNote tz/t/plain "a note, with spaces"
? 0
$ FileNote tz/t/sub "drawer note"
? 0
$ MakeLink tz/t/soft plain SOFT
? 0
$ MakeLink tz/t/sub/hard tz/t/plain
? 0
$ Echo >tz/t/sub/inner "inner"
? 0
$ MakeLink tz/t/sub/up /plain SOFT
? 0
$ MakeLink tz/t/aaa tz/t/sub/inner
? 0
$ SetDate tz/t/#? 01-Jan-99 12:34:56
? 0
$ SetDate tz/t/sub/inner 02-Feb-03 01:02:03
? 0
$ SetDate tz/t/sub 03-Mar-04 04:05:06
? 0
$ SetDate tz/t/empty 04-Apr-05 05:06:07
? 0
$ SetDate tz/t 05-May-06 06:07:08
? 0
$ tar cvf tz/t.tar -C tz t
t/
t/aaa
t/empty/
t/nod
t/plain
t/ro
t/soft
t/spah
t/sub/
t/sub/hard
t/sub/inner
t/sub/up
? 0
$ tar tvf tz/t.tar --exclude soft --exclude up
drwxr-xr-x 0/0               0 2006-05-05 06:07 t/
-rwxrwxrwx 0/0               6 2003-02-02 01:02 t/aaa
drwxr-xr-x 0/0               0 2005-04-04 05:06 t/empty/
-rwxrwxrwx 0/0               4 1999-01-01 12:34 t/nod
-rwxrwxrwx 0/0               6 1999-01-01 12:34 t/plain
-r--r--r-- 0/0               3 1999-01-01 12:34 t/ro
-rwxrwxrwx 0/0               5 1999-01-01 12:34 t/spah
drwxr-xr-x 0/0               0 2004-03-03 04:05 t/sub/
hrwxrwxrwx 0/0               0 1999-01-01 12:34 t/sub/hard link to t/plain
hrwxrwxrwx 0/0               0 2003-02-02 01:02 t/sub/inner link to t/aaa
? 0
$ tar tf tz/t.tar t/soft t/sub/up nope
t/soft
t/sub/up
tar: nope: not found in the archive
? 5
$ tar xf tz/t.tar -C tz/o
? 0
$ find tz/t -exec stat -c "%N|%F|%s|%A|%y|%C" {} "+"
tz/t|drawer|0|----rwed|2006-05-05 06:07:08|
tz/t/aaa|file|6|----rwed|2003-02-02 01:02:03|
tz/t/empty|drawer|0|----rwed|2005-04-04 05:06:07|
tz/t/nod|file|4|----rwe-|1999-01-01 12:34:56|
tz/t/plain|file|6|----rwed|1999-01-01 12:34:56|a note, with spaces
tz/t/ro|file|3|----r---|1999-01-01 12:34:56|
tz/t/soft -> plain|soft link|0|----rwed|1978-01-01 00:00:00|
tz/t/spah|file|5|hsparwed|1999-01-01 12:34:56|
tz/t/sub|drawer|0|----rwed|2004-03-03 04:05:06|drawer note
tz/t/sub/hard|file|6|----rwed|1999-01-01 12:34:56|a note, with spaces
tz/t/sub/inner|file|6|----rwed|2003-02-02 01:02:03|
tz/t/sub/up -> /plain|soft link|0|----rwed|1978-01-01 00:00:00|
? 0
$ find tz/o/t -exec stat -c "%N|%F|%s|%A|%y|%C" {} "+"
tz/o/t|drawer|0|----rwed|2006-05-05 06:07:08|
tz/o/t/aaa|file|6|----rwed|2003-02-02 01:02:03|
tz/o/t/empty|drawer|0|----rwed|2005-04-04 05:06:07|
tz/o/t/nod|file|4|----rwe-|1999-01-01 12:34:56|
tz/o/t/plain|file|6|----rwed|1999-01-01 12:34:56|a note, with spaces
tz/o/t/ro|file|3|----r---|1999-01-01 12:34:56|
tz/o/t/soft -> plain|soft link|0|----rwed|1978-01-01 00:00:00|
tz/o/t/spah|file|5|hsparwed|1999-01-01 12:34:56|
tz/o/t/sub|drawer|0|----rwed|2004-03-03 04:05:06|drawer note
tz/o/t/sub/hard|file|6|----rwed|1999-01-01 12:34:56|a note, with spaces
tz/o/t/sub/inner|file|6|----rwed|2003-02-02 01:02:03|
tz/o/t/sub/up -> /plain|soft link|0|----rwed|1978-01-01 00:00:00|
? 0
$ Echo >>tz/o/t/plain "more"
? 0
$ Type tz/o/t/sub/hard
plain
more
? 0
$ tar xf tz/t.tar -C tz/o
tar: t/nod: is delete-protected, not replaced
tar: t/ro: is delete-protected, not replaced
? 10
$ Type tz/o/t/sub/hard
plain
? 0
$ tar cf - tz/t/sub >tz/p.tar
? 0
$ tar tf - <tz/p.tar
tz/t/sub/
tz/t/sub/hard
tz/t/sub/inner
tz/t/sub/up
? 0
$ tar cf tz/v.tar {T}/tz/t/sub
tar: removing leading `RAM:' from member names
? 0
$ tar tf tz/v.tar --strip-components 2
t/sub/
t/sub/hard
t/sub/inner
t/sub/up
? 0
$ tar cvf tz/w.tar tz/t/sub//plain
tz/t/plain
? 0
$ tar cvf tz/w.tar -C tz/t/sub /plain
tar: removing leading `/' from member names
plain
? 0
$ tar cf tz/x.tar -C tz t --exclude sub --exclude n#?
? 0
$ tar tf tz/x.tar
t/
t/aaa
t/empty/
t/plain
t/ro
t/soft
t/spah
? 0
$ tar cf tz/n.tar --no-recursion -C tz t t/sub
? 0
$ tar tf tz/n.tar
t/
t/sub/
? 0
$ Echo >tz/list "t/plain"
? 0
$ Echo >>tz/list "t/empty"
? 0
$ tar cf tz/l.tar -C tz -T tz/list
? 0
$ tar tf tz/l.tar
t/plain
t/empty/
? 0
$ tar cf tz/t/self.tar -C tz t
tar: t/self.tar: is the archive itself, not stored
? 0
$ tar tf tz/t/self.tar t/s#?
t/soft
t/spah
t/sub/
t/sub/hard
t/sub/inner
t/sub/up
? 0
$ rm tz/t/self.tar
? 0
$ tar cvf tz/pat.tar tz/t/s*
tz/t/soft
tz/t/spah
tz/t/sub/
tz/t/sub/hard
tz/t/sub/inner
tz/t/sub/up
? 0
$ rm -rf tz
? 0
