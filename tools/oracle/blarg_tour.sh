#!/bin/bash
# usage: blarg_tour.sh [scenario ...] | dumps : headless runs of Blarg's Nebula G34 (level 6), one per scenario (task 0390, after
# rilgar_tour.sh / kerwan_tour.sh).
#
# Each scenario starts the level (its landing, game mode 6, ends near frame 745), gives items (the Grindboots 0x1d, the Swingshot 0xc,
# the Blaster 0xf; ITEMS= overrides) and then puts the hero somewhere with RAC1_HERO_AT (the game's own placement) and drives the pad
# with a RAC1_SIM_INPUT=seq: script (src/host/sim_script.h). It prints, per scenario: the exit code, the hero's states seen
# (RAC1_SEQ_TRACE), the TODO_PORT lines other than the host's own stubs, and keeps the screenshots (RAC1_SHOTS) in $OUT/<scenario>/.
# With no argument every scenario runs. `dumps`: oracle batch 141's RAM inputs (assets/dumps/oracle/blarg_0390/).
# Headless (no window); needs $RAC1_BUILD/rac1-native (default build/). One run at a time.
# (task 0415) `clank`: the Clank section played (the Gadgebot command ring, the four commands); `clankdumps`: oracle batch 151's RAM
# (assets/dumps/oracle/clank_0415/).
W=$(cd "$(dirname "$0")/../.." && pwd)
DUMPS=${RAC1_ORACLE_DUMPS:-$W/assets/dumps/oracle}  # where the oracle reads them (docs/oracle.md)
OUT=${OUT:-$W/build/oracle_runs/blarg_tour}
ITEMS=${ITEMS:-0x1d,0xc,0xf}
# the host's stubs every level logs (vsync, DMA kick 0x2a08c8 / Blarg's own copy 0x2ad9c0 from the Vendor's screen draw, DMA handlers, display packet), the pause menu's VU moby draw 0x24ff08
# and the ship's flight drawing 0x2900a0 (shared code, other tasks)
HOST=${HOST:-'0x122298|0x2a08c8|0x2ad9c0|0x1187b0|0x1187d0|0x200f18|0x24ff08|0x2900a0'}

# the level's placed classes with an update function of their own (the development repository's docs/reversing/blarg_classes.md): the first moby of each visited,
# wrench swings and triangle presses beside it
CLASSES="55 87 367 604 605 615 750 803 805 827 832 857 911 1016 1021 1028 1035 1038 1039 1048 1051 1054 1061 1062 1066 1068 1083 1091
1093 1105 1108 1109 1118 1123 1302 1348 1504 1512 1551 1655 1660 1664 1818"
CSEQ="820-1250:ly=-0.2;850+4:t;900+4:s;950+4:s;1000+4:t;1050+4:s;1100+4:x"

# name | frames | RAC1_HERO_AT | seq script | shots | extra env
SCEN=(
  "walk|1500|-1@800,247.9,148.5,138.4,-1.57|820-1450:ly=-1|900,1200,1450|"
  "vendor|1500|11@800,2,0,0,3.14|830+4:t;900+4:r;930+4:x;1000+4:t|840,920,1100|"
  "vendor2|1600|-1@800,293.0,366.5,142.5,3.48|830+4:t;1000+4:o;1100+4:t;1200+4:o;1300+4:t;1400+4:o|900,1050,1150,1250,1350,1550|"
  "infobot|1600|750@800,2,0,0,3.14|830+4:t;900-1000:ly=-1|840,920,1100,1400|"
  "goldbolt0|1000|-1@800,173.07,183.75,133.9|0:l3|810,900|"
  "goldbolt1|1000|-1@800,258.74,372.99,146.7|0:l3|810,900|"
  "grind|2600|-1@800,175.7,309.5,143.5,-1.57|0:l3|850,1000,1300,1600,2000,2500|"
  "grindjump|2000|-1@800,177.6,293.0,145.0,-1.57|820+4:x|850,1000,1300,1600,1900|"
  "boss857|3000|857@800,8,0,1,3.14|820-2900:r2;820-2900:ly=-0.1|900,1200,1600,2000,2500,2950|RAC1_EQUIP=0xf"
  "c1051f|3000|1051@800,10,0,1,3.14|820-2900:r2|900,1200,1600,2000,2500,2950|RAC1_EQUIP=0xf"
  "c1118f|3000|1118@800,6,0,1,3.14|820-2900:r2|900,1200,1600,2000,2500,2950|RAC1_EQUIP=0xf"
  "c1123f|3000|-1@800,249.4,320.0,142.5|820-2900:r2|900,1200,1600,2000,2500,2950|RAC1_EQUIP=0xf"
  "c1109t|2500|1109@800,3,0,1,3.14|830+4:t;900+4:t;1000+4:x;1200+4:x;1400+4:x;1600+4:x|900,1100,1400,1800,2400|"
  "c1016t|2500|1016@800,2,0,1,3.14|830+4:t;900+4:t;1000+4:x;1200+4:x;1400+4:x;1600+4:x|900,1100,1400,1800,2400|"
  "c1118t|2500|1118@800,2,0,1,3.14|830+4:t;900+4:t;1000+4:x;1200+4:x;1400+4:x;1600+4:x|900,1100,1400,1800,2400|"
  "pause|1300|-1@800,247.9,148.5,138.4|900+4:st;960+4:d;990+4:d;1020+4:x;1080+4:o;1120+4:st|920,1000,1100|"
  "death|1500|-1@800,247.9,148.5,138.4|0:l3|900,1100,1400|RAC1_DIE_AT=900"
  "fall|1600|-1@800,247.9,170.0,138.4,1.57|820-1000:ly=-1;850+4:x|830,900,1000,1400|"
)
for c in $CLASSES; do SCEN+=("c$c|1300|$c@800,3,0,1,3.14|$CSEQ|900,1100,1290|"); done

run() {
  local name=$1 frames=$2 at=$3 seq=$4 shots=$5 extra=$6
  local d=$OUT/$name
  mkdir -p "$d"
  (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=6 RAC1_FRAMES="$frames" RAC1_GIVE_FRAME=780 RAC1_GIVE_ITEMS="$ITEMS" \
     RAC1_HERO_AT="$at" RAC1_SIM_INPUT="seq:$seq" RAC1_SEQ_TRACE=10 RAC1_SHOTS="$shots" $extra timeout 900 "$W/${RAC1_BUILD:-build}/rac1-native" > run.log 2>&1)
  local rc=$?
  local states
  states=$(grep '^\[seq\]' "$d/run.log" | awk '$3 >= 790 {print $5}' | uniq | tr '\n' ' ' | cut -c1-120)
  local todo
  todo=$(grep TODO_PORT "$d/run.log" | grep -Ev "$HOST" | sort -u | sed 's/^\[TODO_PORT\] Unported call to //' | tr '\n' ';')
  echo "$name rc=$rc states: $states"
  [ -n "$todo" ] && echo "  TODO_PORT: $todo"
  grep '^\[seq\]' "$d/run.log" | tail -1 | sed 's/^/  last: /'
}

# (task 0415) the Clank section: Ratchet put in the box of class 1061 (the hatch beside Clank's moby, class 87) and triangle: the hero
# is Clank (form 0x1413f4 = 1); Clank put by the Gadgebots' pods (class 302, at 163..168, 203..207) punches the four open; walks
# (RAC1_AUTOPILOT go:, camera-relative) by the crate on the way (punched) to the snake robot 1048 at the ring; the command ring
# (triangle held, the d-pad: up Wait, right Follow, down Attack, left Enter): Attack at 3000 (the Gadgebots destroy 1048), Follow at
# 3400, Enter at 3800 by the Gadgebot Entrance 1302 (the counter on the barrier 4 -> 1).
CLANK_AT="-1@800,220.9,150.3,137.0,1.57/-1@1000,169.5,203.4,132,3.14"
CLANK_SEQ="900+4:t;1050+4:s;1100+4:s;1150+4:s;1200+4:s;1250+4:s;1300+4:s;1350+4:s;1400+4:s;1440-1490:lx=0.79+ly=0.61;1495-1500:lx=1"
CLANK_SEQ="$CLANK_SEQ;1505+4:s;1535+4:s;1565+4:s;1595+4:s;1860+4:s;1890+4:s;1920+4:s;3000-3040:t;3010+3:d;3400-3440:t;3410+3:r;3800-3840:t;3810+3:l"
CLANK_AP="1650-1855:go:167.5,206.5/169.0,207.2/169.6,207.8|1950-2150:go:174,206.3|2200-2450:go:180,205.2|2500-2800:go:184,203|2850-2990:go:186,202"
CLANK_AP="$CLANK_AP|3450-3750:go:189,201.2/191,200.8"
clank_run() {  # <dir> <frames> <shots> [env...]
  local d=$1 fr=$2 shots=$3
  shift 3
  mkdir -p "$d"
  (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=6 RAC1_FRAMES="$fr" RAC1_HERO_AT="$CLANK_AT" RAC1_SIM_INPUT="seq:$CLANK_SEQ" \
     RAC1_AUTOPILOT="$CLANK_AP" RAC1_SEQ_TRACE=10 RAC1_SEQ_WATCH=857 RAC1_SHOTS="$shots" "$@" timeout 1800 "$W/${RAC1_BUILD:-build}/rac1-native" > run.log 2>&1)
}
if [ "$1" = clank ]; then
  d=$OUT/clank
  clank_run "$d" 4200 1000,1300,1600,2400,3020,3100,3300,3420,3820,4000,4190
  echo "clank rc=$?: the Gadgebots' states (857: moby state) and the form / command lines (seq-clank: form, command)"
  grep '^\[seq-watch\]   moby' "$d/run.log" | awk '{print $3, $5}' | sort | uniq -c
  grep '^\[seq-clank\]' "$d/run.log" | awk '{print $5, $7}' | sort | uniq -c
  grep TODO_PORT "$d/run.log" | grep -Ev "$HOST" | sort -u
  exit
fi
if [ "$1" = clankdumps ]; then  # oracle batch 151's inputs: the ring open (Attack chosen by the snake, Enter chosen by the entrance)
  mkdir -p "$DUMPS/clank_0415"
  for fr in 3015 3815; do
    clank_run "$OUT/clankdump_$fr" $((fr + 2)) "$fr" RAC1_RAMHASH=/dev/null RAC1_RAMDUMP="$fr,$DUMPS/clank_0415/ring_$fr.bin"
    ls -l "$DUMPS/clank_0415/ring_$fr.bin"
  done
  exit
fi

if [ "$1" = dumps ]; then  # oracle batch 141's inputs: the RAM at frame 900 (the hero at the landing: the boss 857's parts 302 / 303 alive)
  # and at frame 1100 of the c1051 scenario (the HUD element class 1051 requests, with Blarg's own draw function 0x249738)
  mkdir -p "$DUMPS/blarg_0390"
  for spec in "idle900|900|-1@800,247.9,148.5,138.4|0:l3" "c1051_1100|1100|1051@800,3,0,1,3.14|$CSEQ"; do
    IFS='|' read -r tag fr at seq <<< "$spec"
    d=$OUT/dumps_$tag
    mkdir -p "$d"
    (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=6 RAC1_FRAMES=$((fr + 5)) RAC1_GIVE_FRAME=780 RAC1_GIVE_ITEMS="$ITEMS" \
       RAC1_HERO_AT="$at" RAC1_SIM_INPUT="seq:$seq" RAC1_RAMHASH=/dev/null \
       RAC1_RAMDUMP="$fr,$DUMPS/blarg_0390/$tag.bin" timeout 1800 "$W/${RAC1_BUILD:-build}/rac1-native" > run.log 2>&1)
    ls -l "$DUMPS/blarg_0390/$tag.bin"
  done
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
