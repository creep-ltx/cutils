$ rm ?
rm [-rfv] file | pattern ...
$ rm -x
rm: -x: unknown option
? 10
$ rm
rm: nothing to delete (rm ? for usage)
? 10
$ Execute mktree r
$ rm r/f1
? 0
$ List r LFORMAT "%N"
empty
sub
$ rm r/nope
rm: r/nope: object not found
? 5
$ rm -f r/nope
? 0
$ rm r/z#?
rm: r/z#?: nothing matches
? 5
$ rm -f r/z* r/nope
? 0
$ rm r/sub
rm: r/sub: is a drawer (use -r)
? 10
$ rm r/sub/*
rm: r/sub/deep: is a drawer (use -r)
? 10
$ List r/sub LFORMAT "%N"
deep
$ rm NIL:
rm: NIL:: refusing to remove a volume root
? 10
$ rm -rf NIL:/
rm: NIL:/: refusing to remove a volume root
? 10
$ Execute mktree r
$ rm -rv r/
r/f1
r/sub/f2
r/sub/deep/f3
r/sub/deep
r/empty
r/sub
r
? 0
$ rm r
rm: r: object not found
? 5
$ Execute mktree r
$ Protect r/sub/f2 -d
$ rm -rv r
r/f1
r/sub/deep/f3
r/sub/deep
r/empty
rm: r/sub: directory not empty
rm: r: directory not empty
not deleted:
  r/sub/f2
? 10
$ List r ALL LFORMAT "%P%N"
r/sub
r/sub/f2
$ rm r/sub/f2
not deleted:
  r/sub/f2
? 5
$ rm -rf r
? 0
$ rm r
rm: r: object not found
? 5
$ Execute mktree r
$ Execute mktree w
$ MakeLink w/ld /r/sub SOFT
$ MakeLink w/lf /r/f1 SOFT
$ rm -r w
? 0
$ MakeLink ld r/sub SOFT
$ rm ld
? 0
$ List r ALL LFORMAT "%P%N"
r/f1
r/empty
r/sub
r/sub/f2
r/sub/deep
r/sub/deep/f3
$ rm -r r w
rm: w: object not found
? 5
