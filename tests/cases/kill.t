$ Run >NIL: Wait 60
$ Wait 1
$ ps wait | cut -c27-
COMMAND
Wait
$ kill wait
? 0
$ Wait 1
$ ps wait
CLI ADDRESS    PRI STATE  COMMAND
ps: nothing matches
? 5
$ kill nosuchthing
kill: nosuchthing: no such program (ps -e lists them)
? 5
$ kill -9 1
kill: the Amiga can not force a program out safely; kill sends a break (Ctrl-C)
? 10
$ kill -l
C (INT TERM HUP 2 15)  D  E  F
$ kill -QUIT 1
kill: -QUIT: unknown signal (kill -l lists them)
? 10
$ kill
kill: which program? (a Shell's number, 0x address or name; kill ? for help)
? 10
