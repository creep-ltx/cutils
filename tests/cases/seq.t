$ seq ?
seq [-w] [-s SEP] [-f FORMAT] [FIRST [INC]] LAST
$ seq 3
1
2
3
? 0
$ seq 2 4
2
3
4
$ seq 10 -3 1
10
7
4
1
$ seq -w 8 10
08
09
10
$ seq -s , 5
1,2,3,4,5
$ seq -s ", " 3
1, 2, 3
$ seq -3 1
-3
-2
-1
0
1
$ seq 0 0.25 1
0.00
0.25
0.50
0.75
1.00
$ seq 1 0.5 2.4
1.0
1.5
2.0
$ seq -w -2 1
-2
-1
00
01
$ seq -f %03d 3
001
002
003
$ seq -f "n=%.1f" 1 2
n=1.0
n=2.0
$ seq -f "%5g|" 9 11
    9|
   10|
   11|
$ seq -f "%-3d|" 2
1  |
2  |
$ seq 5 1
? 0
$ seq 100000 | wc -l
100000
$ seq 1 0 5
seq: 0: an increment of 0 never ends
? 10
$ seq x
seq: x: not a number
? 10
$ seq -f %d%d 3
seq: %d%d: needs one %d, %f or %g
? 10
$ seq
seq: no last number (seq ? for usage)
? 10
$ seq 1 2 3 4
seq: 4: too many numbers (seq ? for usage)
? 10
$ seq -q 3
seq: -q: unknown option
? 10
