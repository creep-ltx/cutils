$ ln ?
ln [-sfv] TARGET [LINK]  |  ln [-sfv] TARGET ... DRAWER
$ rm -rf lz
$ MakeDir lz lz/d
$ Echo >lz/f "data"
$ ln lz/f lz/h
? 0
$ Type lz/h
data
$ Echo >>lz/f "more"
$ Type lz/h
data
more
$ ln lz/f lz/h
ln: lz/h: is already there (-f replaces it)
? 10
$ ln -f lz/f lz/h
ln: lz/h: is the same file as the target
? 10
$ ln -f lz/f lz
ln: lz/f: is the same file as the target
? 10
$ Type lz/f
data
more
$ ln -s f lz/rel
? 0
$ Type lz/rel
data
more
$ stat -c %N lz/rel
lz/rel -> f
$ ln -sv lz/f lz/d
lz/d/f -> lz/f
$ stat -c %N lz/d/f
lz/d/f -> lz/f
$ ln -s nofile lz/dangling
? 0
$ stat -c "%F %N" lz/dangling
soft link lz/dangling -> nofile
$ ln -s S:Startup-Sequence lz/ss
$ stat -c %N lz/ss
lz/ss -> S:Startup-Sequence
$ diff lz/ss S:Startup-Sequence
? 0
$ ln S:Startup-Sequence lz/x
ln: lz/x: a hard link cannot go to another volume (ln -s makes a soft one)
? 10
$ ln RAM: lz/r
ln: RAM:: a volume or assign root cannot be hard linked
? 10
$ ln lz/d lz/dh
? 0
$ ln -v lz/f lz/h lz/d
ln: lz/d/f: is already there (-f replaces it)
lz/d/h -> lz/h
? 10
$ ln -f lz/f lz/d
? 0
$ Type lz/d/f
data
more
$ ln nofile lz/n
ln: nofile: object not found
? 10
$ ln a.txt b.txt nodir
ln: nodir: is not a drawer
? 10
$ ln
ln: no target (ln ? for usage)
? 10
$ ln -x a b
ln: -x: unknown option
? 10
$ rm -r lz
? 0
