$ stat ?
stat [-L] [-t] [-c FORMAT] file | pattern ...
$ Delete >NIL: sz ALL QUIET
$ MakeDir sz
$ Echo >sz/f "hello"
$ FileNote sz/f "a note"
$ Protect sz/f rwd
$ SetDate sz/f 02-Jan-20 13:14:15
$ stat sz/f
   File: sz/f
   Type: file
   Size: 6 bytes, 1 block
Protect: ----rw-d
   Date: 2020-01-02 13:14:15 (Thursday)
Comment: a note
? 0
$ stat -c "%n %s %A %F %y %Y %C|%%" sz/f
sz/f 6 ----rw-d file 2020-01-02 13:14:15 1577970855 a note|%
$ stat -c "%b\t%s" sz/f
1	6
$ stat -t sz/f
sz/f 6 1 ----rw-d 2020-01-02 13:14:15
$ Protect sz/f +hspa
$ stat -c %A sz/f
hsparw-d
$ stat -c "%n %F" sz
sz drawer
$ ln -s sz/f sz/lnk
$ stat -c "%n|%F|%N" sz/lnk
sz/lnk|soft link|sz/lnk -> sz/f
$ stat -c %n sz/#?
sz/f
sz/lnk
$ stat -t sz/none#?
stat: sz/none#?: nothing matches
? 5
$ stat nofile
stat: nofile: object not found
? 5
$ stat
stat: no file named (stat ? for usage)
? 10
$ stat -q x
stat: -q: unknown option
? 10
$ Delete >NIL: sz ALL QUIET
