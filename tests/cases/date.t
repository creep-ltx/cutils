$ date ?
date [DAY] [DATE] [TIME] [TO|VER file]  |  date [-u] [-r file | -d date] [+FORMAT]
$ Delete >NIL: dz dtz
$ Echo >dz "x"
$ SetDate dz 02-Jan-20 13:14:15
$ date -r dz
Thursday 02-Jan-20 13:14:15
? 0
$ date -r dz "+%Y-%m-%d %H:%M:%S"
2020-01-02 13:14:15
$ date -r dz "+%y|%b|%B|%a|%A|%j|%e|%p|%Z|%s"
20|Jan|January|Thu|Thursday|002| 2|PM||1577970855
$ date -r dz +%F_%T_%D_%R_%I_%l_%k_%u_%w_%C_%h_%%
2020-01-02_13:14:15_01/02/20_13:14_01_ 1_13_4_4_20_Jan_%
$ date -r dz "+a%tb%q"
a	b%q
$ date -u -r dz +%H
13
$ date -d 2020-02-29 +%A
Saturday
$ date -d "2021-03-04 05:06:07" "+%F %T"
2021-03-04 05:06:07
$ date -d 14-Oct-26 +%F
2026-10-14
$ date -d bogus
date: bogus: not a date
? 10
$ date -r nofile
date: nofile: object not found
? 10
$ date -r dz extra
date: extra: not a +FORMAT (date ? for usage)
? 10
$ date -x
date: -x: unknown option
? 10
$ date | grep -c "^[A-Z][a-z]**day [0-3][0-9]-[A-Z][a-z][a-z]-[0-9][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9]"
1
$ date TO dtz
$ grep -c "^[A-Z][a-z]**day [0-3][0-9]-[A-Z][a-z][a-z]-[0-9][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9]" dtz
1
$ date 31-feb-30
date: bad number
- use DD-MMM-YY or YYYY-MM-DD etc. to set date
       HH:MM:SS etc. to set time
? 10
$ date 25:00
date: bad number
- use DD-MMM-YY or YYYY-MM-DD etc. to set date
       HH:MM:SS etc. to set time
? 10
$ date 1-jan-30 TO
date: TO: needs a file name
? 10
$ date 1-jan-30 12:00:00 1-jan-31 2:00
date: 2:00: too many arguments (date ? for usage)
? 10
$ date 1-jan-30 12:00:00 TO dtz
? 0
$ Type dtz
Tuesday 01-Jan-30 12:00:00
$ C:Date | grep -c "^Tuesday 01-Jan-30 12:0"
1
$ date | grep -c "^Tuesday 01-Jan-30 12:0"
1
$ date 14-feb-31
$ date "+%F %H"
2031-02-14 12
$ date 23:00 Monday
$ date "+%F %a %H:%M"
2031-02-17 Mon 23:00
$ date tomorrow
$ date +%F
2031-02-18
$ date Tuesday
$ date +%F
2031-02-25
$ date 01-03-32
$ date +%F
2032-03-01
$ date yesterday 8:05 TO=dtz
$ Type dtz
Sunday 29-Feb-32 08:05:00
$ date 1-Jan-77
$ date +%F%a
2077-01-01Fri
$ SetClock LOAD
$ date +%Y | grep -c "^20[2-6][0-9]$"
1
$ Delete >NIL: dz dtz
