$ cut -d: -f2 ct.txt
two
b
no colon here
2
$ cut -d: -f2 -s ct.txt
two
b
2
$ cut -d: -f1,3- ct.txt
one:three:four
a
no colon here
1:3:4:5:6
$ cut -d: -f-2 ct.txt
one:two
a:b
no colon here
1:2
$ cut -d: -f3,1 ct.txt
one:three
a
no colon here
1:3
$ cut -c1-3 ct.txt
one
a:b
no 
1:2
$ cut -c 5- ct.txt
two:three:four

olon here
3:4:5:6
$ cut -b2,4 ct.txt
n:
:
oc
::
$ cut -d: -f2 --complement ct.txt
one:three:four
a
no colon here
1:3:4:5:6
$ cut -d: -f1,2 --output-delimiter=+ ct.txt
one+two
a+b
no colon here
1+2
$ cut -f2 ct-tab.txt
b
e
$ cut -c2- --complement ct.txt
o
a
n
1
$ cut ct.txt
cut: -f, -c or -b, with a list
? 10
$ cut -f 0 ct.txt
cut: 0: not a list (N, N-M, N-, -M, with commas)
? 10
$ cut -f 3-2 ct.txt
cut: 3-2: not a list (N, N-M, N-, -M, with commas)
? 10
$ cut -f 1 -c 2 ct.txt
cut: only one of -f, -c, -b
? 10
$ type ct.txt | cut -d: -f 2
two
b
no colon here
2
$ cut -d "\t" -f 1 ct-tab.txt
a
d
$ cut -d : -f 2- ct.txt ct-tab.txt
two:three:four
b
no colon here
2:3:4:5:6
a	b	c
d	e
