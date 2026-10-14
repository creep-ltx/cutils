$ grep cat g.txt
the cat sat
concatenate
? 0
$ grep -i cat g.txt
the cat sat
concatenate
Cat food
$ grep -w cat g.txt
the cat sat
$ grep -iw cat g.txt
the cat sat
Cat food
$ grep -c a g.txt
5
$ grep -v a g.txt
x+y=z
$ grep -n foo g.txt
3:Cat food
6:foo bar foo
$ grep -x "Cat food" g.txt
Cat food
$ grep dog g.txt
? 5
$ grep "(paren)" g.txt
a (paren) line
$ grep -E "\(paren\)" g.txt
a (paren) line
$ grep -E "sat|food" g.txt
the cat sat
Cat food
$ grep "sat\|food" g.txt
the cat sat
Cat food
$ grep "x+y" g.txt
x+y=z
$ grep -F "x+y" g.txt
x+y=z
$ grep "^c" g.txt
concatenate
$ grep "t$" g.txt
the cat sat
$ grep -oE "fo+" g.txt
foo
foo
foo
$ grep -o "^foo" g.txt
foo
$ grep -oi "c[a-z]t" g.txt
cat
cat
Cat
$ grep "[[:digit:]]" g.txt
? 5
$ grep -l a g.txt b.txt a.txt
g.txt
b.txt
$ grep -L alpha g.txt b.txt
g.txt
$ grep a g.txt b.txt
g.txt:the cat sat
g.txt:concatenate
g.txt:Cat food
g.txt:a (paren) line
g.txt:foo bar foo
b.txt:alpha
b.txt:Beta
b.txt:gamma delta
$ grep -h Beta g.txt b.txt
Beta
$ grep -c a #?.txt
a.txt:0
b.txt:3
crlf.txt:0
empty.txt:0
g.txt:5
long.txt:0
n100.txt:0
nonl.txt:1
$ grep -m 2 line n100.txt
line 1
line 2
$ grep -A 1 "line 50$" n100.txt
line 50
line 51
$ grep -C 1 -e "line 10$" -n n100.txt
9-line 9
10:line 10
11-line 11
$ grep -B1 -A1 "line [37]$" n100.txt
line 2
line 3
line 4
--
line 6
line 7
line 8
$ grep -A1 "line [23]$" n100.txt
line 2
line 3
line 4
$ type b.txt | grep -i BETA
Beta
$ grep -q alpha b.txt
? 0
$ grep alpha nope.txt b.txt
grep: nope.txt: object not found
b.txt:alpha
? 10
$ grep -s alpha nope.txt
? 10
$ grep "(paren" g.txt
a (paren) line
$ grep -E "(paren" g.txt
grep: (paren: bad pattern
? 10
$ grep
grep: no pattern (grep ? for help)
? 10
$ grep -c x long.txt
1
$ grep -r alpha RAM:cut
RAM:cut/b.txt:alpha
