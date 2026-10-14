$ xargs echo <xa.txt
one two three four five six
$ xargs -n 2 echo <xa.txt
one two
three four five
six
$ xargs -L 1 echo <xa.txt
one two
three four five
six
$ xargs -I {} echo [{}] <xa.txt
[one two]
[three four five]
[six]
$ xargs -s 20 echo <xa.txt
one two
three four
five six
$ xargs echo <xa-q.txt
a b x*y he said "hi" p * q

$ xargs <xa.txt
one two three four five six
$ xargs echo <empty.txt
? 0
$ xargs -n 1 wc -l <xa-names.txt
6 a.txt
3 b.txt
$ xargs grep -q zzz <xa-names.txt
? 5
$ xargs -n 0 echo
xargs: 0: not a count
? 10
$ xargs -I {} <xa.txt
xargs: -I needs a command
? 10
$ xargs -s 7 echo <xa.txt
xargs: one: longer than a command line
? 10
$ xargs -n 2 -t echo x <xa-names.txt
echo x a.txt b.txt
x a.txt b.txt
