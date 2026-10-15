$ top -b -n 1 -d 0.5 >RAM:topo
? 0
$ grep -c -E "^top - [0-9][0-9]:[0-9][0-9]:[0-9][0-9] up [0-9:]+.* [0-9]+ shells?" RAM:topo
1
$ grep -c -E "^Tasks: [0-9]+ total, 1 running, [0-9]+ ready, [0-9]+ waiting" RAM:topo
1
$ grep -c -E "^%Cpu: [0-9]+\.[0-9] busy, [0-9]+\.[0-9] idle" RAM:topo
1
$ grep -c -E "^(Chip|Fast): [0-9.]+[KMG] total, [0-9.]+[KMG] used, [0-9.]+[KMG] free, [0-9.]+[KMG] largest" RAM:topo
2
$ grep "^CLI" RAM:topo
CLI ADDRESS   PRI STATE  CPU%    TIME+  STACK   FREE COMMAND
$ grep -c -E "^ +[0-9]+ [0-9a-f]{8} +-?[0-9]+ run +[0-9]+\.[0-9] +[0-9]+:[0-9][0-9]\.[0-9][0-9] +[0-9]+ +[0-9]+ top" RAM:topo
1
$ grep -c -E " wait +[0-9]+\.[0-9] " RAM:topo >NIL:
? 0
$ grep -c "\[RAM\]" RAM:topo
1
$ Delete RAM:topo QUIET
$ top -b -n 2 -d 0.2 | grep -c "^Tasks:"
2
$ Run >NIL: top -b -n 1 -d 4 >RAM:topo2
$ Wait 1
$ top -b -n 1 -d 0.5 | grep -c -E " run +[0-9]+\.[0-9] +- +[0-9]+ +[0-9]+ top"
1
$ Wait 4
$ grep -c -E " run +[0-9]+\.[0-9] +[0-9]+:[0-9][0-9]\.[0-9][0-9] +[0-9]+ +[0-9]+ top" RAM:topo2
1
$ top -b -n 1 -d 0.5 | grep -c -E " run +[0-9]+\.[0-9] +[0-9]+:[0-9][0-9]\.[0-9][0-9] +[0-9]+ +[0-9]+ top"
1
$ Delete RAM:topo2 QUIET
$ Run >NIL: seq 1 100000000 >NIL:
$ top -b -n 1 -d 2 | grep -c -E " (8[0-9]|9[0-9]|100)\.[0-9] +0:0[1-9]\.[0-9][0-9] +[0-9]+ +[0-9]+ seq"
1
$ top -b -n 1 -d 1 | grep -c -E "^%Cpu: (9[0-9]|100)\.[0-9] busy"
1
$ kill seq
? 0
$ Wait 1
$ top -b -n 1 -d 1 | grep -c -E "[0-9] seq"
0
$ top -x
top: -x: unknown option
? 10
$ top -d abc
top: abc: not a delay in seconds
? 10
$ top -n 0
top: 0: not a count
? 10
$ top extra
top: extra: unexpected argument
? 10
