$ tail n100.txt
line 91
line 92
line 93
line 94
line 95
line 96
line 97
line 98
line 99
line 100
$ tail -3 n100.txt
line 98
line 99
line 100
$ tail -n +98 n100.txt
line 98
line 99
line 100
$ type n100.txt | tail -n 2
line 99
line 100
$ type n100.txt | tail -n +99
line 99
line 100
$ tail -2 nonl.txt b.txt
==> nonl.txt <==
no newline at end
==> b.txt <==
Beta
gamma delta
$ tail -c 6 b.txt
delta
$ type b.txt | tail -c 6
delta
$ tail -c +7 b.txt
Beta
gamma delta
$ tail -1 long.txt
short
$ tail -n 1 empty.txt
$ tail -n 0 b.txt
$ tail -n 20000 a.txt
one
two


 three four
five
$ tail -2 long.txt | wc -c
20007
$ tail +99 n100.txt
line 99
line 100
