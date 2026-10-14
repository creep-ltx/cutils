$ Delete >NIL: lst lsb lsc ALL QUIET
$ MakeDir lst lst/dir lst/dir/sub lsb lsc
$ Echo >lst/file10.txt a
$ Echo >lst/file9.txt bb
$ Echo >lst/Zebra.lha ccc
$ Echo >lst/pic.info x
$ Echo >lst/tool y
$ Echo >lst/dir/inner.c z
$ Echo >lst/dir/sub/deep.txt z
$ ls lst
dir
file9.txt
file10.txt
tool
Zebra.lha
? 0
$ ls -F lst
dir/
file9.txt
file10.txt
tool*
Zebra.lha
$ ls lst -F
dir/
file9.txt
file10.txt
tool*
Zebra.lha
$ ls -a lst
dir
file9.txt
file10.txt
pic.info
tool
Zebra.lha
$ ls -A lst
dir
file9.txt
file10.txt
pic.info
tool
Zebra.lha
$ ls -S lst
Zebra.lha
file9.txt
file10.txt
tool
dir
$ ls -r lst
Zebra.lha
tool
file10.txt
file9.txt
dir
$ ls -1Sr lst
dir
tool
file10.txt
file9.txt
Zebra.lha
$ ls lst/#?.txt
lst/file9.txt
lst/file10.txt
$ ls lst/*.txt
lst/file9.txt
lst/file10.txt
$ ls lst/#?.info
lst/pic.info
$ ls lst/file9.txt
lst/file9.txt
$ ls -d lst/dir
lst/dir
$ ls -dF lst/dir lst/tool
lst/dir/
lst/tool*
$ ls nothere
ls: nothere: object not found
? 5
$ ls lst/#?.none
ls: lst/#?.none: nothing matches
? 5
$ ls lst/dir lst/file9.txt
lst/file9.txt

lst/dir:
inner.c
sub
$ ls lst/dir lst/dir/sub
lst/dir:
inner.c
sub

lst/dir/sub:
deep.txt
$ ls -R lst
lst:
dir
file9.txt
file10.txt
tool
Zebra.lha

lst/dir:
inner.c
sub

lst/dir/sub:
deep.txt
$ ls -R lst/dir/sub
lst/dir/sub:
deep.txt
$ ls -- lst/dir
inner.c
sub
$ Echo >lsb/one x
$ Echo >lsb/two xx
$ Echo >lsb/three xxx
$ SetDate lsb/one 01-Jan-20 12:34:00
$ SetDate lsb/two 01-Mar-22 08:00:00
$ SetDate lsb/three 15-Jun-21 23:59:00
$ ls -t lsb
two
three
one
$ ls -tr lsb
one
three
two
$ FileNote lsb/two "a note"
$ ls -l lsb
----rwed   2 01-Jan-20 12:34 one
----rwed   4 15-Jun-21 23:59 three
----rwed   3 01-Mar-22 08:00 two
: a note
$ Copy long.txt lsc/big
$ SetDate lsc/big 02-Feb-22 10:20:00
$ MakeDir lsc/d
$ SetDate lsc/d 02-Feb-22 10:21:00
$ ls -l lsc
----rwed 20007 02-Feb-22 10:20 big
----rwed   Dir 02-Feb-22 10:21 d
$ ls -lh lsc
----rwed   19K 02-Feb-22 10:20 big
----rwed   Dir 02-Feb-22 10:21 d
$ Echo >lsc/hid h
$ Protect lsc/hid +h
$ ls lsc
big
d
$ ls -a lsc
big
d
hid
$ ls -x
ls: -x: unknown option
? 10
$ ls ?
ls [-1aAdFhlrRSt] [path | pattern ...]
? 0
$ Delete >NIL: lst lsb lsc ALL QUIET
