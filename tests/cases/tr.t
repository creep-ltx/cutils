$ tr a-z A-Z <tr.txt
HELLO, WORLD!  ÄRLIG OCH öPPEN.
AAABBBCCC   SPACES
$ tr -d "[:punct:]" <tr.txt
Hello World  Ärlig och öppen
aaabbbccc   spaces
$ tr -s " " <tr.txt
Hello, World! Ärlig och öppen.
aaabbbccc spaces
$ tr -s a-z <tr.txt
Helo, World!  Ärlig och öpen.
abc   spaces
$ tr -cd "a-z\n" <tr.txt
elloorldrligochppen
aaabbbcccspaces
$ tr -s a-c x <tr.txt
Hello, World!  Ärlig oxh öppen.
x   spxes
$ tr a-f [x*2]y <tr.txt
Hyllo, Worly!  Ärlig oyh öppyn.
xxxxxxyyy   spxyys
$ tr -d "\054\041" <tr.txt
Hello World  Ärlig och öppen.
aaabbbccc   spaces
$ tr -ds " " a-z <tr.txt
Helo,World!Ärligochöpen.
abcspaces
$ tr a-c [x*]Z <tr.txt
Hello, World!  Ärlig oZh öppen.
xxxxxxZZZ   spxZes
$ tr -c "a-z\n" "_" <tr.txt
_ello___orld____rlig_och__ppen_
aaabbbccc___spaces
$ tr [:lower:] [:upper:] <tr.txt
HELLO, WORLD!  ÄRLIG OCH ÖPPEN.
AAABBBCCC   SPACES
$ tr [:upper:] [:lower:] <tr.txt
hello, world!  ärlig och öppen.
aaabbbccc   spaces
$ type tr.txt | tr -d [:alpha:]
, !    .
   
$ tr a
tr: no SET2
? 10
$ tr z-a x
tr: a range is backwards
? 10
$ tr -d a b
tr: b: -d takes one set (with -s, two)
? 10
$ tr [:nope:] x
tr: nope: no such class
? 10
