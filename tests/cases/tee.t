$ type b.txt | tee RAM:tee1 RAM:tee2 | wc -l
3
$ cat RAM:tee1 RAM:tee2
alpha
Beta
gamma delta
alpha
Beta
gamma delta
$ type nonl.txt | tee -a RAM:tee1 >NIL:
$ tail -2 RAM:tee1
gamma delta
no newline at end
$ type b.txt | tee RAM:nodir/x
tee: RAM:nodir/x: object not found
alpha
Beta
gamma delta
? 10
$ Delete RAM:tee1 RAM:tee2 QUIET
