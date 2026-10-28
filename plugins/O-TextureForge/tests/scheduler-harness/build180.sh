#!/bin/bash
# v1.8.0: builds tfh_new (working tree) and tfh_old (default: the v1.7.0 backup),
# both with HAS_SEED so a seeded render is comparable bit for bit.
# usage: [OUT=..] build180.sh [baselineDspDir]   then: OUT=... python3 gate_v180.py
set -e
H="$(cd "$(dirname "$0")" && pwd)"
R="$(cd "$H/../../../.." && pwd)"
OUT="${OUT:-${TMPDIR:-/tmp}/tfh}"; mkdir -p "$OUT"
BASE="${1:-$R/backups/O-TextureForge/v1.7.0/Source/dsp}"
FL=(-std=c++20 -O2 -DNDEBUG -DHAS_SEED -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1
    -I "$H/shim" -I "${JUCE_DIR:-$HOME/JUCE}/modules" -I "$R/build/_deps/nanoflann-src/include"
    -idirafter "$R/modules/synthesis/grain-engine/cpp")
FW=(-framework Foundation -framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework IOKit -framework Security -framework AppKit)
[ -f "$OUT/juce_mods.o" ] || clang++ "${FL[@]}" -x objective-c++ -c "$H/juce_mods.mm" -o "$OUT/juce_mods.o"
S="$R/plugins/O-TextureForge/Source/dsp"
clang++ "${FL[@]}" -DNEW_ENGINE -I "$S" "$H/main.cpp" "$S/GrainScheduler.cpp" "$S/KDTreeSearch.cpp" "$OUT/juce_mods.o" "${FW[@]}" -o "$OUT/tfh_new"
clang++ "${FL[@]}" -I "$BASE" "$H/main.cpp" "$BASE/GrainScheduler.cpp" "$BASE/KDTreeSearch.cpp" "$OUT/juce_mods.o" "${FW[@]}" -o "$OUT/tfh_old"
