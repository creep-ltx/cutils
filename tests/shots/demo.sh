#!/usr/bin/env bash
# demo.sh DIR - the files the screenshots show: two versions of a startup snippet, a list
# of names, and MyGame/ with one file of every kind ls colours (from tests/files)
set -e
T=$1; F=$(cd "$(dirname "$0")/../files" && pwd)
mkdir -p "$T/MyGame/src" "$T/MyGame/gfx"; cd "$T"
printf '; my startup additions\nAssign Work: DH1:\nAssign Projects: Work:Projects\nPath Work:C ADD\nSetEnv EDITOR C:Ed\nRun >NIL: C:wasabid\n' > old.txt
printf '; my startup additions\nAssign Work: DH1:\nAssign Projects: Work:Projects\nPath Work:C ADD\nSetEnv EDITOR Work:Sumi/sumi\nSetEnv PAGER C:man\nRun >NIL: C:wasabid\n' > new.txt
printf 'Lotus Turbo Challenge\nTurrican II\nLemmings\nTurrican II\nShadow of the Beast\nLemmings\nLemmings\nSpeedball 2\n' > names.txt
G=MyGame
cp "$F/fx.mod" $G/title.mod; cp "$F/fx.8svx" $G/jump.8svx; cp "$F/fx.lha" $G/MyGame-0.3.lha
cp "$F/fx.info" $G/MyGame.info; cp "$F/fx.guide" $G/MyGame.guide; cp "$F/fx.exe" $G/MyGame
cp "$F/fx.ilbm" $G/gfx/logo.iff; cp "$F/fx_ham.iff" $G/gfx/backdrop.iff; cp "$F/fx.png" $G/gfx/sprites.png
printf -- '----rwed 2026-10-16 00:00:00.00 \n' > $G/MyGame.uaem
printf 'Echo "Installing MyGame"\nCopy MyGame SYS:Games/\n' > $G/Install
printf -- '-s--rwed 2026-10-16 00:00:00.00 \n' > $G/Install.uaem
printf 'MyGame 0.3\n\nA small game.\n' > $G/ReadMe.txt
printf '#include <stdio.h>\nint main(void) { return 0; }\n' > $G/src/main.c
printf 'int score;\n' > $G/src/score.c; printf 'extern int score;\n' > $G/src/game.h
tar czf MyGame.tgz --exclude='*.uaem' --owner=0 --group=0 MyGame   # made on the PC (TODO.md 1)
