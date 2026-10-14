$ cp ?
cp [-fr] from ... to
$ cp b.txt
cp: needs FROM and TO (cp ? for usage)
? 10
$ cp -p a b
cp: -p: unknown option
? 10
$ rm -rf c d e
$ Execute mktree c
$ cp c/f1 c/g
? 0
$ List c/g LFORMAT "%N %L %A %D %T %C"
g 4 -s--rwed 02-Jan-20 13:14:15 note one
$ cp c/sub/f2 c/g
not copied:
  c/sub/f2
? 5
$ Type c/g
one
$ cp -f c/sub/f2 c/g
? 0
$ Type c/g
two
$ cp c/g c/g
cp: c/g: source and target are the same file
? 10
$ cp -f c/g c/g
cp: c/g: source and target are the same file
? 10
$ cp -f c/g c//c/g
cp: c/g: source and target are the same file
? 10
$ Type c/g
two
$ cp c/nope c/x
cp: c/nope: object not found
? 5
$ cp c/sub c/x
cp: c/sub: is a drawer (use -r)
? 10
$ MakeDir e e/f1
$ cp -f c/f1 e
cp: e/f1: is a drawer, not replaced by a file
? 10
$ cp -f c/f1 e/x
? 0
$ Type e/x
one
$ rm -r e
$ cp c/f1 c/sub/f2 c/nodir
cp: c/nodir: not a drawer (several sources need one)
? 10
$ cp c/f#? c/nodir
cp: c/nodir: not a drawer (several sources need one)
? 10
$ cp c/f1 c/sub/f2 c/sub/deep
? 0
$ List c/sub/deep LFORMAT "%N %L"
f2 4
f1 4
f3 6
$ cp -r c d
? 0
$ List d ALL LFORMAT "%P%N %L %A [%C]"
d/empty Dir ----rwed []
d/sub Dir ----rwed []
d/g 4 ----rwed []
d/f1 4 -s--rwed [note one]
d/sub/deep Dir ----rwed []
d/sub/f2 4 ----rwed []
d/sub/deep/f2 4 ----rwed []
d/sub/deep/f1 4 -s--rwed [note one]
d/sub/deep/f3 6 ----rwed []
$ List d/sub/deep/f1 LFORMAT "%N %D %T"
f1 02-Jan-20 13:14:15
$ cp -r c c/sub
cp: c: cannot copy a drawer into itself
? 10
$ cp -r c c
cp: c: cannot copy a drawer into itself
? 10
$ rm d/sub/f2
$ cp -r c/sub d
not copied:
  c/sub/deep/f3
  c/sub/deep/f1
  c/sub/deep/f2
? 5
$ List d/sub LFORMAT "%N"
f2
deep
$ cp -rf c/sub d
? 0
$ Protect c/sub +p
$ FileNote c/sub "subnote"
$ MakeLink c/lf /c/f1 SOFT
$ MakeLink c/ld /c/sub SOFT
$ cp -R c e
cp: c/ld: a link to a drawer, not followed
? 5
$ List e LFORMAT "%N %L %A [%C]"
empty Dir ----rwed []
sub Dir --p-rwed [subnote]
lf 4 -s--rwed [note one]
g 4 ----rwed []
f1 4 -s--rwed [note one]
$ cp c/ld e
cp: c/ld: a link to a drawer, not followed
? 5
$ rm -r c d e
