$ basename ?
basename NAME [SUFFIX] | -a [-s SUFFIX] NAME ...
$ basename Work:dir/file.c
file.c
? 0
$ basename Work:dir/file.c .c
file
$ basename file.c file.c
file.c
$ basename Work:dir/
dir
$ basename Work:
Work:
$ basename RAM:x
x
$ basename /
/
$ basename -a a/b c:d e
b
d
e
$ basename -s .txt a.txt dir/b.txt c
a
b
c
$ basename -as .c x.c y.h
x
y.h
$ basename
basename: no name (basename ? for usage)
? 10
$ basename a b c
basename: c: one name and a suffix at most (-a for more)
? 10
$ basename -x a
basename: -x: unknown option
? 10
