$ mkdir ?
mkdir [-p] drawer ...
$ mkdir
mkdir: no drawer named (mkdir ? for usage)
? 10
$ mkdir -v k
mkdir: -v: unknown option
? 10
$ rm -rf k
$ mkdir k
? 0
$ mkdir k
mkdir: k: object already exists
? 10
$ mkdir k/a k/nope/c k/b
mkdir: k/nope/c: object not found
? 10
$ List k LFORMAT "%N %L"
b Dir
a Dir
$ mkdir -p k/x/y/z
? 0
$ List k/x/y LFORMAT "%N %L"
z Dir
$ mkdir -p k/x/y/z k/x/ k/a
? 0
$ Echo >k/f "x"
$ mkdir -p k/f/g
mkdir: k/f: exists and is not a drawer
? 10
$ mkdir -p k/f
mkdir: k/f: exists and is not a drawer
? 10
$ mkdir k/f/g
mkdir: k/f/g: object is not of required type
? 10
$ mkdir -p k//k/w
? 0
$ List k PAT w LFORMAT "%N %L"
w Dir
$ mkdir "k/#?"
? 0
$ List k LFORMAT "%N"
#?
w
f
x
b
a
$ mkdir -p RAM:
? 0
$ rm -r k
? 0
