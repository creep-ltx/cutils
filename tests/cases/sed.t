$ sed ?
sed [-nEsuz] [-i[SUFFIX]] [-l N] [-e SCRIPT]... [-f FILE]... [SCRIPT] [file ...]
$ sed --version
sed 0.1b2 (16.10.2026)
$ sed s/a/X/ sed-abc
X
b
c
? 0
$ sed -n 2p sed-abc
b
$ sed 2d sed-abc
a
c
$ sed "s/[abc]/<&>/g" sed-abc
<a>
<b>
<c>
$ sed -n "$p" sed-abc
c
$ sed "1!G;h;$!d" sed-abc
c
b
a
$ sed -e 1d -e "s/c/C/" sed-abc
b
C
$ sed -n "/a/p;/c/p" sed-abc
a
c
$ sed -f sed-script sed-abc
A
B
after b
before last
c
$ sed -s -n "$p" sed-abc sed-xyz
c
z
$ sed -n "$=" sed-abc sed-xyz
6
$ sed -n "$=" n100.txt
100
$ sed "y/abc/xyz/" sed-abc
x
y
z
$ sed = sed-nonl
1
n1
2
n2
$ sed "2a added" sed-abc
a
b
added
c
$ sed "1i top" sed-abc
top
a
b
c
$ sed "2c changed" sed-abc
a
changed
c
$ sed -n "/b/,$p" sed-abc
b
c
$ sed 2,3d sed-abc
a
$ sed -n 0~2p sed-words
jumps over
$ sed -n 2,+1p sed-words
jumps over
the lazy dog
$ sed -E "s/(\w+) (\w+)/\2 \1/" sed-words
quick the brown fox
over jumps
lazy the dog
$ sed "s/\w\+/\u&/g" sed-words
The Quick Brown Fox
Jumps Over
The Lazy Dog
$ sed "s/\bthe\b/THE/g;s/o\>/0/g" sed-words
THE quick brown fox
jumps over
THE lazy dog
$ sed -n "s/.**/\U&/;l" sed-latin
CAF\311 \305LAND \334BER$
$ sed "H;$!d;x;s/\n/,/g;s/^,//" sed-abc
a,b,c
$ sed -n "/\[other\]/,$p" sed-ini
[other]
key=val
$ sed "/^;/d;s/ = /=/" sed-ini
[main]
name=cu
[other]
key=val
$ sed -n 1F sed-abc
sed-abc
$ sed "1r sed-xyz" sed-abc
a
x
y
z
b
c
$ sed = sed-abc | sed "N;s/\n/ /"
1 a
2 b
3 c
$ sed s/x/X/ <sed-xyz
X
y
z
$ type sed-abc | sed -n 2p
b
$ sed -n "1s/.*/[&]/p" sed-words
[t]he quick brown fox
$ sed -n "1s/.**/[&]/p" sed-words
[the quick brown fox]
$ sed "s/b/**/" sed-abc
a
*
c
$ sed "s/b/*"q*"/" sed-abc
a
"q"
c
$ sed 2q sed-abc
a
b
? 0
$ sed 2q5 sed-abc
a
b
? 5
$ sed 1Q sed-abc
? 0
$ sed "$!N;s/\n/+/" sed-abc
a+b
c
$ sed -n "$!{N;P;D};p" sed-nonl
n1
n2
$ Copy sed-abc sedi1
$ Protect sedi1 +s
$ FileNote sedi1 "kept"
$ sed -i s/b/B/ sedi1
? 0
$ type sedi1
a
B
c
$ List sedi1 LFORMAT "%N %A %C"
sedi1 -s--rwed kept
$ sed -i.bak 2d sedi1
$ type sedi1
a
c
$ type sedi1.bak
a
B
c
$ Copy sed-xyz sedi2
$ sed -i -n 1p sedi1 sedi2
$ type sedi1 sedi2
a
x
$ sed -i "2q" sedi1.bak
$ type sedi1.bak
a
B
$ sed -ibak_* "s/a/A/" sedi1
$ type bak_sedi1 sedi1
a
A
$ ls -1 #?sedi#?
bak_sedi1
sedi1
sedi1.bak
sedi2
$ Delete #?sedi#? QUIET
$ sed -n "/[ac]/w RAM:sedw" sed-abc
$ type RAM:sedw
a
c
$ Delete RAM:sedw QUIET
$ sed "s/a/b" sed-abc
sed: -e expression #1, char 5: unterminated `s' command
? 10
$ sed k sed-abc
sed: -e expression #1, char 1: unknown command: `k'
? 10
$ sed "s/\(a\)\1/x/" sed-abc
sed: -e expression #1, char 12: back-references in a pattern are not supported
? 10
$ sed -n p nosuch sed-xyz
sed: nosuch: object not found
x
y
z
? 10
$ sed -i p
sed: no input files
? 10
$ sed -x p sed-abc
sed: -x: unknown option
? 10
