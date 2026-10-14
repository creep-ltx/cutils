$ mv ?
mv [-fb] from ... to
$ mv b.txt
mv: needs FROM and TO (mv ? for usage)
? 10
$ mv -r a b
mv: -r: unknown option
? 10
$ rm -rf m mt ml SYS:T/cumv#?
$ Execute mktree m
$ mv m/f1 m/g
? 0
$ List m LFORMAT "%N"
g
empty
sub
$ List m/g LFORMAT "%N %L %A %D %T [%C]"
g 4 -s--rwed 02-Jan-20 13:14:15 [note one]
$ mv m/sub/f2 m/g
not moved:
  m/sub/f2
? 5
$ Type m/g
one
$ mv -f m/sub/f2 m/g
? 0
$ Type m/g
two
$ mv m/g m/g
mv: m/g: source and target are the same file
? 10
$ mv -f m/g m//m/g
mv: m/g: source and target are the same file
? 10
$ Type m/g
two
$ mv m/g m/G
? 0
$ List m/G LFORMAT "%N"
G
$ mv m/nope m/x
mv: m/nope: object not found
? 5
$ mv NIL: m
mv: NIL:: cannot move a volume root
? 10
$ mv m/G m/empty m/nodir
mv: m/nodir: not a drawer (several sources need one)
? 10
$ mv m/sub m/sub/deep
mv: m/sub: object is in use
? 10
$ Echo >m/h "new"
$ Echo >m/t "old"
$ mv -b m/h m/t
? 0
$ Type m/t
new
$ Type m/t.mvbak
old
$ Echo >m/h "newer"
$ mv -b m/h m/t
mv: m/h: not moved, m/t.mvbak already exists
not moved:
  m/h
? 10
$ mv -bf m/h m/t
? 0
$ Type m/t
newer
$ Type m/t.mvbak
new
$ MakeDir m/x m/x/deep
$ mv m/sub/deep m/x
mv: m/x/deep: is a drawer, not replaced
? 10
$ mv m/sub/d* m/G m/x
mv: m/x/deep: is a drawer, not replaced
? 10
$ List m/x LFORMAT "%N"
G
deep
$ SetDate m/t 02-Jan-20 13:14:15
$ FileNote m/t "tn"
$ mv m/t SYS:T/cumvt
? 0
$ List SYS:T/cumvt LFORMAT "%N %L %A %D %T [%C]"
cumvt 6 ----rwed 02-Jan-20 13:14:15 [tn]
$ mv SYS:T/cumvt m
? 0
$ List m/cumvt LFORMAT "%N %L %A %D %T [%C]"
cumvt 6 ----rwed 02-Jan-20 13:14:15 [tn]
$ rm SYS:T/cumvt
rm: SYS:T/cumvt: object not found
? 5
$ Execute mktree mt
$ mv mt SYS:T/cumvd
? 0
$ List SYS:T/cumvd ALL LFORMAT "%P%N %L %A [%C]"
SYS:T/cumvd/sub Dir ----rwed []
SYS:T/cumvd/empty Dir ----rwed []
SYS:T/cumvd/f1 4 -s--rwed [note one]
SYS:T/cumvd/sub/deep Dir ----rwed []
SYS:T/cumvd/sub/f2 4 ----rwed []
SYS:T/cumvd/sub/deep/f3 6 ----rwed []
$ List SYS:T/cumvd/f1 LFORMAT "%N %D %T"
f1 02-Jan-20 13:14:15
$ rm mt
rm: mt: object not found
? 5
$ mv SYS:T/cumvd mt
? 0
$ List mt ALL LFORMAT "%P%N %L %A [%C]"
mt/sub Dir ----rwed []
mt/empty Dir ----rwed []
mt/f1 4 -s--rwed [note one]
mt/sub/deep Dir ----rwed []
mt/sub/f2 4 ----rwed []
mt/sub/deep/f3 6 ----rwed []
$ Protect mt/sub/f2 -r
$ mv mt SYS:T/cumvd
mv: mt/sub/f2: file is read protected
mv: mt: move abandoned, source untouched
? 10
$ rm SYS:T/cumvd
rm: SYS:T/cumvd: object not found
? 5
$ List mt ALL LFORMAT "%P%N %A"
mt/sub ----rwed
mt/empty ----rwed
mt/f1 -s--rwed
mt/sub/deep ----rwed
mt/sub/f2 -----wed
mt/sub/deep/f3 ----rwed
$ Protect mt/sub/f2 +r
$ Execute mktree ml
$ MakeLink ml/sub/lk /f2 SOFT
$ mv ml SYS:T/cumvl
mv: ml/sub/lk: a link in the tree, not movable across volumes
mv: ml: move abandoned, source untouched
? 10
$ rm SYS:T/cumvl
rm: SYS:T/cumvl: object not found
? 5
$ MakeLink lk ml SOFT
$ mv lk SYS:T/cumvl
mv: lk: a link, not movable across volumes
? 10
$ mv lk ml/lk2
? 0
$ List ml LFORMAT "%N"
lk2
f1
empty
sub
$ Protect mt/sub/f2 -d
$ mv mt SYS:T/cumvd
mv: mt/sub/f2: object is protected from deletion
mv: mt/sub: directory not empty
mv: mt: directory not empty
mv: mt: moved, but the source could not be fully deleted
? 5
$ List SYS:T/cumvd/sub LFORMAT "%N %A"
deep ----rwed
f2 ----rwe-
$ rm -rf m mt ml SYS:T/cumvd
? 0
