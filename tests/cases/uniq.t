$ uniq uq.txt
apple
Apple
banana
cherry
x one
y one
apple
$ uniq -c uq.txt
      2 apple
      1 Apple
      1 banana
      3 cherry
      1 x one
      1 y one
      1 apple
$ uniq -i uq.txt
apple
banana
cherry
x one
y one
apple
$ uniq -ic uq.txt
      3 apple
      1 banana
      3 cherry
      1 x one
      1 y one
      1 apple
$ uniq -d uq.txt
apple
cherry
$ uniq -u uq.txt
Apple
banana
x one
y one
apple
$ uniq -c -f 1 uq.txt
      7 apple
      2 x one
      1 apple
$ uniq -s 1 uq.txt
apple
banana
cherry
x one
apple
$ uniq -c -s 2 -w 1 uq.txt
      3 apple
      1 banana
      3 cherry
      2 x one
      1 apple
$ uniq uq.txt uq-out.txt
$ type uq-out.txt
apple
Apple
banana
cherry
x one
y one
apple
$ type uq.txt | uniq -d
apple
cherry
$ uniq a b c
uniq: c: one input and one output at most
? 10
$ uniq -f x uq.txt
uniq: x: not a count
? 10
$ uniq nothere.txt
uniq: nothere.txt: object not found
? 5
$ uniq -du uq.txt
? 0
