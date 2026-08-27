#!/bin/sh
set -e
J=/Users/taylorbrook/JUCE/modules
SRC=/Users/taylorbrook/Dev/VST-development/plugins/O-Octagon/Source
D="-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1 -DJUCE_MODULE_AVAILABLE_juce_audio_formats=1 -DJUCE_MODULE_AVAILABLE_juce_events=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -DNDEBUG=1 -DOOCTAGON_INSTRUMENT=1"
F="-O2 -std=gnu++17 -arch arm64 -mmacosx-version-min=11.0 -Wno-deprecated-declarations"
I="-I$J -I$SRC"
clang++ $D $F $I -c $J/juce_core/juce_core.mm                   -o juce_core.o
clang++ $D $F $I -c $J/juce_audio_basics/juce_audio_basics.mm   -o juce_audio_basics.o
clang++ $D $F $I -c $J/juce_events/juce_events.mm -o juce_events.o
clang++ $D $F $I -c $J/juce_audio_formats/juce_audio_formats.mm -o juce_audio_formats.o
clang++ $D $F $I -c juce_glue.cpp -o juce_glue.o
clang++ $D $F $I -c $J/juce_dsp/juce_dsp.mm                     -o juce_dsp.o
clang++ $D $F $I -c $SRC/DSP/MonitorFold.cpp                    -o MonitorFold.o
clang++ $D $F $I -c main.cpp                                 -o main.o
clang++ -arch arm64 -mmacosx-version-min=11.0 \
  juce_core.o juce_events.o juce_audio_basics.o juce_audio_formats.o juce_dsp.o juce_glue.o MonitorFold.o main.o -o monitor-fold-test \
  -framework Foundation -framework Cocoa -framework IOKit -framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Security
echo "linked ok"
