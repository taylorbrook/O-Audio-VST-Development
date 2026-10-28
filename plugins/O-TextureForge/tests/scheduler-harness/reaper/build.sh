#!/bin/bash
# Builds the processor-level gates (no ASan: it hangs before main on macOS 26) against
# the working-tree processor, or SRC=<a backup's Source dir> with SUFFIX=_old for a control.
#   v1.6.1 (c): $OUT/reaper_stress new ; $OUT/reaper_stress old (must show poisoned blocks)
#   v1.6.2:     $OUT/prepare_stress <scratchDir> ; the v1.6.1 control must fail A or C
# usage: [OUT=..] [SRC=..] [SUFFIX=..] [TARGETS="reaper_stress prepare_stress"] build.sh
set -e
H="$(cd "$(dirname "$0")" && pwd)"
R="$(cd "$H/../../../../.." && pwd)"
OUT="${OUT:-${TMPDIR:-/tmp}/tfr}"; mkdir -p "$OUT"
S="${SRC:-$R/plugins/O-TextureForge/Source}"
FL=(-std=c++17 -O2 -g -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_WEB_BROWSER=0 -DJUCE_USE_CURL=0 -DJUCE_PLUGINHOST_VST3=0 -DJUCE_PLUGINHOST_AU=0
    -DJUCE_MODAL_LOOPS_PERMITTED=1
    -I "$H/shim" -I "${JUCE_DIR:-$HOME/JUCE}/modules" -I "$R/build/_deps/nanoflann-src/include" -I "$S" -I "$S/dsp"
    -idirafter "$R/modules/synthesis/grain-engine/cpp")   # v1.7.0 BandlimitedReader.h; after -I so it cannot shadow dsp/
FW=(-framework Foundation -framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox
    -framework IOKit -framework Security -framework AppKit -framework QuartzCore -framework Metal -framework WebKit
    -framework CoreImage -framework CoreText -framework Carbon -framework UniformTypeIdentifiers -framework AudioUnit -framework CoreAudioKit)
J="${JUCE_DIR:-$HOME/JUCE}/modules"
MODS=()
for m in juce_core juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra juce_audio_basics \
         juce_audio_formats juce_audio_processors_headless juce_audio_processors juce_dsp; do
    for src in "$J/$m/$m.mm" "$J/$m"/${m}_*.cpp "$J/$m"/${m}_*.mm "$J/$m"/${m}_*.c; do
        [ -f "$src" ] || continue
        case "$src" in *.cpp) [ -f "${src%.cpp}.mm" ] && continue;; esac
        o="$OUT/$(basename "$src").o"
        case "$src" in *.c) lang=c;; *) lang=objective-c++;; esac
        if [ "$lang" = c ]; then FLX=(-O2 -g -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -I "$J"); else FLX=("${FL[@]}"); fi
        [ -f "$o" ] || clang -x $lang "${FLX[@]}" -c "$src" -o "$o"
        MODS+=("$o")
    done
done
SRCS=("$S/PluginProcessor.cpp" "$S/dsp/GrainScheduler.cpp" "$S/dsp/KDTreeSearch.cpp" "$S/dsp/CorpusLoader.cpp"
      "$S/dsp/DescriptorExtractor.cpp" "$S/dsp/MFCCExtractor.cpp" "$S/dsp/PCAProjection.cpp")
for t in ${TARGETS:-reaper_stress prepare_stress}; do
    clang++ "${FL[@]}" "$H/$t.cpp" "${SRCS[@]}" "${MODS[@]}" "${FW[@]}" -o "$OUT/$t${SUFFIX}"
done
