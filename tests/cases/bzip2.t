$ bzip2 ?
bzip2 [-cdfkqrtv1..9] [-S .suf] [file | pattern | - ...]
$ rm -rf czb
$ MakeDir czb
$ Copy czdata czb/a
$ SetDate czb/a 02-Jan-20 13:14:15
$ Protect czb/a +s
$ FileNote czb/a "note one"
$ bzip2 czb/a
? 0
$ List czb LFORMAT "%N %A %D %T %C"
a.bz2 -s--rwed 02-Jan-20 13:14:15 note one
$ bzip2 -t czb/a.bz2
? 0
$ bzip2 -d czb/a.bz2
? 0
$ List czb LFORMAT "%N %L %A %D %T %C"
a 1191 -s--rwed 02-Jan-20 13:14:15 note one
$ diff czdata czb/a
? 0
$ bzip2 -k1 czb/a
? 0
$ ls -1 czb
a
a.bz2
$ bzip2 czb/a
bzip2: czb/a.bz2: already exists; not overwritten
? 5
$ bzip2 czb/a.bz2
bzip2: czb/a.bz2: already has .bz2 suffix -- unchanged
? 5
$ bzip2 -dc czb/a.bz2 | wc -c
1191
$ bzip2 -c czdata | bzip2 -d | wc -l
25
$ bzip2 -9 -c czdata >czb/b.bz2
$ bzip2 -tv czb/b.bz2
czb/b.bz2:	 OK
$ bzip2 -dc czdata.bz2 | wc -c
1191
$ bzip2 -t czdata.gz
bzip2: czdata.gz: not in bzip2 format
? 10
$ bzip2 -d czb/nothere
bzip2: czb/nothere: object not found
? 5
$ bzip2 -fv czb/a
czb/a:	 86.3% -- replaced with czb/a.bz2
$ rm -rf czb
