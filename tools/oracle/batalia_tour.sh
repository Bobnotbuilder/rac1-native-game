#!/bin/bash
# usage: batalia_tour.sh one <name> <frames> <hero_at> <seq> <shots> [extra env] | dumps
# Headless runs of Batalia (level 8), after umbris_tour.sh (Batalia part 2). A run starts the level (landing: game mode 6 to frame ~740,
# the arrival's scene: mode 2 to ~2700), gives ITEMS at 780, puts the hero with RAC1_HERO_AT and drives the pad with a seq: script
# (`a-b/p:in` = a press every p frames). Prints the game modes, the TODO_PORT lines other than the host's own stubs, the last [seq] line.
# BIN: the game (default build/rac1-native). OUT: the runs' directory.
# `dumps` writes oracle batch 160's RAM dumps into assets/dumps/oracle/batalia_part2/ and batch 146's Batalia pair into assets/dumps/oracle/trespasser_0411/.
# Scenarios used (Batalia part 2):
#   the Commando's exit (Orxon unlocked, 0x13dd40[10]): one cmd 11000 "1130@2720,3,0,1,3.14" "3400-5100/150:t;9500-10900:ly=-1" ...
#   the deserter (Gaspar, 2000 bolts): one d 10500 "1144@2720,3,0,1,3.14" "2800-4500/100:t;6500-8000/100:t" "" "RAC1_POKE8=2710:0x15ed9a=1"
#   the turret (class 1283's talk, then class 440): one t 8000 "1283@2720,3,0,1,3.14" "2800-5200/100:t;5300-7900:o+lx=0.6"
#   the Metal Detector (1283 forced to its state 5): one m 6000 "1283@2720,3,0,1,3.14" "2800-4800/100:t" "5010" "RAC1_POKE8=5000:0x1c05460=5"
#   the Trespasser socket: ITEMS=0xc,0xf,0x2,0x1a one p 3400 "-1@2720,216.0,200.9,37.0,-1.57" "2800-2860:o" "2950" "RAC1_EQUIP=0x1a"
#   a gold bolt: one g 3600 "1134@2720,0,0,0.5" "" "2760"
W=$(cd "$(dirname "$0")/../.." && pwd)
DUMPS=${RAC1_ORACLE_DUMPS:-$W/assets/dumps/oracle}  # where the oracle reads them (docs/oracle.md)
BIN=${BIN:-$W/build/rac1-native}
OUT=${OUT:-$W/build/oracle_runs/batalia_tour}
ITEMS=${ITEMS:-0xc,0xf,0x2}
HOST=${HOST:-'0x122298|0x2a08c8|0x1187b0|0x1187d0|0x200f18|0x24ff08|0x2900a0|0x2a8ce8'}
expand() {
  echo "$1" | tr ';' '\n' | awk -F'[-/:]' 'NF == 4 && $1 ~ /^[0-9]+$/ && $3 ~ /^[0-9]+$/ { for (f = $1; f < $2; f += $3) printf "%d+4:%s;", f, $4; next } { printf "%s;", $0 }'
}
run() {
  local name=$1 frames=$2 at=$3 seq=$4 shots=$5 extra=$6
  seq=$(expand "$seq")
  local d=$OUT/$name
  mkdir -p "$d"
  (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=8 RAC1_FRAMES="$frames" RAC1_GIVE_FRAME=780 RAC1_GIVE_ITEMS="$ITEMS" \
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
  DD=$DUMPS/batalia_part2; mkdir -p "$DD"
  ITEMS=0xc,0xf,0x2,0xd,0x5
  dump rain2800 2800 "" "0:l3"; dump arrival1500 1500 "" "0:l3"
  for c in 252 327 440 452 600 671 1135 1649; do dump c$c 2900 "$c@2720,3,0,1,3.14" "2740-2890:ly=-0.2"; done
  dump c1818 2900 "1818@2720,2,0,1,3.14" "0:l3" "RAC1_POKE8=2730:0x14142c=5"
  dump vb832 2950 "832@2720,2,0,1,3.14" "2800+4:o;2810-2950:ly=1" "RAC1_EQUIP=0xd"
  ITEMS=0xc,0xf,0x2
  for f in 2760 2800 2900 2960 3035 3050; do dump para$f $f "1023@2720,6,0,1,3.14" "2800-3900:ly=-0.3"; done
  dump c1629n 2900 "1629@2720,0,0,1,3.14" "0:l3"
  for f in 5003 5040 5150; do dump slot7_$f $f "1283@2720,3,0,1,3.14" "2800-4800/100:t" "RAC1_POKE8=5000:0x1c05460=5"; done
  DD=$DUMPS/trespasser_0411; mkdir -p "$DD"; ITEMS=0xc,0xf,0x2,0x1a
  dump pad_L8 2790 "-1@2720,216.0,200.9,37.0,-1.57" "2800-2860:o" "RAC1_EQUIP=0x1a"
  dump puzzle_L8 2950 "-1@2720,216.0,200.9,37.0,-1.57" "2800-2860:o" "RAC1_EQUIP=0x1a"
  exit
fi
[ "$1" = one ] && { shift; run "$@"; }
