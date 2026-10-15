$ touch ?
touch [-c] [-d date | -r file] file | pattern ...
$ touch
touch: no file named (touch ? for usage)
? 10
$ touch -x a
touch: -x: unknown option
? 10
$ rm -rf tz
$ MakeDir tz
$ touch tz/new
? 0
$ List tz/new LFORMAT "%N %L %D|" | sed "s/ *|/|/"
new empty Today|
$ touch -c tz/none
? 0
$ rm tz/none
rm: tz/none: object not found
? 5
$ Echo >tz/old "keep"
$ SetDate tz/old 02-Jan-20 13:14:15
$ touch tz/old
? 0
$ List tz/old LFORMAT "%N %L %D|" | sed "s/ *|/|/"
old 5 Today|
$ Type tz/old
keep
$ touch -d 2020-02-29 tz/old
? 0
$ List tz/old LFORMAT "%N %D %T"
old 29-Feb-20 00:00:00
$ touch -d "1999-12-31 23:59:58" tz/old tz/new2
? 0
$ List tz LFORMAT "%N %L %D|" | sed "s/ *|/|/"
new2 empty 31-Dec-99|
old 5 31-Dec-99|
new empty Today|
$ touch -d 2025-10-14T08:05 -c tz/old
? 0
$ List tz/old LFORMAT "%D %T"
14-Oct-25 08:05:00
$ touch -d "01-Mar-21 10:11:12" tz/old
? 0
$ List tz/old LFORMAT "%D %T"
01-Mar-21 10:11:12
$ touch -d 2021-02-29 tz/old
touch: 2021-02-29: not a date
? 10
$ touch -d 1977-12-31 tz/old
touch: 1977-12-31: not a date
? 10
$ touch -d "2021-01-01 25:00" tz/old
touch: 2021-01-01 25:00: not a date
? 10
$ touch -d tuesdayish tz/old
touch: tuesdayish: not a date
? 10
$ touch -r tz/new2 tz/new
? 0
$ List tz/new LFORMAT "%D %T"
31-Dec-99 23:59:58
$ touch -r tz/nope tz/new
touch: tz/nope: object not found
? 10
$ touch -d 2001-01-01 tz/n#?
? 0
$ List tz LFORMAT "%N %D"
new2 01-Jan-01
old 01-Mar-21
new 01-Jan-01
$ touch tz/z#?
touch: tz/z#?: nothing matches
? 5
$ touch -d 2001-01-01 tz
? 0
$ List PAT tz LFORMAT "%N %D"
tz 01-Jan-01
$ touch tz/nodir/f
touch: tz/nodir/f: object not found
? 10
$ rm -r tz
