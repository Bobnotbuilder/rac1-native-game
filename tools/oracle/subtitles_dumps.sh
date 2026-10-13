#!/bin/bash
# usage: subtitles_dumps.sh : oracle batch 149's inputs (task 0416), two headless runs with the Options' Subtitles switch
# (0x15ee40) poked on at frame 2, each RAM dumped in a game-mode-2 scene with a subtitle table:
#   assets/dumps/oracle/subtitles_0416/veldin_scene2_L0.bin    Veldin, the hero put at the finished ship (class 834) at 1100: scene 1,
#                                                        then scene 2 (Clank meets Ratchet), frame 3000 (the scene's frame 1362);
#   assets/dumps/oracle/subtitles_0416/umbris_arrival_L7.bin   Umbris's arrival scene (scene 0, Qwark's welcome), frame 1500 (its 761).
# Headless (no window); needs $RAC1_BUILD/rac1-native (default build/). One run at a time.
W=$(cd "$(dirname "$0")/../.." && pwd)
DUMPS=${RAC1_ORACLE_DUMPS:-$W/assets/dumps/oracle}  # where the oracle reads them (docs/oracle.md)
BIN=${RAC1_BUILD:-build}
case "$BIN" in /*) ;; *) BIN=$W/$BIN ;; esac
OUT=$DUMPS/subtitles_0416
d=$W/build/oracle_runs/subtitles_dumps
mkdir -p "$OUT" "$d"
cd "$d" || exit 1
ln -sfn "$W/assets" assets
RAC1_HEADLESS=1 RAC1_NO_AUDIO=1 RAC1_LEVEL=0 RAC1_SIM_INPUT=ship RAC1_HERO_AT=834@1100 RAC1_FRAMES=3002 RAC1_POKE8=2:0x15ee40=1 \
  RAC1_RAMHASH=ramhash.bin RAC1_RAMDUMP="3000,$OUT/veldin_scene2_L0.bin" timeout 1500 "$BIN/rac1-native" > veldin.log 2>&1
echo "veldin rc=$?"
RAC1_HEADLESS=1 RAC1_LEVEL=7 RAC1_FRAMES=1502 RAC1_POKE8=2:0x15ee40=1 \
  RAC1_RAMHASH=ramhash.bin RAC1_RAMDUMP="1500,$OUT/umbris_arrival_L7.bin" timeout 1500 "$BIN/rac1-native" > umbris.log 2>&1
echo "umbris rc=$?"
rm -f ramhash.bin
ls -l "$OUT"
