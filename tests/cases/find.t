$ Delete >NIL: ft fd fl fs ALL QUIET
$ MakeDir ft ft/sub ft/sub/deep ft/void
$ Echo >ft/a.txt one
$ Echo >ft/B.c x
$ Copy empty.txt ft/empty
$ Echo >ft/sub/c.txt "hello world"
$ Copy long.txt ft/sub/deep/big.txt
$ Echo >ft/file9 nine
$ Echo >ft/file10 ten
$ SetDate ft/a.txt 01-Jan-20 12:00:00
$ find ft
ft
ft/a.txt
ft/B.c
ft/empty
ft/file9
ft/file10
ft/sub
ft/sub/c.txt
ft/sub/deep
ft/sub/deep/big.txt
ft/void
? 0
$ find ft -name *.txt
ft/a.txt
ft/sub/c.txt
ft/sub/deep/big.txt
$ find ft -name #?.TXT -print
ft/a.txt
ft/sub/c.txt
ft/sub/deep/big.txt
$ find ft -iname "#?.C"
ft/B.c
$ find ft -type d
ft
ft/sub
ft/sub/deep
ft/void
$ find ft -type f -size +100
ft/sub/deep/big.txt
$ find ft -type f -size -3
ft/B.c
ft/empty
$ find ft -size 12c
ft/sub/c.txt
$ find ft -size 20k
ft/sub/deep/big.txt
$ find ft -type f -size -1k
ft/empty
$ find ft -empty
ft/empty
ft/void
$ find ft -maxdepth 1
ft
ft/a.txt
ft/B.c
ft/empty
ft/file9
ft/file10
ft/sub
ft/void
$ find ft -maxdepth 0
ft
$ find ft -mindepth 2
ft/sub/c.txt
ft/sub/deep
ft/sub/deep/big.txt
$ find ft -name sub -prune -o -print
ft
ft/a.txt
ft/B.c
ft/empty
ft/file9
ft/file10
ft/void
$ find ft ! -type d
ft/a.txt
ft/B.c
ft/empty
ft/file9
ft/file10
ft/sub/c.txt
ft/sub/deep/big.txt
$ find ft -type f ( -name a#? -o -name b.#? )
ft/a.txt
ft/B.c
$ find ft -not -name #?.txt -type f
ft/B.c
ft/empty
ft/file9
ft/file10
$ find ft -name file#? -a -name #?0
ft/file10
$ find ft -name file9 -or -name file10
ft/file9
ft/file10
$ find ft -depth -mindepth 1 ! -type f
ft/sub/deep
ft/sub
ft/void
$ find ft -type f -newer ft/a.txt
ft/B.c
ft/empty
ft/file9
ft/file10
ft/sub/c.txt
ft/sub/deep/big.txt
$ find ft -type f ! -newer ft/a.txt
ft/a.txt
$ find ft -type f -mtime +30
ft/a.txt
$ find ft -name B.c -mtime 0
ft/B.c
$ find ft -name B.c -mtime -1 -mmin -60
ft/B.c
$ find ft -type f -mtime -1 -name a.txt
$ find ft/file#?
ft/file9
ft/file10
$ find ft/sub -exec echo found {} ";"
found ft/sub
found ft/sub/c.txt
found ft/sub/deep
found ft/sub/deep/big.txt
$ find ft/sub -name c.txt -exec echo {}
ft/sub/c.txt
$ find ft/sub -exec echo {} "+"
ft/sub ft/sub/c.txt ft/sub/deep ft/sub/deep/big.txt
$ find ft -type f -exec Search ">NIL:" {} hello ";" -print
ft/sub/c.txt
$ find ft/sub -type f -print0 | wc -c
33
$ Echo >ft-default.scr "CD ft/sub*Nfind"
$ Execute ft-default.scr
c.txt
deep
deep/big.txt
$ Echo >ft-default.scr "CD ft/sub*Nfind -name big#?"
$ Execute ft-default.scr
deep/big.txt
$ MakeDir fd fd/sub fd/sub2
$ Echo >fd/keep.txt k
$ Echo >fd/x.tmp x
$ Echo >fd/sub/y.tmp y
$ Echo >fd/sub2/z.tmp z
$ find fd -name #?.tmp -delete
? 0
$ find fd
fd
fd/keep.txt
fd/sub
fd/sub2
$ Echo >fd/sub/stay x
$ find fd -name sub#? -delete
find: fd/sub: directory not empty
? 5
$ find fd
fd
fd/keep.txt
fd/sub
fd/sub/stay
$ find RAM: -maxdepth 0 -delete
find: RAM:: a root, not deleted
? 5
$ MakeDir fl
$ MakeLink fl/l ft/sub SOFT
$ find fl
fl
fl/l
$ find fl -type l
fl/l
$ MakeDir fs
$ Echo >"fs/a b" x
$ find fs -type f -exec echo [{}] ";"
[fs/a b]
$ find "fs/a b"
fs/a b
$ find nothere
find: nothere: object not found
? 5
$ find ft -foo
find: -foo: unknown test
? 10
$ find ft -name
find: -name: needs a value
? 10
$ find ft ( -name x
find: (: no ) to end it
? 10
$ find ft -type x
find: -type: is f, d or l
? 10
$ find ft -size abc
find: abc: not a number
? 10
$ find ft -o -print
find: -o: not expected here
? 10
$ find ft -name x junk
find: junk: paths go before the tests
? 10
$ find ft !
find: !: something should follow
? 10
$ find ?
find [path | pattern ...] [-name PAT] [-type f|d|l] [-size [+-]N[ckM]] [-mtime [+-]N] [-newer FILE] [-empty] [-maxdepth N] [-mindepth N] [! -o -a ( )] [-print] [-prune] [-delete] [-exec CMD {} ;]
? 0
$ Delete >NIL: ft fd fl fs ft-default.scr ALL QUIET
