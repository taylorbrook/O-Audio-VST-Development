#!/bin/bash
# Builds the alias-gate harness twice: tfa_new from the working tree, tfa_old from a
# baseline source tree (default: the v1.6.2 backup, linear reads only).
# usage: build.sh [baselineDspDir]   then: OUT=... python3 gate.py
set -e
H="$(cd "$(dirname "$0")" && pwd)"
SH="$H/../scheduler-harness"
R="$(cd "$H/../../../.." && pwd)"
OUT="${OUT:-${TMPDIR:-/tmp}/tfa}"; mkdir -p "$OUT"
BASE="${1:-$R/backups/O-TextureForge/v1.6.2/Source/dsp}"
FL=(-std=c++20 -O2 -DNDEBUG -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1
    -I "$SH/shim" -I "${JUCE_DIR:-$HOME/JUCE}/modules" -I "$R/build/_deps/nanoflann-src/include"
    -idirafter "$R/modules/synthesis/grain-engine/cpp")   # after -I: its GrainScheduler.h must not shadow ours
FW=(-framework Foundation -framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework IOKit -framework Security -framework AppKit)
[ -f "$OUT/juce_mods.o" ] || clang++ "${FL[@]}" -x objective-c++ -c "$SH/juce_mods.mm" -o "$OUT/juce_mods.o"
S="$R/plugins/O-TextureForge/Source/dsp"
clang++ "${FL[@]}" -DNEW_ENGINE -I "$S" "$H/alias.cpp" "$S/GrainScheduler.cpp" "$S/KDTreeSearch.cpp" "$OUT/juce_mods.o" "${FW[@]}" -o "$OUT/tfa_new"
clang++ "${FL[@]}" -I "$BASE" "$H/alias.cpp" "$BASE/GrainScheduler.cpp" "$BASE/KDTreeSearch.cpp" "$OUT/juce_mods.o" "${FW[@]}" -o "$OUT/tfa_old"
