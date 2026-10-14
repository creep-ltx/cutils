$ ps | head -n 1
CLI ADDRESS    PRI STATE  COMMAND
? 0
$ ps -l | head -n 1
CLI ADDRESS    PRI STATE  STACK  FREE SIGWAIT    COMMAND
$ ps ps >RAM:pso
$ cut -c27- RAM:pso
COMMAND
ps
$ Delete RAM:pso QUIET
$ ps -e | grep -c "\[RAM\]"
1
$ ps nothinglikethis
CLI ADDRESS    PRI STATE  COMMAND
ps: nothing matches
? 5
$ ps -x
ps: -x: unknown option
? 10
