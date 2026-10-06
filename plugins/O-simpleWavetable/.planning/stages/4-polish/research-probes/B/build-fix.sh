set -e
cd "$(dirname "$0")"; . ./flags.sh; DEFS2=$(cat defs2.txt)
FW2="$FW -framework WebKit -framework CoreAudioKit -framework DiscRecording -framework Carbon -framework CoreImage -framework Metal -framework MetalKit -framework CoreVideo -framework AVFoundation -framework UniformTypeIdentifiers"
build () {  # $1 = tag, $2 = src dir, $3 = extra
  mkdir -p obj-$1
  for f in PluginProcessor BankFactory BuiltInBanks MipmapBuilder WavetableImporter CycleView; do
    xcrun clang++ -std=gnu++17 -O2 -mmacosx-version-min=11.0 $DEFS2 -DOSIW_TEST_HOOKS=1 $3 -I$J -I$2 -x objective-c++ -c $2/$f.cpp -o obj-$1/$f.o &
  done
  xcrun clang++ -std=gnu++17 -O2 -mmacosx-version-min=11.0 $DEFS2 -DOSIW_TEST_HOOKS=1 $3 -I$J -I$2 -x objective-c++ -c fixprobe.cpp -o obj-$1/fixprobe.o &
  wait
  xcrun clang++ obj-$1/*.o obj2/*.o $FW2 -o fixprobe-$1
}
#build orig /Users/taylorbrook/Dev/VST-development/plugins/O-simpleWavetable/Source ""
#build fix src-fix "-DFIXBUILD=1"
echo BUILD-FIX-DONE
link () { xcrun clang++ obj-$1/*.o obj2/*.o $FW2 -o fixprobe-$1; }

