$ head n100.txt
line 1
line 2
line 3
line 4
line 5
line 6
line 7
line 8
line 9
line 10
$ head -3 n100.txt
line 1
line 2
line 3
$ head -n 2 a.txt b.txt
==> a.txt <==
one
two

==> b.txt <==
alpha
Beta
$ head -qn1 a.txt b.txt
one
alpha
$ head -c 7 b.txt
alpha
B
$ type n100.txt | head -n2
line 1
line 2
$ head -n x a.txt
head: x: not a count
? 10
$ head -n1 long.txt | wc -c
20001
