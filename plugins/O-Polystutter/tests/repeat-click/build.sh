#!/bin/bash
# Builds the O-Polystutter repeat-click probe twice: probe_new from the working-tree
# RepeatLane, probe_old from a baseline revision's (default 002c2497 = v1.18.2).
# usage: build.sh [baselineRev]   then: OUT=... python3 gate.py new|old
set -e
H="$(cd "$(dirname "$0")" && pwd)"
R="$(cd "$H/../../../.." && pwd)"
OUT="${OUT:-${TMPDIR:-/tmp}/prc}"; mkdir -p "$OUT"
REV="${1:-002c2497}"
BASE="$OUT/old_src/$REV"
rm -rf "$BASE"; mkdir -p "$BASE"
git -C "$R" archive "$REV" plugins/O-Polystutter/Source/DSP | tar -x -C "$BASE" --strip-components 4
FL=(-std=c++17 -O2 -DNDEBUG -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1
    -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -I "$H/../scheduler-harness/shim" -I "${JUCE_DIR:-$HOME/JUCE}/modules")
FW=(-framework Foundation -framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework IOKit -framework Security -framework AppKit)
MODS=()
for m in juce_core juce_audio_basics juce_audio_formats juce_dsp; do
    [ -f "$OUT/$m.o" ] || clang++ "${FL[@]}" -x objective-c++ -c "${JUCE_DIR:-$HOME/JUCE}/modules/$m/$m.mm" -o "$OUT/$m.o"
    MODS+=("$OUT/$m.o")
done
S="$R/plugins/O-Polystutter/Source/DSP"
GE="$R/modules/synthesis/grain-engine/cpp"   # v1.19.0: BandlimitedReader.h
clang++ "${FL[@]}" -I "$S" -I "$GE" "$H/probe.cpp" "$S/RepeatLane.cpp" "${MODS[@]}" "${FW[@]}" -o "$OUT/probe_new"
clang++ "${FL[@]}" -I "$BASE" "$H/probe.cpp" "$BASE/RepeatLane.cpp" "${MODS[@]}" "${FW[@]}" -o "$OUT/probe_old"
echo "built $OUT/probe_new $OUT/probe_old (baseline $REV)"
