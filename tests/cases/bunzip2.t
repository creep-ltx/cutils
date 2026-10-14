$ bunzip2 ?
bunzip2 [-cfkqrtv] [-S .suf] [file | pattern | - ...]
$ rm -rf czn
$ MakeDir czn
$ Copy czdata.bz2 czn/a.bz2
$ SetDate czn/a.bz2 03-Feb-21 10:11:12
$ bunzip2 czn/a.bz2
? 0
$ List czn LFORMAT "%N %L %D %T"
a 1191 03-Feb-21 10:11:12
$ diff czdata czn/a
? 0
$ Copy czdata.bz2 czn/t.tbz2
$ bunzip2 -k czn/t.tbz2
? 0
$ ls -1 czn
a
t.tar
t.tbz2
$ bunzip2 -c czdata.bz2 | wc -c
1191
$ type czdata.bz2 | bunzip2 | wc -l
25
$ bunzip2 -t czdata.bz2
? 0
$ bunzip2 -t czdata.zst
bunzip2: czdata.zst: not in bzip2 format
? 10
$ bunzip2 -c czwrong.gz
bunzip2: czwrong.gz: not in bzip2 format
? 10
$ Copy czdata czn/plain
$ bunzip2 czn/plain
bunzip2: czn/plain: unknown suffix -- ignored
? 5
$ rm -rf czn
