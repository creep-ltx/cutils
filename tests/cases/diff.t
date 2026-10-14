$ diff ?
diff [-uqsiwbBNra] [-U N] [--color=WHEN] FROM TO
$ SetDate diff1 02-Jan-20 13:14:15
$ SetDate diff2 03-Jan-20 08:00:00
$ diff diff1 diff2
2a3
> #include <stdlib.h>
6c7
<     printf("hello\n");
---
>     printf("hello, world\n");
14d14
< /* end */
? 5
$ diff diff1 diff1
? 0
$ diff -u diff1 diff2
--- diff1	2020-01-02 13:14:15
+++ diff2	2020-01-03 08:00:00
@@ -1,9 +1,10 @@
 /* hello.c */
 #include <stdio.h>
+#include <stdlib.h>
 
 int main(void)
 {
-    printf("hello\n");
+    printf("hello, world\n");
     return 0;
 }
 
@@ -11,4 +12,3 @@
 static int y;
 static int z;
 
-/* end */
? 5
$ diff -U0 diff1 diff2
--- diff1	2020-01-02 13:14:15
+++ diff2	2020-01-03 08:00:00
@@ -2,0 +3 @@
+#include <stdlib.h>
@@ -6 +7 @@
-    printf("hello\n");
+    printf("hello, world\n");
@@ -14 +14,0 @@
-/* end */
$ diff -U 1 diff1 diff2
--- diff1	2020-01-02 13:14:15
+++ diff2	2020-01-03 08:00:00
@@ -2,2 +2,3 @@
 #include <stdio.h>
+#include <stdlib.h>
 
@@ -5,3 +6,3 @@
 {
-    printf("hello\n");
+    printf("hello, world\n");
     return 0;
@@ -13,2 +14 @@
 
-/* end */
$ diff --color=always -U0 diff1 diff2
[1m--- diff1	2020-01-02 13:14:15[0m
[1m+++ diff2	2020-01-03 08:00:00[0m
[36m@@ -2,0 +3 @@[0m
[32m+#include <stdlib.h>[0m
[36m@@ -6 +7 @@[0m
[31m-    printf("hello\n");[0m
[32m+    printf("hello, world\n");[0m
[36m@@ -14 +14,0 @@[0m
[31m-/* end */[0m
$ diff --color=never -q diff1 diff2
Files diff1 and diff2 differ
? 5
$ diff -s diff1 diff1
Files diff1 and diff1 are identical
? 0
$ diff diffws1 diffws2
1,5c1,4
< one two
< Three
<   four
< five  
< 
---
> one  two
> three
> four
> five
? 5
$ diff -w diffws1 diffws2
2c2
< Three
---
> three
5d4
< 
$ diff -b diffws1 diffws2
2,3c2,3
< Three
<   four
---
> three
> four
5d4
< 
$ diff -i -b diffws1 diffws2
3c3
<   four
---
> four
5d4
< 
$ diff -i diffws1 diffws2
1c1
< one two
---
> one  two
3,5c3,4
<   four
< five  
< 
---
> four
> five
$ diff -iwB diffws1 diffws2
? 0
$ diff dbin diff1
Binary files dbin and diff1 differ
? 5
$ Echo >dn1 "a*nb" NOLINE
$ Echo >dn2 "a*nc" NOLINE
$ diff dn1 dn2
2c2
< b
\ No newline at end of file
---
> c
\ No newline at end of file
$ Echo >dn2 "a*nb"
$ diff dn1 dn2
2c2
< b
\ No newline at end of file
---
> b
? 5
$ type diff2 | diff -q diff1 -
Files diff1 and - differ
? 5
$ seq 10000 >ds1
$ seq 2 10001 >ds2
$ diff ds1 ds2
1d0
< 1
10000a10000
> 10001
? 5
$ seq 0 2 20000 >ds1
$ seq 0 3 30000 >ds2
$ diff ds1 ds2 | grep -c "^[<>]"
13334
$ Delete >NIL: dn1 dn2 ds1 ds2
$ rm -rf dr
$ MakeDir dr dr/a dr/b dr/a/sub dr/b/sub dr/a/onlya dr/a/same dr/b/same
$ Echo >dr/a/f1 "one*ntwo"
$ Echo >dr/b/f1 "one*nTWO"
$ Echo >dr/a/f2 "same"
$ Echo >dr/b/f2 "same"
$ Echo >dr/a/gone "x"
$ Echo >dr/b/new "y"
$ Echo >dr/a/sub/d "deep"
$ Echo >dr/b/sub/d "deeper"
$ Echo >dr/a/kind "file"
$ MakeDir dr/b/kind
$ diff -r dr/a dr/b
diff -r dr/a/f1 dr/b/f1
2c2
< two
---
> TWO
Only in dr/a: gone
File dr/a/kind is a file while file dr/b/kind is a drawer
Only in dr/b: new
Only in dr/a: onlya
diff -r dr/a/sub/d dr/b/sub/d
1c1
< deep
---
> deeper
? 5
$ diff -rq dr/a dr/b
Files dr/a/f1 and dr/b/f1 differ
Only in dr/a: gone
File dr/a/kind is a file while file dr/b/kind is a drawer
Only in dr/b: new
Only in dr/a: onlya
Files dr/a/sub/d and dr/b/sub/d differ
? 5
$ diff dr/a dr/b
diff dr/a/f1 dr/b/f1
2c2
< two
---
> TWO
Only in dr/a: gone
File dr/a/kind is a file while file dr/b/kind is a drawer
Only in dr/b: new
Only in dr/a: onlya
Common subdirectories: dr/a/same and dr/b/same
Common subdirectories: dr/a/sub and dr/b/sub
? 5
$ diff -rN dr/a dr/b
diff -rN dr/a/f1 dr/b/f1
2c2
< two
---
> TWO
diff -rN dr/a/gone dr/b/gone
1d0
< x
File dr/a/kind is a file while file dr/b/kind is a drawer
diff -rN dr/a/new dr/b/new
0a1
> y
Only in dr/a: onlya
diff -rN dr/a/sub/d dr/b/sub/d
1c1
< deep
---
> deeper
? 5
$ diff -r dr/a/sub dr/a/sub
? 0
$ diff diff1 dr
diff: dr/diff1: object not found
? 10
$ Copy diff1 dr/diff1
$ diff diff1 dr
? 0
$ diff -s dr diff1
Files dr/diff1 and diff1 are identical
$ rm -r dr
$ diff a.txt
diff: two files to compare (diff ? for usage)
? 10
$ diff a.txt b.txt c.txt
diff: only two files at a time
? 10
$ diff nofile a.txt
diff: nofile: object not found
? 10
$ diff -U x a.txt b.txt
diff: x: not a count
? 10
$ diff -y a.txt b.txt
diff: -y: unknown option
? 10
