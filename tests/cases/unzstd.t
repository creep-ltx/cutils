$ unzstd ?
unzstd [-cfklqrtv] [--rm] [-S .suf] [file | pattern | - ...]
$ rm -rf czs
$ MakeDir czs
$ Copy czdata.zst czs/a.zst
$ SetDate czs/a.zst 03-Feb-21 10:11:12
$ unzstd czs/a.zst
? 0
$ List czs/a LFORMAT "%N %L %D %T"
a 1191 03-Feb-21 10:11:12
$ ls -1 czs
a
a.zst
$ diff czdata czs/a
? 0
$ unzstd czs/a.zst
unzstd: czs/a: already exists; not overwritten
? 5
$ unzstd -f --rm czs/a.zst
? 0
$ ls -1 czs
a
$ Copy czdata.zst czs/t.tzst
$ unzstd -v czs/t.tzst
czs/t.tzst:	 90.4% -- created czs/t.tar
$ unzstd -c czdata.zst | wc -c
1191
$ type czdata.zst | unzstd | wc -l
25
$ unzstd -t czdata.zst
? 0
$ unzstd -t czdata.bz2
unzstd: czdata.bz2: not in zstd format
? 10
$ rm -rf czs
