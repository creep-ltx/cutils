$ gunzip ?
gunzip [-cfklqrtv] [-S .suf] [file | pattern | - ...]
$ gunzip -z czdata
gunzip: -z: this command only unpacks
? 10
$ rm -rf czu
$ MakeDir czu
$ Copy czdata.gz czu/a.gz
$ SetDate czu/a.gz 03-Feb-21 10:11:12
$ FileNote czu/a.gz "packed"
$ gunzip czu/a.gz
? 0
$ List czu LFORMAT "%N %L %D %T %C"
a 1191 03-Feb-21 10:11:12 packed
$ diff czdata czu/a
? 0
$ Copy czdata.gz czu/a.gz
$ gunzip czu/a.gz
gunzip: czu/a: already exists; not overwritten
? 5
$ gunzip -f czu/a.gz
? 0
$ ls -1 czu
a
$ Copy czmulti.gz czu/m.gz
$ gunzip -k czu/m.gz
? 0
$ ls -1 czu
a
m
m.gz
$ diff czdata czu/m
? 0
$ gunzip -c czdata.gz | wc -c
1191
$ type czdata.gz | gunzip | wc -l
25
$ gunzip -c czwrong.gz
gunzip: czwrong.gz: not in gzip format
? 10
$ gunzip -cf czwrong.gz | wc -c
1191
$ gunzip -t czdata.gz czmulti.gz
? 0
$ gunzip -tv czdata.gz
czdata.gz:	 OK
$ gunzip -t czcrc.gz
gunzip: czcrc.gz: damaged: CRC wrong
? 10
$ gunzip -t czdata.xz
gunzip: czdata.xz: not in gzip format
? 10
$ Copy czbad.gz czu/bad.gz
$ gunzip czu/bad.gz
gunzip: czu/bad.gz: damaged: bad deflate data
? 10
$ ls -1 czu
a
bad.gz
m
m.gz
$ Copy czdata czu/plain
$ gunzip czu/plain
gunzip: czu/plain: unknown suffix -- ignored
? 5
$ gunzip -l czdata.gz
         compressed        uncompressed  ratio uncompressed_name
                137                1191  88.4% czdata
$ Copy cztar.tgz czu/t.tgz
$ gunzip -v czu/t.tgz
czu/t.tgz:	-117.6% -- replaced with czu/t.tar
$ gunzip czu/nothere.gz
gunzip: czu/nothere.gz: object not found
? 5
$ rm -rf czu
