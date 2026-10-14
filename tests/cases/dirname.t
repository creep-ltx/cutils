$ dirname ?
dirname NAME ...
$ dirname Work:a/b
Work:a
? 0
$ dirname Work:a
Work:
$ dirname Work:
Work:
$ dirname a/b/c
a/b
$ dirname a b/c

b
$ dirname Work:a/b/
Work:a
$ dirname /x
/
$ dirname a//b
a//
$ dirname
dirname: no name (dirname ? for usage)
? 10
