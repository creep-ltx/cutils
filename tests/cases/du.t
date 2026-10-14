$ Delete >NIL: dt ALL QUIET
$ MakeDir dt dt/sub dt/sub/deep dt/void
$ Echo >dt/a.txt one
$ Echo >dt/B.c x
$ Copy empty.txt dt/empty
$ Echo >dt/sub/c.txt "hello world"
$ Copy long.txt dt/sub/deep/big.txt
$ Echo >dt/file9 nine
$ Echo >dt/file10 ten
$ du dt
20	dt/sub/deep
21	dt/sub
0	dt/void
25	dt
? 0
$ du -s dt
25	dt
$ du -a dt/sub
1	dt/sub/c.txt
20	dt/sub/deep/big.txt
20	dt/sub/deep
21	dt/sub
$ du -d 1 dt
21	dt/sub
0	dt/void
25	dt
$ du -d0 dt
25	dt
$ du -a -d 1 dt
1	dt/a.txt
1	dt/B.c
0	dt/empty
1	dt/file9
1	dt/file10
21	dt/sub
0	dt/void
25	dt
$ du -c dt/sub dt/void
20	dt/sub/deep
21	dt/sub
0	dt/void
21	total
$ du -sc dt/file#?
1	dt/file9
1	dt/file10
2	total
$ du dt/sub/c.txt
1	dt/sub/c.txt
$ du -h dt
20K	dt/sub/deep
21K	dt/sub
0	dt/void
25K	dt
$ du -h dt/sub/c.txt dt/empty
1.0K	dt/sub/c.txt
0	dt/empty
$ Copy long.txt dt/void/b1
$ Copy long.txt dt/void/b2
$ Copy long.txt dt/void/b3
$ Copy long.txt dt/void/b4
$ Copy long.txt dt/void/b5
$ Copy long.txt dt/void/b6
$ Copy long.txt dt/void/b7
$ Copy long.txt dt/void/b8
$ Copy long.txt dt/void/b9
$ Copy long.txt dt/void/b10
$ Copy long.txt dt/void/b11
$ Copy long.txt dt/void/b12
$ Copy long.txt dt/void/b13
$ Copy long.txt dt/void/b14
$ Copy long.txt dt/void/b15
$ Copy long.txt dt/void/b16
$ Copy long.txt dt/void/b17
$ Copy long.txt dt/void/b18
$ Copy long.txt dt/void/b19
$ Copy long.txt dt/void/b20
$ Copy long.txt dt/void/b21
$ Copy long.txt dt/void/b22
$ Copy long.txt dt/void/b23
$ Copy long.txt dt/void/b24
$ Copy long.txt dt/void/b25
$ Copy long.txt dt/void/b26
$ Copy long.txt dt/void/b27
$ Copy long.txt dt/void/b28
$ Copy long.txt dt/void/b29
$ Copy long.txt dt/void/b30
$ Copy long.txt dt/void/b31
$ Copy long.txt dt/void/b32
$ Copy long.txt dt/void/b33
$ Copy long.txt dt/void/b34
$ Copy long.txt dt/void/b35
$ Copy long.txt dt/void/b36
$ Copy long.txt dt/void/b37
$ Copy long.txt dt/void/b38
$ Copy long.txt dt/void/b39
$ Copy long.txt dt/void/b40
$ Copy long.txt dt/void/b41
$ Copy long.txt dt/void/b42
$ Copy long.txt dt/void/b43
$ Copy long.txt dt/void/b44
$ Copy long.txt dt/void/b45
$ Copy long.txt dt/void/b46
$ Copy long.txt dt/void/b47
$ Copy long.txt dt/void/b48
$ Copy long.txt dt/void/b49
$ Copy long.txt dt/void/b50
$ Copy long.txt dt/void/b51
$ du -s dt/void
1020	dt/void
$ du -sh dt/void
1020K	dt/void
$ Copy long.txt dt/void/b52
$ du -sh dt/void dt
1.1M	dt/void
1.1M	dt
$ du -s dt/void dt
1040	dt/void
1065	dt
$ Echo >dt-default.scr "CD dt/sub*Ndu"
$ Execute dt-default.scr
20	deep
21	""
$ MakeDir dl
$ MakeLink dl/l dt/sub SOFT
$ du -a dl
0	dl/l
0	dl
$ du nothere
du: nothere: object not found
? 5
$ du -d x dt
du: x: not a depth
? 10
$ du -x
du: -x: unknown option
? 10
$ du ?
du [-achs] [-d N] [path | pattern ...]
? 0
$ Delete >NIL: dt dl dt-default.scr ALL QUIET
