#!/bin/bash
# usage: umbris_tour.sh [scenario ...] | one <name> <frames> <hero_at> <seq> <shots> [extra env] : headless runs of Umbris (level 7), one per
# scenario (task 0391, after kerwan_tour.sh / rilgar_tour.sh).
#
# Each scenario starts the level (its landing, game mode 6, then the arrival's scene), gives Umbris's items (the Hydrodisplacer 0x16, the
# Swingshot 0xc, the Blaster 0xf, the Heli-Pack 0x2; ITEMS= overrides) and then puts the hero somewhere with RAC1_HERO_AT (the game's own
# placement, in game mode 0 only) and drives the pad with a RAC1_SIM_INPUT=seq: script (src/host/sim_script.h). It prints, per
# scenario: the exit code, the hero's states seen (RAC1_SEQ_TRACE), the game modes, the TODO_PORT lines other than the host's own stubs,
# and keeps the screenshots (RAC1_SHOTS) in $OUT/<scenario>/. With no argument every scenario runs; `one ...` runs an ad-hoc scenario.
# Headless (no window); needs $RAC1_BUILD/rac1-native (default build/). One run at a time.
W=$(cd "$(dirname "$0")/../.." && pwd)
DUMPS=${RAC1_ORACLE_DUMPS:-$W/assets/dumps/oracle}  # where the oracle reads them (docs/oracle.md)
BIN=${RAC1_BUILD:-build}
case "$BIN" in /*) ;; *) BIN=$W/$BIN ;; esac
OUT=${OUT:-$W/build/oracle_runs/umbris_tour}
ITEMS=${ITEMS:-0x16,0xc,0xf,0x2}
# the host's stubs every level logs (vsync, DMA kick 0x2a08c8 / Umbris's copy 0x2c7570 from the Vendor's screen, DMA handlers, display packet), the pause menu's VU moby draw 0x24ff08 and the
# ship's flight drawing on the loading screen 0x2900a0 (shared code, other tasks)
HOST=${HOST:-'0x122298|0x2a08c8|0x2c7570|0x1187b0|0x1187d0|0x200f18|0x24ff08|0x2900a0'}
# the first wrench / triangle / jump presses next to a moby of the class
# (the arrival's scene, game mode 2, ends at frame ~2640: the hero is placed at 2660)
NEAR="2680-3150:ly=-0.2;2700+4:t;2760+4:s;2820+4:s;2880+4:t;2940+4:s;3000+4:x"

# name | frames | RAC1_HERO_AT | seq script | shots | extra env
SCEN=(
  "c38|3250|38@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c258|3250|258@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c341|3250|341@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c436|3250|436@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c447|3250|447@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c460|3250|460@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c529|3250|529@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c605|3250|605@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c737|3250|737@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c758|3250|758@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c803|3250|803@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c805|3250|805@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c823|3250|823@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c832|3250|832@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c871|3250|871@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c886|3250|886@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c902|3250|902@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1013|3250|1013@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1018|3250|1018@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1041|3250|1041@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1059|3250|1059@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1063|3250|1063@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1069|3250|1069@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1080|3250|1080@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1104|3250|1104@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1106|3250|1106@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1110|3250|1110@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1112|3250|1112@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1113|3250|1113@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1126|3250|1126@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1128|3250|1128@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1129|3250|1129@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1133|3250|1133@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1134|3250|1134@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1135|3250|1135@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1142|3250|1142@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1474|3250|1474@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1552|3250|1552@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1650|3250|1650@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1663|3250|1663@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1686|3250|1686@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1789|3250|1789@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c11|3250|11@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c750|3250|750@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "c1564|3250|1564@2660,3,0,1,3.14|$NEAR|2700,2900,3240|"
  "walk|3400||2700-3350:ly=-1|2800,3100,3390|"
  "vendor|3700|11@2660,2,0,0,3.14|2700+4:t;2870+4:o;2970+4:t;3070+4:o;3170+4:t;3270+4:o;3370+4:t|2710,2900,3000,3200,3400,3690|"
  "goldbolt0|3000|-1@2660,279.8,286.1,52.6|0:l3|2700,2900|"
  "goldbolt1|3000|-1@2660,339.6,249.9,49.0|0:l3|2700,2900|"
  "hydro341|4200|341@2660,0,0,0.5,3.14|2700+4:o;2900-4100:o|2700,3000,3400,3800,4190|RAC1_EQUIP=0x16"
  "swamp|3400|-1@2660,290,330,68,1.57|2700-3350:ly=-1;2700-3350/60:x|2700,2900,3100,3390|"
  "qwark447|9000|447@2660,3,0,1,3.14|0:l3|2700,3500,4500,6000,7500,8100,8990|"
  "boss|11000|447@2660,3,0,1,3.14|8150-10900:o;8115-10900/90:x|8300,8700,9000,9500,10000,10990|RAC1_GIVE_FRAME=8110 RAC1_EQUIP=0xf RAC1_AUTOPILOT=8150-10900:class:1106 RAC1_SEQ_WATCH=1106 HEAL=8130:11000:20:4 REFILL=15:8130:11000:100"
  "pillar|14000|447@2660,3,0,1,3.14/1113@8800,0,0,6|8150-8740:o;8900-13900:o|8600,8800,8870,8970,9500,13990|RAC1_GIVE_FRAME=8110 RAC1_EQUIP=0xf RAC1_SEQ_WATCH=1106 RAC1_AUTOPILOT=8150-8740:class:1106 HEAL=8130:20000:20:4 REFILL=15:8130:20000:100"
  "bossdeath|11000|447@2660,3,0,1,3.14|8150-10900:o;8115-10900/90:x|8300,9500,10000,10990|RAC1_GIVE_FRAME=8110 RAC1_EQUIP=0xf RAC1_AUTOPILOT=8150-10900:class:1106"
  "lava|3400|1113@2660,3,0,1,3.14|0:l3|2700,2900,3390|"
  "death|3400|-1@2660,264.3,79,46.5|0:l3|2800,3000,3390|RAC1_DIE_AT=2800"
  "pause|3400|-1@2660,264.3,79,46.5|2800+4:st;2860+4:d;2890+4:d;2920+4:x;2980+4:o;3020+4:st|2820,2900,3000,3100|"
)

# a repeated press: the segment `<first>-<last>/<period>:<input>` becomes `<f>+4:<input>` every <period> frames (the pad script has no
# repetition of its own)
expand() {
  echo "$1" | tr ';' '\n' | awk -F'[-/:]' 'NF == 4 && $1 ~ /^[0-9]+$/ && $3 ~ /^[0-9]+$/ { for (f = $1; f < $2; f += $3) printf "%d+4:%s;", f, $4; next } { printf "%s;", $0 }'
}

# REFILL=<item (decimal)>:<first>:<last>:<period>: that weapon's ammo 0x13d428[item] set to 200 every <period> frames; HEAL=<first>:<last>:
# <period>:<n>: the hero's health 0x1415f8 set to n (RAC1_POKE8; testing switches for the boss fight, not the game's)
refill() {
  [ -n "$REFILL$HEAL" ] || return 0
  { [ -n "$REFILL" ] && echo "$REFILL" | awk -F: '{ a = 1299496 + 4 * $1; for (f = $2; f < $3; f += $4) printf "%d:0x%x=200\n", f, a }'
    [ -n "$HEAL" ] && echo "$HEAL" | awk -F: '{ for (f = $1; f < $2; f += $3) printf "%d:0x1415f8=%d\n", f, $4 }'; } | paste -sd, | sed 's/^/RAC1_POKE8=/'
}

run() {
  local name=$1 frames=$2 at=$3 seq=$4 shots=$5 extra=$6
  seq=$(expand "$seq")
  local HEAL=$HEAL REFILL=$REFILL kv  # (a scenario's HEAL= / REFILL=: the testing pokes, see refill())
  for kv in $extra; do case $kv in HEAL=*) HEAL=${kv#HEAL=} ;; REFILL=*) REFILL=${kv#REFILL=} ;; esac; done
  extra="$extra $(refill)"
  local d=$OUT/$name
  mkdir -p "$d"
  (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=7 RAC1_FRAMES="$frames" RAC1_GIVE_FRAME=780 RAC1_GIVE_ITEMS="$ITEMS" \
     RAC1_HERO_AT="$at" RAC1_SIM_INPUT="seq:$seq" RAC1_SEQ_TRACE=10 RAC1_SHOTS="$shots" $extra timeout 1800 "$BIN/rac1-native" > run.log 2>&1)
  local rc=$?
  local states modes
  states=$(grep '^\[seq\]' "$d/run.log" | awk '$3 >= 790 {print $5}' | uniq | tr '\n' ' ' | cut -c1-120)
  modes=$(grep '^\[seq\]' "$d/run.log" | awk '{print $11}' | uniq | tr '\n' ' ' | cut -c1-60)
  local todo
  todo=$(grep TODO_PORT "$d/run.log" | grep -Ev "$HOST" | sort -u | sed 's/^\[TODO_PORT\] Unported call to //' | tr '\n' ';')
  echo "$name rc=$rc states: $states modes: $modes"
  [ -n "$todo" ] && echo "  TODO_PORT: $todo"
  grep '^\[seq\]' "$d/run.log" | tail -1 | sed 's/^/  last: /'
}

if [ "$1" = dumps ]; then  # oracle batch 142's inputs: the RAM in Qwark's scene (frame 3000) and in the Snagglebeast's fight (the "boss"
  # scenario's line: 315 the landing (882 spawned), 800 the arrival's scene, 8140 its first shock wave, 8700 while it takes the Blaster's hits, 9000 in its invulnerable phase, the glow drawn; the
  # "pillar" line: the hero put on the pillar 1113 at its phase's end (state 11): 8870 its charge at the pillar (state 9), 8970 state 5)
  mkdir -p "$DUMPS/umbris_0391"
  for fr in 315 800 3000 8140 8655 8700 9000; do
    d=$OUT/dumps$fr
    mkdir -p "$d"
    (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=7 RAC1_FRAMES=$((fr + 5)) RAC1_GIVE_FRAME=8110 RAC1_GIVE_ITEMS="$ITEMS" RAC1_EQUIP=0xf \
       RAC1_HERO_AT="447@2660,3,0,1,3.14" RAC1_SIM_INPUT="seq:$(expand "8150-10400:o;8115-10400/90:x")" RAC1_AUTOPILOT="8150-10400:class:1106" \
       RAC1_RAMHASH=/dev/null RAC1_RAMDUMP="$fr,$DUMPS/umbris_0391/umbris$fr.bin" timeout 1800 "$BIN/rac1-native" > run.log 2>&1)
    ls -l "$DUMPS/umbris_0391/umbris$fr.bin"
  done
  HEAL=8130:20000:20:4 REFILL=15:8130:20000:100
  for fr in 8870 8970; do
    d=$OUT/dumps$fr
    mkdir -p "$d"
    (cd "$d" && ln -sfn "$W/assets" assets && env RAC1_HEADLESS=1 RAC1_LEVEL=7 RAC1_FRAMES=$((fr + 5)) RAC1_GIVE_FRAME=8110 RAC1_GIVE_ITEMS="$ITEMS" RAC1_EQUIP=0xf \
       RAC1_HERO_AT="447@2660,3,0,1,3.14/1113@8800,0,0,6" RAC1_SIM_INPUT="seq:8150-8740:o;8900-13900:o" RAC1_AUTOPILOT="8150-8740:class:1106" $(refill) \
       RAC1_RAMHASH=/dev/null RAC1_RAMDUMP="$fr,$DUMPS/umbris_0391/umbris$fr.bin" timeout 1800 "$BIN/rac1-native" > run.log 2>&1)
    ls -l "$DUMPS/umbris_0391/umbris$fr.bin"
  done
  exit
fi

if [ "$1" = one ]; then
  shift
  run "$@"
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
