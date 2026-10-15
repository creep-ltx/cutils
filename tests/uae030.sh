#!/usr/bin/env bash
# uae030.sh start|stop|reset - a second private FS-UAE for tests that need
# memory: the shared clean test Amiga a1200-32-mui5 (~/UAE/TestMachines,
# `amiga-test`, README there) as a 68030/50 with 32 MB more fast RAM, the
# emulated bsdsocket (127.0.0.1 is the PC), AmiSSL 5 in SYS:T/amissl
# (Assign LIBS: SYS:T/amissl ADD), volume AmigaOS3.2:, wasabid on port 1247
# (tests/w030). FAST=1: the emulator at full speed. Its own copy in
# ~/.cache/amiga-test/cutils-030 and its own systemd scope (2 GB at most);
# stop ends that one only.
M=a1200-32-mui5
A=(--as cutils-030 --port 1247 --label AmigaOS3.2)
X=(--cpu 030); [ -n "$FAST" ] && X+=(--config "uae_cpu_speed = max" --config "accuracy = 0")
AMISSL="$HOME/FS-UAE/Hard Drives/AmigaOS3.2/T/amissl"
case "$1" in
start)
  H=$(amiga-test $M dir "${A[@]}") || exit 1
  if [ ! -d "$H/T/amissl" ]; then
    cp -a "$AMISSL" "$H/T/amissl"
    printf 'Assign LIBS: SYS:T/amissl ADD\n' >> "$H/S/User-Startup"
  fi
  amiga-test $M start "${A[@]}" "${X[@]}" && echo up ;;
stop)  amiga-test $M stop "${A[@]}" ;;
reset) amiga-test $M reset "${A[@]}" ;;
*) echo "usage: uae030.sh start|stop|reset" >&2; exit 2 ;;
esac
