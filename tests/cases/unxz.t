$ unxz ?
unxz [-cfklqrtv] [-S .suf] [file | pattern | - ...]
$ rm -rf czx
$ MakeDir czx
$ Copy czdata.xz czx/a.xz
$ SetDate czx/a.xz 03-Feb-21 10:11:12
$ Protect czx/a.xz +s
$ unxz czx/a.xz
? 0
$ List czx LFORMAT "%N %L %A %D %T"
a 1191 -s--rwed 03-Feb-21 10:11:12
$ diff czdata czx/a
? 0
$ Copy czdata.xz czx/t.txz
$ unxz -kv czx/t.txz
czx/t.txz:	 84.8% -- created czx/t.tar
$ ls -1 czx
a
t.tar
t.txz
$ unxz -c czdata.xz | wc -c
1191
$ type czdata.xz | unxz | wc -l
25
$ unxz -t czdata.xz
? 0
$ unxz -t czcut.xz
unxz: czcut.xz: truncated
? 10
$ unxz -t czdata.gz
unxz: czdata.gz: not in xz format
? 10
$ unxz -l czdata.xz
         compressed        uncompressed  ratio uncompressed_name
                180                1191  84.8% czdata
$ rm -rf czx
