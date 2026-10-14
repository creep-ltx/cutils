$ gzip ?
gzip [-cdfklnqrtv1..9] [-S .suf] [file | pattern | - ...]
$ gzip -x czdata
gzip: -x: unknown option
? 10
$ rm -rf czg
$ MakeDir czg
$ Copy czdata czg/a
$ SetDate czg/a 02-Jan-20 13:14:15
$ Protect czg/a +s
$ FileNote czg/a "note one"
$ gzip czg/a
? 0
$ List czg LFORMAT "%N %A %D %T %C"
a.gz -s--rwed 02-Jan-20 13:14:15 note one
$ gzip -t czg/a.gz
? 0
$ gzip -d czg/a.gz
? 0
$ List czg LFORMAT "%N %L %A %D %T %C"
a 1191 -s--rwed 02-Jan-20 13:14:15 note one
$ diff czdata czg/a
? 0
$ gzip -k czg/a
? 0
$ ls -1 czg
a
a.gz
$ gzip czg/a
gzip: czg/a.gz: already exists; not overwritten
? 5
$ gzip -q czg/a
? 5
$ gzip czg/a.gz
gzip: czg/a.gz: already has .gz suffix -- unchanged
? 5
$ gzip -f czg/a
? 0
$ ls -1 czg
a.gz
$ gunzip -c czg/a.gz | wc -c
1191
$ gzip -c czdata | gzip -d | wc -l
25
$ gzip -c czdata >czg/b.gz
$ gzip -tv czg/b.gz
czg/b.gz:	 OK
$ gzip -dc czg/b.gz >czg/b
$ diff czdata czg/b
? 0
$ gzip -9vf czg/b
czg/b:	 88.4% -- replaced with czg/b.gz
$ gzip -1 -c czdata >czg/c.gz
$ gzip -dc czg/c.gz | wc -c
1191
$ gzip -l czdata.gz
         compressed        uncompressed  ratio uncompressed_name
                137                1191  88.4% czdata
$ gzip -l czdata.gz czmulti.gz
         compressed        uncompressed  ratio uncompressed_name
                137                1191  88.4% czdata
                210                 791  73.4% czmulti
                347                1982  82.4% (totals)
$ gzip -dc czmulti.gz | wc -c
1191
$ gzip -t czmulti.gz
? 0
$ gzip -t czbad.gz
gzip: czbad.gz: damaged: bad deflate data
? 10
$ gzip -t czcrc.gz
gzip: czcrc.gz: damaged: CRC wrong
? 10
$ gzip -t czjunk.gz
gzip: czjunk.gz: decompression OK, trailing garbage ignored
? 5
$ gzip -t czwrong.gz
gzip: czwrong.gz: not in gzip format
? 10
$ gzip -t czdata.bz2
gzip: czdata.bz2: not in gzip format
? 10
$ gzip -t nothere.gz
gzip: nothere.gz: object not found
? 5
$ gzip czg
gzip: czg: is a drawer -- ignored
? 5
$ rm -rf czg
$ MakeDir czg czg/sub
$ Copy czdata czg/x
$ Copy czdata czg/sub/y
$ gzip -r czg
? 0
$ find czg -type f
czg/sub/y.gz
czg/x.gz
$ gzip -dr czg
? 0
$ find czg -type f
czg/sub/y
czg/x
$ gzip czg/#?
gzip: czg/sub: is a drawer -- ignored
? 5
$ gzip -d czg/x
? 0
$ gzip -S .z czg/sub/y
? 0
$ ls -1 czg/sub
y.z
$ gzip -d -S .z czg/sub/y.z
? 0
$ Copy cztar.tgz czg/t.tgz
$ gzip -dv czg/t.tgz
czg/t.tgz:	-117.6% -- replaced with czg/t.tar
$ Type czg/t.tar
tarball stand-in
$ gzip -c czdata | gzip -dc - | wc -c
1191
$ rm -rf czg
