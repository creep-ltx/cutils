$ chmod ?
chmod [-Rvc] MODE file | pattern ...
$ rm -rf cz
$ MakeDir cz cz/sub
$ Echo >cz/f "x"
$ Echo >cz/sub/g "y"
$ Protect cz/f rwed
$ chmod -x cz/f
? 0
$ stat -c %A cz/f
----rw-d
$ chmod +x cz/f
$ stat -c %A cz/f
----rwed
$ chmod -w cz/f
$ stat -c %A cz/f
----r-e-
$ chmod +w cz/f
$ stat -c %A cz/f
----rwed
$ chmod +s cz/f
$ stat -c %A cz/f
-s--rwed
$ chmod +p,+a cz/f
$ stat -c %A cz/f
-sparwed
$ chmod -s,-p,-a cz/f
$ stat -c %A cz/f
----rwed
$ chmod +h cz/f
$ stat -c %A cz/f
h---rwed
$ chmod =rw cz/f
$ stat -c %A cz/f
----rw-d
$ chmod =re cz/f
$ stat -c %A cz/f
----r-e-
$ chmod u+wd cz/f
$ stat -c %A cz/f
----rwed
$ chmod a-x cz/f
$ stat -c %A cz/f
----rw-d
$ chmod 644 cz/f
$ stat -c %A cz/f
----rw-d
$ chmod 0400 cz/f
$ stat -c %A cz/f
----r---
$ chmod -v 755 cz/f
cz/f: ----r--- -> ----rwed
$ chmod -v 755 cz/f
cz/f: ----rwed (unchanged)
$ chmod -c 755 cz/f
$ chmod -c -d cz/f
cz/f: ----rwed -> ----rwe-
$ chmod go+r cz/f
? 0
$ chmod -R -w cz
? 0
$ stat -c "%n %A" cz cz/f cz/sub cz/sub/g
cz ----r-e-
cz/f ----r-e-
cz/sub ----r-e-
cz/sub/g ----r-e-
$ chmod -Rv +wd cz
cz: ----r-e- -> ----rwed
cz/f: ----r-e- -> ----rwed
cz/sub: ----r-e- -> ----rwed
cz/sub/g: ----r-e- -> ----rwed
$ chmod -c -e cz/#?
cz/f: ----rwed -> ----rw-d
cz/sub: ----rwed -> ----rw-d
$ ln -s f cz/l
$ chmod +x cz/l
chmod: cz/l: a soft link: left as it is
? 5
$ chmod +z cz/f
chmod: +z: not a mode (chmod ? for usage)
? 10
$ chmod 999 cz/f
chmod: 999: not a mode (chmod ? for usage)
? 10
$ chmod +x
chmod: no file named (chmod ? for usage)
? 10
$ chmod
chmod: no mode (chmod ? for usage)
? 10
$ chmod +x nofile
chmod: nofile: object not found
? 10
$ chmod +x cz/n#?
chmod: cz/n#?: nothing matches
? 5
$ rm -r cz
? 0
