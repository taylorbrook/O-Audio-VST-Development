#!/bin/bash
set -e
J=/Users/taylorbrook/JUCE/modules
S=/Users/taylorbrook/Dev/VST-development/plugins/O-simpleWavetable/Source
D=$(cd "$(dirname "$0")"; pwd)
CXX="clang++ -std=c++17 -O3 -DNDEBUG -fno-objc-arc -I$J -I$S -include $D/cfg.h -w"
cd "$D/obj"
pids=()
for f in juce_core/juce_core.mm juce_core/juce_core_CompilationTime.cpp juce_events/juce_events.mm juce_data_structures/juce_data_structures.mm \
         juce_graphics/juce_graphics.mm juce_graphics/juce_graphics_Harfbuzz.cpp juce_gui_basics/juce_gui_basics.mm juce_gui_extra/juce_gui_extra.mm \
         juce_audio_basics/juce_audio_basics.mm juce_audio_devices/juce_audio_devices.mm juce_audio_formats/juce_audio_formats.mm \
         juce_audio_processors/juce_audio_processors.mm juce_audio_processors_headless/juce_audio_processors_headless.mm \
         juce_audio_processors_headless/juce_audio_processors_headless_ara.cpp juce_audio_utils/juce_audio_utils.mm juce_dsp/juce_dsp.mm; do
  o=$(echo "$f" | tr '/' '_').o
  case "$f" in *.mm) X="-x objective-c++";; *) X="-x objective-c++";; esac
  $CXX $X -c "$J/$f" -o "$o" > "$o.log" 2>&1 & pids+=($!)
done
clang -O3 -w -I$J -include $D/cfg.h -x c -c $J/juce_graphics/juce_graphics_Sheenbidi.c -o sheenbidi.o > sheen.log 2>&1 & pids+=($!)
for s in PluginProcessor BankFactory BuiltInBanks MipmapBuilder WavetableImporter CycleView; do
  $CXX -x objective-c++ -c "$S/$s.cpp" -o "src_$s.o" > "src_$s.log" 2>&1 & pids+=($!)
done
$CXX -x objective-c++ -c "$D/perf.cpp" -o perf.o > perf.log 2>&1 & pids+=($!)
fail=0; for p in "${pids[@]}"; do wait $p || fail=1; done
if [ $fail = 1 ]; then echo COMPILE FAIL; grep -l "error" *.log; exit 1; fi
clang++ -O3 *.o -o "$D/bin/perfprobe" -framework Foundation -framework Security -framework Accelerate -framework CoreAudio -framework AudioToolbox \
  -framework CoreMIDI -framework IOKit -framework AppKit -framework Carbon -framework QuartzCore -framework CoreImage -framework Metal -framework WebKit \
  -framework CoreText -framework UniformTypeIdentifiers -framework CoreVideo -framework DiscRecording -framework AudioUnit -framework CoreAudioKit -framework Cocoa
echo BUILD OK
