#!/bin/bash
# Builds the O-Polystutter scheduler harness twice: psh_new from the working-tree
# DSP, psh_old from a baseline DSP tree (default: git HEAD's Source/DSP, which is
# v1.16.0's — the last DSP commit is v1.14.2). Binaries and renders go to $OUT.
# usage: build.sh [baselineRev]   then: OUT=... python3 gate.py
set -e
H="$(cd "$(dirname "$0")" && pwd)"
R="$(cd "$H/../../../.." && pwd)"
OUT="${OUT:-${TMPDIR:-/tmp}/psh}"; mkdir -p "$OUT"
REV="${1:-HEAD}"
BASE="$OUT/old_src/$REV"
rm -rf "$BASE"; mkdir -p "$BASE"
git -C "$R" archive "$REV" plugins/O-Polystutter/Source/DSP | tar -x -C "$BASE" --strip-components 4
FL=(-std=c++17 -O2 -DNDEBUG -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1
    -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -I "$H/shim" -I "${JUCE_DIR:-$HOME/JUCE}/modules")
FW=(-framework Foundation -framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework IOKit -framework Security -framework AppKit)
# One translation unit per JUCE module (a single unity TU redefines the CF helpers)
MODS=()
for m in juce_core juce_audio_basics juce_audio_formats juce_dsp; do
    [ -f "$OUT/$m.o" ] || clang++ "${FL[@]}" -x objective-c++ -c "${JUCE_DIR:-$HOME/JUCE}/modules/$m/$m.mm" -o "$OUT/$m.o"
    MODS+=("$OUT/$m.o")
done
S="$R/plugins/O-Polystutter/Source/DSP"
clang++ "${FL[@]}" -DNEW_ENGINE -I "$S" "$H/main.cpp" "$S/RepeatLane.cpp" "$S/TapeDegrader.cpp" "$S/TriggerScheduler.cpp" "${MODS[@]}" "${FW[@]}" -o "$OUT/psh_new"
clang++ "${FL[@]}" -I "$BASE" "$H/main.cpp" "$BASE/RepeatLane.cpp" "$BASE/TapeDegrader.cpp" "$BASE/TriggerRouter.cpp" "${MODS[@]}" "${FW[@]}" -o "$OUT/psh_old"
echo "built $OUT/psh_new $OUT/psh_old (baseline $REV)"
