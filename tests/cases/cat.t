$ cat b.txt
alpha
Beta
gamma delta
? 0
$ cat -n a.txt
     1	one
     2	two
     3	
     4	
     5	 three four
     6	five
$ cat -b a.txt
     1	one
     2	two


     3	 three four
     4	five
$ cat -sE a.txt
one$
two$
$
 three four$
five$
$ cat nonl.txt empty.txt b.txt
no newline at endalpha
Beta
gamma delta
$ type b.txt | cat - nonl.txt
alpha
Beta
gamma delta
no newline at end
$ cat -v crlf.txt
dos one^M
dos two^M
$ cat #?.txt | wc -l
119
$ cat *.txt | wc -l
119
$ cat nope.txt
cat: nope.txt: object not found
? 5
$ cat -x
cat: -x: unknown option
? 10
$ cat ?
cat [-bnsEv] [file | pattern | - ...]
$ cat z#?
cat: z#?: nothing matches
? 5
$ cat RAM:cut
cat: RAM:cut: is a drawer
? 5
$ cat long.txt | wc -c
20007
