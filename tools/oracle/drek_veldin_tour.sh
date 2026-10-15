#!/bin/bash
# usage: drek_veldin_tour.sh one <level> <name> <frames> <hero_at> <seq> <shots> [extra env] | dumps
# Headless runs of Drek's fleet (level 17) and Veldin's finale (level 18), after batalia_tour.sh (the 0.3.0 port of levels 17 / 18). A run starts
# the level (the landing: game mode 6 to frame ~740), gives ITEMS at 780, puts the hero with RAC1_HERO_AT and drives the pad with a seq: script
# (`a-b/p:in` = a press every p frames). Prints the game modes, the TODO_PORT lines other than the host's own stubs, the last [seq] line.
# BIN: the game (default build/rac1-native). OUT: the runs' directory.
# `dumps` writes oracle batch 271's RAM dumps into assets/dumps/oracle/drek_veldin/ (the level's own data are laid out the same in every run: on
# level 18 the final boss, class 1422, is the moby at 0x1f07c80, its pvars at 0x1f552d0):
#   fight<f>: the boss's state set to 27 (0x1b) at frame 1300, the hero next to it: its intro scene (3), then the fight (state 7 ..)
#   credits<f> / movie / menu<f>: the boss's state 27 with its step +0x34c = 8 (the fight won): its last scene (5), the credits (game mode 7, from
#   the frame ~5560), the last movie (game mode 1), then the menu at the end of the game (menu state 0x21, game mode 3, from the frame ~17920)
#   p17_<f>: level 17, the hero put at 1000 next to a Sentry-bot (class 44) where the level kills him: the respawn's particles (types 0x47 / 0x4f)
W=$(cd "$(dirname "$0")/../.." && pwd)
DUMPS=${RAC1_ORACLE_DUMPS:-$W/assets/dumps/oracle}  # where the oracle reads them (docs/oracle.md)
BIN=${BIN:-$W/build/rac1-native}
OUT=${OUT:-$W/build/oracle_runs/drek_veldin_tour}
ITEMS=${ITEMS:-0xc,0xf,0x2}
HOST=${HOST:-'0x122298|0x2a08c8|0x1187b0|0x1187d0|0x200f18|0x24ff08|0x2900a0|0x2a8ce8|0x122e68|0x1188a0|0x1f8540'}
expand() {
  echo "$1" | tr ';' '\n' | awk -F'[-/:]' 'NF == 4 && $1 ~ /^[0-9]+$/ && $3 ~ /^[0-9]+$/ { for (f = $1; f < $2; f += $3) printf "%d+4:%s;", f, $4; next } { printf "%s;", $0 }'
}
run() {
  local level=$1 name=$2 frames=$3 at=$4 seq=$5 shots=$6 extra=$7
  seq=$(expand "$seq")
  local d=$OUT/L$level-$name
  mkdir -p "$d"
  (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL="$level" RAC1_FRAMES="$frames" RAC1_GIVE_FRAME=780 RAC1_GIVE_ITEMS="$ITEMS" \
     ${at:+RAC1_HERO_AT="$at"} RAC1_SIM_INPUT="seq:$seq" RAC1_SEQ_TRACE=10 RAC1_SHOTS="$shots" $extra timeout 3000 "$BIN" > run.log 2>&1)
  local rc=$?
  echo "L$level $name rc=$rc modes: $(grep '^\[seq\]' "$d/run.log" | awk '{print $11}' | uniq -c | tr '\n' ' ' | cut -c1-200)"
  local todo
  todo=$(grep TODO_PORT "$d/run.log" | grep -Ev "$HOST" | sort -u | sed 's/^\[TODO_PORT\] Unported call to //' | tr '\n' ';')
  [ -n "$todo" ] && echo "  TODO_PORT: $todo"
  grep '^\[seq\]' "$d/run.log" | tail -1 | sed 's/^/  last: /'
}
if [ "$1" = dumps ]; then
  DD=$DUMPS/drek_veldin; mkdir -p "$DD"
  BOSS="1422@1250,-6,0,0"
  # the fight: several frames of it in one run (RAC1_RAMDUMP takes one frame: a run per dump)
  for f in 4200 5000 6000; do
    run 18 fight$f $((f + 3)) "$BOSS" "3600-$f/90:s" "" "RAC1_RAMHASH=/dev/null RAC1_RAMDUMP=$f,$DD/fight$f.bin RAC1_POKE8=1300:0x1f07ca0=0x1b" > /dev/null
    ls -l "$DD/fight$f.bin"
  done
  for f in 5700 9000 18000 18300; do
    run 18 end$f $((f + 3)) "$BOSS" "0:l3" "" "RAC1_RAMHASH=/dev/null RAC1_RAMDUMP=$f,$DD/end$f.bin RAC1_POKE8=1300:0x1f07ca0=0x1b,1300:0x1f5561c=8" > /dev/null
    ls -l "$DD/end$f.bin"
  done
  for f in 1002 1005 1009; do
    run 17 p17_$f $((f + 3)) "44@1000,3,0,1" "0:l3" "" "RAC1_RAMHASH=/dev/null RAC1_RAMDUMP=$f,$DD/p17_$f.bin" > /dev/null
    ls -l "$DD/p17_$f.bin"
  done
  exit
fi
[ "$1" = one ] && { shift; run "$@"; }
