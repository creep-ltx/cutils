$ which ?
which [-a] name ...  |  FILE/A,NORES/S,RES/S,ALL/S
$ which List
AmigaOS3.2:C/List
? 0
$ C:Which List
AmigaOS3.2:C/List
? 0
$ which list
AmigaOS3.2:C/List
$ which Cd
INTERNAL Cd
? 0
$ which cd
INTERNAL cd
$ which Assign
RES Assign
? 0
$ which Assign NORES
AmigaOS3.2:C/Assign
$ which Assign RES
RES Assign
$ which List RES
? 5
$ which -a List
AmigaOS3.2:C/List
AmigaOS3.2:C/List
? 0
$ which List ALL
AmigaOS3.2:C/List
AmigaOS3.2:C/List
$ which Assign ALL
RES Assign
AmigaOS3.2:C/Assign
AmigaOS3.2:C/Assign
? 0
$ C:Which Assign ALL
RES Assign
AmigaOS3.2:C/Assign
AmigaOS3.2:C/Assign
? 0
$ which Cd ALL
INTERNAL Cd
? 5
$ C:Which Cd ALL
INTERNAL Cd
? 5
$ which nosuch
? 5
$ C:Which nosuch
? 5
$ which C:
AmigaOS3.2:C
$ which RAM:
Ram Disk:
$ which C:List
AmigaOS3.2:C/List
$ which C:List ALL
AmigaOS3.2:C/List
$ which FILE List
AmigaOS3.2:C/List
$ which FILE=List
AmigaOS3.2:C/List
$ which s:Startup-Sequence
? 5
$ which >NIL: a.txt
? 0
$ Echo >wx "hi"
$ Protect wx -e
$ which wx
? 5
$ MakeDir wdir
$ which wdir
? 5
$ Delete >NIL: wx wdir
$ which List Cd nosuch
AmigaOS3.2:C/List
INTERNAL Cd
? 5
$ which -a cd list
INTERNAL cd
AmigaOS3.2:C/List
AmigaOS3.2:C/List
? 5
$ which RES NORES List
? 5
$ which -x
which: -x: unknown option
? 10
$ which
which: no command named (which ? for usage)
? 10
