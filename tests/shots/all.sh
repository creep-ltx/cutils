#!/usr/bin/env bash
# all.sh - every screenshot in docs/images, on rig.sh's FS-UAE (started if not up).
# wget's needs tests/httpd.py on port 8481 (started here).
cd "$(dirname "$0")" || exit 1
bash rig.sh start >/dev/null || exit 1
(cd .. && exec python3 httpd.py 8481 8482 >/tmp/cutils-shots-httpd.log 2>&1) &
WEB=$!
sleep 2
PR=1.AmigaOS3.2:T/demo/MyGame ./shot.sh files 8 '!CD MyGame' 'ls' 'ls -la' 'ls -lhS gfx' 'du -h' 'df -h'
./shot.sh text 8 'grep -n -i -E "assign|setenv" new.txt' 'diff -u old.txt new.txt' 'sort names.txt | uniq -c | sort -rn' 'sed -n "s/^SetEnv //p" new.txt' 'find MyGame -name "#?.c" | xargs wc -l'
./shot.sh archives 10 'file MyGame/#? MyGame/gfx/#?' 'tar tvzf MyGame.tgz'
./shot.sh system 8 'uptime' 'free -h' 'ps' 'which ls Ed' 'date "+%Y-%m-%d %H:%M:%S"'
./shot.sh top 12 '!Run >NIL: C:Wait 120' 'top'
./shot.sh ctop 14 'ctop'
./shot.sh wget 6 '!CD RAM:' 'wget http://127.0.0.1:8481/hello.txt' 'wget http://127.0.0.1:8481/slow.bin'
# man last: ending CTerm with man open resets the Amiga (TODO.md 2)
./shot.sh man 8 'man ls'
kill $WEB 2>/dev/null
