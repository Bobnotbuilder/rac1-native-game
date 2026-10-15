#!/bin/bash
# usage: LEVEL=13|14 gemlik_oltanis_tour.sh one <name> <frames> <hero_at> <seq> <shots> [extra env] | dumps
# Headless runs of Gemlik Base (level 13, the code's "Oltanis Orbit") and Oltanis (level 14), after batalia_tour.sh. A run starts the
# level (RAC1_LEVEL), gives ITEMS at GIVE (default 780), puts the hero with RAC1_HERO_AT and drives the pad with a seq: script
# (`a-b/p:in` = a press every p frames). Prints the game modes, the TODO_PORT lines other than the host's own stubs, the last [seq] line.
# BIN: the game (default build/rac1-native). OUT: the runs' directory.
# `dumps` writes the RAM dumps of oracle batches 222 / 223 into assets/dumps/oracle/gemlik_oltanis/ (the command lines below).
W=$(cd "$(dirname "$0")/../.." && pwd)
DUMPS=${RAC1_ORACLE_DUMPS:-$W/assets/dumps/oracle}  # where the oracle reads them (docs/oracle.md)
BIN=${BIN:-$W/build/rac1-native}
LEVEL=${LEVEL:-13}
OUT=${OUT:-$W/build/oracle_runs/tour_l$LEVEL}
ITEMS=${ITEMS:-0xc,0xf,0x2}
GIVE=${GIVE:-780}
HOST=${HOST:-'0x122298|0x2a08c8|0x1187b0|0x1187d0|0x200f18|0x24ff08|0x2900a0|0x2a8ce8'}
expand() {
  echo "$1" | tr ';' '\n' | awk -F'[-/:]' 'NF == 4 && $1 ~ /^[0-9]+$/ && $3 ~ /^[0-9]+$/ { for (f = $1; f < $2; f += $3) printf "%d+4:%s;", f, $4; next } { printf "%s;", $0 }'
}
run() {
  local name=$1 frames=$2 at=$3 seq=$4 shots=$5 extra=$6
  seq=$(expand "$seq")
  local d=$OUT/$name
  mkdir -p "$d"
  (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=$LEVEL RAC1_FRAMES="$frames" RAC1_GIVE_FRAME=$GIVE RAC1_GIVE_ITEMS="$ITEMS" \
     RAC1_HERO_AT="$at" RAC1_SIM_INPUT="seq:$seq" RAC1_SEQ_TRACE=10 RAC1_SHOTS="$shots" $extra timeout 2400 "$BIN" > run.log 2>&1)
  local rc=$?
  echo "$name rc=$rc modes: $(grep '^\[seq\]' "$d/run.log" | awk '{print $11}' | uniq -c | tr '\n' ' ' | cut -c1-200)"
  local todo
  todo=$(grep TODO_PORT "$d/run.log" | grep -Ev "$HOST" | sort -u | sed 's/^\[TODO_PORT\] Unported call to //' | tr '\n' ';')
  [ -n "$todo" ] && echo "  TODO_PORT: $todo"
  grep '^\[seq\]' "$d/run.log" | tail -1 | sed 's/^/  last: /'
}
dump() {  # name frame hero_at seq [extra]
  RAC1_RAMHASH=/dev/null run "$1" $(($2 + 3)) "$3" "$4" "" "RAC1_RAMHASH=/dev/null RAC1_RAMDUMP=$2,$DD/$1.bin $5" > /dev/null
  ls -l "$DD/$1.bin"
}
if [ "$1" = dumps ]; then
  DD=$DUMPS/gemlik_oltanis; mkdir -p "$DD"
  # batch 222 (Gemlik Base, LEVEL=13): the landing (the ship's particles, type 0x44), Qwark's space fight (the hero put next to class 388,
  # its scene, then the ship's run with the lasers fired: cross every 6 frames)
  LEVEL=13; GIVE=780
  for f in 60 200 400; do dump land${f}_L13 $f "" "0:l3"; done
  for f in 2000 2800 3500 4000 5000 6500; do dump boss${f}_L13 $f "388@800,12,0,1,3.14" "2700-$f/6:x"; done
  # batch 223 (Oltanis, LEVEL=14): the arrival's scene (class 684's scene 0, the extras' effect in its frames 0x1c..0x21), the town after it,
  # the hero by class 712 / 924 / 851 / 1395 (talks, the wrench), the scenes 924's and 851's talks start
  LEVEL=14; GIVE=1600
  for f in 30 60 300 1000; do dump arr${f}_L14 $f "" "0:l3"; done
  dump town1700_L14 1700 "" "0:l3"
  for c in 712 924 851 1395 685; do dump c${c}_L14 2400 "$c@1650,3,0,1,3.14" "1700-2390/100:t;1720-2390/25:s;2000-2390:ly=-0.5"; done
  dump talk924_L14 2900 "924@1650,3,0,1,3.14" "1700-2890/100:t;1720-2890/25:s;2000-2400:ly=-0.5"
  dump talk851_L14 2900 "851@1650,3,0,1,3.14" "1700-2890/100:t;1720-2890/25:s;2000-2400:ly=-0.5"
  exit
fi
[ "$1" = one ] && { shift; run "$@"; }
