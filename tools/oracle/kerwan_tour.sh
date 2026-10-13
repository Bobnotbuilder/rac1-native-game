#!/bin/bash
# usage: kerwan_tour.sh [scenario ...] : headless runs of Kerwan (level 3), one per scenario (task 0385).
#
# Each scenario starts the level (its landing, game mode 6, ends near frame 740), gives the Kerwan items (the Grindboots 0x1d,
# the Swingshot 0xc, the Heli-Pack 0x2, the Blaster 0xf) and then puts the hero somewhere with RAC1_HERO_AT (the game's own
# placement) and drives the pad with a RAC1_SIM_INPUT=seq: script (src/host/sim_script.h). It prints, per scenario:
# the exit code, the hero's states seen (RAC1_SEQ_TRACE), the TODO_PORT lines other than the host's own stubs and the ship's
# draw callbacks, and keeps the screenshots (RAC1_SHOTS) in $OUT/<scenario>/. With no argument every scenario runs.
# Headless (no window); needs $RAC1_BUILD/rac1-native (default build/). One run at a time.
W=$(cd "$(dirname "$0")/../.." && pwd)
DUMPS=${RAC1_ORACLE_DUMPS:-$W/assets/dumps/oracle}  # where the oracle reads them (docs/oracle.md)
BIN=${RAC1_BUILD:-build}  # (task 0385c: the build directory, as the other tools/oracle scripts take it; relative to the repo or absolute)
case "$BIN" in /*) ;; *) BIN=$W/$BIN ;; esac
OUT=${OUT:-$W/build/oracle_runs/kerwan_tour}
ITEMS=${ITEMS:-0x1d,0xc,0x2,0xf}
# the host's stubs every level logs (vsync, DMA kick 0x2a08c8 / Kerwan's 0x28d210, DMA handlers, display packet) and the ship's
# draw callbacks (Veldin's 0x28e670 / 0x28efc8 on Kerwan: shared code, not Kerwan's), the pause menu's VU moby draw 0x24ff08 (shared)
HOST=${HOST:-'0x122298|0x2a08c8|0x28d210|0x1187b0|0x1187d0|0x200f18|0x27afb8|0x27b910|0x23d968|0x2900a0|0x24ff08'}

# name | frames | RAC1_HERO_AT | seq script | shots | extra env
SCEN=(
  "rail0|1300|-1@800,197.5,148.4,81.5,1.79|0:l3|880,950,1100|"
  "rail1|1500|-1@800,120.5,187.6,92.5,-1.6|0:l3|860,950,1200|"
  "rail2|1300|-1@800,139.0,118.3,72.3,0.2|0:l3|860,950,1100|"
  "helipack|1300|-1@800,264,141,50.5,1.57|820+4:x;834-1100:x;820-1100:ly=-1|840,870,950|"
  "swing758a|1100|758@800,0,-6,-3|870+6:o;900+6:o;930+40:o|880,905,940|RAC1_EQUIP=0xc"
  "swing758b|1100|758@800,0,-12,-6|870+6:o;900+6:o;930+40:o|880,905,940|RAC1_EQUIP=0xc"
  "swing758c|1100|758@800,10,0,-2,3.14|870+6:o;900+6:o;930+40:o|880,905,940|RAC1_EQUIP=0xc"
  "swing758d|1200|758@800,0,10,-2,-1.57|870-1150:o|900,940,980,1020,1100|RAC1_EQUIP=0xc"
  "swing758e|1100|758@800,-10,0,-2,0|870+6:o;900+6:o;930+40:o|880,905,940|RAC1_EQUIP=0xc"
  "swing803|1100|803@800,0,-8,-6|870+6:o;900+6:o;930+40:o|880,905,940|RAC1_EQUIP=0xc"
  "helga|2600|890@800,2,0,0,3.14|830+4:t;900+4:t;1000+4:x;1100+4:x;1200+4:x;1300+4:x;1400+4:st;1500+4:x;1700+4:x|840,920,1100,1400,1800,2500|"
  "bigal|1500|909@800,2,0,0,3.14|830+4:t;900+4:x;1000+4:x;1100+4:x|840,920,1100,1400|"
  "qwark914|1500|914@800,2,0,0,3.14|830+4:t;900+4:x;1000+4:x;1100+4:x|840,920,1100,1400|"
  "infobot|1900|750@800,2,0,0|820+4:t;860-880:ly=-1;900+4:t;1000+4:x;1200+4:x;1400+4:x|840,900,1200,1600,1850|"
  "vendor|1500|11@800,2,0,0,3.14|830+4:t;900+4:r;930+4:x;1000+4:t|840,920,1100|"
  "goldbolt0|1000|-1@800,177.07,227.38,50.5|0:l3|810,900|"
  "goldbolt1|1000|-1@800,262.52,124.6,34.6|0:l3|810,900|"
  "goldbolt2|1000|-1@800,278.62,394.51,89.4|0:l3|810,900|"
  "taxi816|2200|816@800,0,0,3|900+4:t;960+4:t;1020+4:t;1100+4:t|900,1100,1400,1800,2100|"
  "taxi1012|2200|1012@800,0,0,4|900+4:t;960+4:t;1020+4:t;1100+4:t|900,1100,1400,1800,2100|"
  "train822|1600|822@800,0,0,3.0|0:l3|900,1100,1400|"
  "trainride|2600|-1@800,210.3,366.2,81.5|1800+4:st;1900+4:st|880,1100,1500,1700,1850,2500|"  # (task 0385b: on a carriage of the far station's train: the shot, the ride, the hero handed back ~1661, the pause menu)
  "spawner|1800|455@800,6,0,1,3.14|820-1700:ly=-0.3;900-1700:r2|900,1200,1500|RAC1_EQUIP=0xf"
  "robodogs|1600|573@800,5,0,1,3.14|820-1500:r2|900,1100,1400|RAC1_EQUIP=0xf"
  "minelayer|1500|578@800,5,0,1,3.14|820-1400:r2|900,1100|RAC1_EQUIP=0xf"
  "helicopter|1600|631@800,8,0,2,3.14|820-1500:r2|900,1100,1400|RAC1_EQUIP=0xf"
  "sandmouse|1500|1818@800,3,0,1,3.14|820-900:ly=-1;900+4:t|900,1100|"
  "class994|1100|-1@800,154.7,58.0,62.0,1.57|0:l3|850,1000|"
  "platforms|1500|868@800,0,0,2|820-1400:ly=-1;900+4:x;1000+4:x|900,1100,1300|"
  "lift726|1400|726@800,0,0,1|0:l3|900,1100,1300|"
  "scriptcam|1400|737@800,0,0,0|0:l3|820,900,1100|"
  "pause|1300|-1@800,264,141,50.5|900+4:st;960+4:d;990+4:d;1020+4:x;1080+4:o;1120+4:st|920,1000,1100|"
  "death|1500|-1@800,264,141,50.5|0:l3|900,1100,1400|RAC1_DIE_AT=900"
  "fall|1600|-1@800,172,154,58|0:l3|830,900,1400|"
)

run() {
  local name=$1 frames=$2 at=$3 seq=$4 shots=$5 extra=$6
  local d=$OUT/$name
  mkdir -p "$d"
  (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=3 RAC1_FRAMES="$frames" RAC1_GIVE_FRAME=780 RAC1_GIVE_ITEMS="$ITEMS" \
     RAC1_HERO_AT="$at" RAC1_SIM_INPUT="seq:$seq" RAC1_SEQ_TRACE=10 RAC1_SHOTS="$shots" $extra timeout 900 "$BIN/rac1-native" > run.log 2>&1)
  local rc=$?
  local states
  states=$(grep '^\[seq\]' "$d/run.log" | awk '$3 >= 790 {print $5}' | uniq | tr '\n' ' ' | cut -c1-120)
  local todo
  todo=$(grep TODO_PORT "$d/run.log" | grep -Ev "$HOST" | sort -u | sed 's/^\[TODO_PORT\] Unported call to //' | tr '\n' ';')
  echo "$name rc=$rc states: $states"
  [ -n "$todo" ] && echo "  TODO_PORT: $todo"
  grep '^\[seq\]' "$d/run.log" | tail -1 | sed 's/^/  last: /'
}

if [ "$1" = dumps ]; then  # oracle batch 138's inputs: the RAM at frame 20 of the landing (the ships' class-235 mobys alive), the ride's
  d=$OUT/dumps
  mkdir -p "$d" "$DUMPS/kerwan_0385"
  (cd "$d" && ln -sfn "$W/assets" assets && RAC1_HEADLESS=1 RAC1_LEVEL=3 RAC1_FRAMES=25 RAC1_RAMHASH=ramhash.bin \
     RAC1_RAMDUMP="20,$DUMPS/kerwan_0385/landing20.bin" timeout 900 "$BIN/rac1-native" > run.log 2>&1)
  # (task 0385b) and the train ride's, 855 frames after the hero boarded a carriage of the far station's train at frame 800 (the far
  # station's shot near its end)
  (cd "$d" && RAC1_HEADLESS=1 RAC1_LEVEL=3 RAC1_FRAMES=1660 RAC1_HERO_AT=-1@800,210.3,366.2,81.5 RAC1_RAMHASH=ramhash.bin \
     RAC1_RAMDUMP="1655,$DUMPS/kerwan_0385/train1655.bin" timeout 900 "$BIN/rac1-native" > run2.log 2>&1)
  ls -l "$DUMPS/kerwan_0385/landing20.bin" "$DUMPS/kerwan_0385/train1655.bin"
  exit
fi

want=("$@")
for s in "${SCEN[@]}"; do
  IFS='|' read -r name frames at seq shots extra <<< "$s"
  if [ ${#want[@]} -gt 0 ]; then
    hit=0; for x in "${want[@]}"; do [ "$x" = "$name" ] && hit=1; done
    [ $hit = 0 ] && continue
  fi
  run "$name" "$frames" "$at" "$seq" "$shots" "$extra"
done
