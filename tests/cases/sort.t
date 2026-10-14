$ sort so-words.txt
  date 20
Apple 10
Banana 7
apple 1
banana 3
banana 3
cherry 2
$ sort -f so-words.txt
  date 20
apple 1
Apple 10
banana 3
banana 3
Banana 7
cherry 2
$ sort -r so-words.txt
cherry 2
banana 3
banana 3
apple 1
Banana 7
Apple 10
  date 20
$ sort -u so-words.txt
  date 20
Apple 10
Banana 7
apple 1
banana 3
cherry 2
$ sort -k2n so-words.txt
apple 1
cherry 2
banana 3
banana 3
Banana 7
Apple 10
  date 20
$ sort -k2,2nr so-words.txt
  date 20
Apple 10
Banana 7
banana 3
banana 3
cherry 2
apple 1
$ sort -bk1,1 so-words.txt
Apple 10
Banana 7
apple 1
banana 3
banana 3
cherry 2
  date 20
$ sort -s -f -k1,1 so-words.txt
  date 20
Apple 10
apple 1
banana 3
banana 3
Banana 7
cherry 2
$ sort -fu so-words.txt
  date 20
apple 1
Apple 10
banana 3
Banana 7
cherry 2
$ sort -k1,1 -u so-words.txt
  date 20
Apple 10
Banana 7
apple 1
banana 3
cherry 2
$ sort -c so-words.txt
sort: so-words.txt:2: disorder: Apple 10
? 5
$ sort -n so-nums.txt
-3
-0.5
0
abc
1K
1M
2.25
2.5
10
512
$ sort -h so-nums.txt
-3
-0.5
0
abc
2.25
2.5
10
512
1K
1M
$ sort -rn so-nums.txt
512
10
2.5
2.25
1M
1K
abc
0
-0.5
-3
$ sort -V so-ver.txt
file1.txt
file1.9
file1.10
file9.txt
file10.txt
$ sort -t: -k3n so-pw.txt
root:x:0:0
bin:x:1:1
daemon:x:2:2
creep:x:1000:100
nobody:x:65534:65534
$ sort -t: -k3,3 so-pw.txt
root:x:0:0
bin:x:1:1
creep:x:1000:100
daemon:x:2:2
nobody:x:65534:65534
$ sort -t : -k1.2,1.3 so-pw.txt
daemon:x:2:2
bin:x:1:1
nobody:x:65534:65534
root:x:0:0
creep:x:1000:100
$ sort -t: -k4,4n -k1,1r so-pw.txt
root:x:0:0
bin:x:1:1
daemon:x:2:2
creep:x:1000:100
nobody:x:65534:65534
$ sort -k2 so-words.txt so-pw.txt
bin:x:1:1
creep:x:1000:100
daemon:x:2:2
nobody:x:65534:65534
root:x:0:0
apple 1
Apple 10
cherry 2
  date 20
banana 3
banana 3
Banana 7
$ sort -d so-amiga.txt

  zed
007
10
100
12 apples
3abc
+4
-5
9
Apple
Banana
ab
ab	z
alpha
apple
banana
cherry
_x
x
zebra
Ärlig
ärlig
$ type so-words.txt | sort -k2,2n
apple 1
cherry 2
banana 3
banana 3
Banana 7
Apple 10
  date 20
$ sort so-w*.txt
  date 20
Apple 10
Banana 7
apple 1
banana 3
banana 3
cherry 2
$ sort -o so-o.txt so-words.txt
$ type so-o.txt
  date 20
Apple 10
Banana 7
apple 1
banana 3
banana 3
cherry 2
$ sort -r -o so-o.txt so-o.txt
$ type so-o.txt
cherry 2
banana 3
banana 3
apple 1
Banana 7
Apple 10
  date 20
$ sort -C so-words.txt
? 5
$ sort so-words.txt | sort -c
? 0
$ sort -k 0 so-words.txt
sort: 0: not a key (F[.C][flags][,F[.C][flags]])
? 10
$ sort -t ab so-words.txt
sort: ab: the separator must be one character
? 10
$ sort nothere.txt
sort: nothere.txt: object not found
? 5
$ sort -n -k2 -t "\t" so-amiga.txt

  zed
+4
-5
007
10
100
12 apples
3abc
9
Apple
Banana
_x
ab
ab	z
alpha
apple
banana
cherry
x
zebra
Ärlig
ärlig
$ C:Sort FROM so-amiga.txt

  zed
+4
-5
007
10
100
12 apples
3abc
9
ab	z
ab
alpha
Apple
apple
banana
Banana
cherry
x
zebra
_x
ärlig
Ärlig
$ sort FROM so-amiga.txt

  zed
+4
-5
007
10
100
12 apples
3abc
9
ab	z
ab
alpha
Apple
apple
banana
Banana
cherry
x
zebra
_x
ärlig
Ärlig
$ C:Sort so-amiga.txt CASE

  zed
+4
-5
007
10
100
12 apples
3abc
9
Apple
Banana
_x
ab	z
ab
alpha
apple
banana
cherry
x
zebra
Ärlig
ärlig
$ sort so-amiga.txt CASE

  zed
+4
-5
007
10
100
12 apples
3abc
9
Apple
Banana
_x
ab	z
ab
alpha
apple
banana
cherry
x
zebra
Ärlig
ärlig
$ C:Sort so-amiga.txt NUMERIC
banana
Apple
apple
cherry
Banana
  zed
ab	z
ab

ärlig
Ärlig
alpha
zebra
_x
+4
x
-5
3abc
007
9
10
12 apples
100
$ sort so-amiga.txt NUMERIC
banana
Apple
apple
cherry
Banana
  zed
ab	z
ab

ärlig
Ärlig
alpha
zebra
_x
+4
x
-5
3abc
007
9
10
12 apples
100
$ C:Sort so-amiga.txt LEXICOGRAPHIC

  zed
+4
-5
007
10
100
12 apples
3abc
9
ab	z
ab
alpha
Apple
apple
Ärlig
ärlig
Banana
banana
cherry
x
zebra
_x
$ sort so-amiga.txt LEXICOGRAPHIC

  zed
+4
-5
007
10
100
12 apples
3abc
9
ab	z
ab
alpha
Apple
apple
Ärlig
ärlig
Banana
banana
cherry
x
zebra
_x
$ C:Sort so-amiga.txt DESCENDING
Ärlig
ärlig
_x
zebra
x
cherry
Banana
banana
apple
Apple
alpha
ab
ab	z
9
3abc
12 apples
100
10
007
-5
+4
  zed
$ sort so-amiga.txt DESCENDING
Ärlig
ärlig
_x
zebra
x
cherry
Banana
banana
apple
Apple
alpha
ab
ab	z
9
3abc
12 apples
100
10
007
-5
+4
  zed
$ C:Sort so-amiga.txt SKIPBLANKS

+4
-5
007
10
100
12 apples
3abc
9
ab	z
ab
alpha
Apple
apple
banana
Banana
cherry
x
zebra
  zed
_x
ärlig
Ärlig
$ sort so-amiga.txt SKIPBLANKS

+4
-5
007
10
100
12 apples
3abc
9
ab	z
ab
alpha
Apple
apple
banana
Banana
cherry
x
zebra
  zed
_x
ärlig
Ärlig
$ C:Sort so-amiga.txt COLSTART 2

9
x
  zed
10
100
007
12 apples
+4
-5
3abc
banana
Banana
ab	z
ab
zebra
cherry
alpha
Apple
apple
ärlig
Ärlig
_x
$ sort so-amiga.txt COLSTART 2

9
x
  zed
10
100
007
12 apples
+4
-5
3abc
banana
Banana
ab	z
ab
zebra
cherry
alpha
Apple
apple
ärlig
Ärlig
_x
$ C:Sort so-amiga.txt COLSTART=3 SKIPBLANKS NUMERIC
banana
Apple
apple
cherry
Banana
  zed
10
9
-5
ab	z
ab

ärlig
Ärlig
alpha
zebra
_x
+4
12 apples
3abc
x
100
007
$ sort so-amiga.txt COLSTART=3 SKIPBLANKS NUMERIC
banana
Apple
apple
cherry
Banana
  zed
10
9
-5
ab	z
ab

ärlig
Ärlig
alpha
zebra
_x
+4
12 apples
3abc
x
100
007
$ C:Sort from=so-amiga.txt colstart 2 descending case
_x
Ärlig
ärlig
apple
Apple
alpha
cherry
zebra
ab
ab	z
Banana
banana
3abc
-5
+4
12 apples
007
100
10
  zed
x
9
$ sort from=so-amiga.txt colstart 2 descending case
_x
Ärlig
ärlig
apple
Apple
alpha
cherry
zebra
ab
ab	z
Banana
banana
3abc
-5
+4
12 apples
007
100
10
  zed
x
9
$ C:Sort so-amiga.txt NUMERIC DESCENDING BUFSIZE=20
100
12 apples
10
9
007
3abc
-5
x
+4
_x
zebra
alpha
Ärlig
ärlig

ab
ab	z
  zed
Banana
cherry
apple
Apple
banana
$ sort so-amiga.txt NUMERIC DESCENDING BUFSIZE=20
100
12 apples
10
9
007
3abc
-5
x
+4
_x
zebra
alpha
Ärlig
ärlig

ab
ab	z
  zed
Banana
cherry
apple
Apple
banana
$ C:Sort so-amiga.txt sa-real.txt NUMERIC
$ sort so-amiga.txt sa-ours.txt NUMERIC
$ type sa-real.txt
banana
Apple
apple
cherry
Banana
  zed
ab	z
ab

ärlig
Ärlig
alpha
zebra
_x
+4
x
-5
3abc
007
9
10
12 apples
100
$ type sa-ours.txt
banana
Apple
apple
cherry
Banana
  zed
ab	z
ab

ärlig
Ärlig
alpha
zebra
_x
+4
x
-5
3abc
007
9
10
12 apples
100
$ sort so-words.txt sa-two.txt
$ type sa-two.txt
  date 20
apple 1
Apple 10
banana 3
banana 3
Banana 7
cherry 2
$ sort CASE
sort: required argument missing
? 10
$ sort FROM so-amiga.txt a b
sort: wrong number of arguments
? 10
$ sort nothere.txt sa-x.txt
sort: nothere.txt: object not found
? 10
$ sort so-amiga.txt COLSTART
sort: COLSTART: needs a value
? 10
