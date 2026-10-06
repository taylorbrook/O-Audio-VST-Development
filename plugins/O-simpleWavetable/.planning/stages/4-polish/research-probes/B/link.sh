# usage: link.sh src.cpp out "objs..." [extra flags]
cd "$(dirname "$0")"; . ./flags.sh
src=$1; out=$2; objs=$3; shift 3
xcrun clang++ $CXXF "$@" -x objective-c++ -c "$src" -o "$out.o" && xcrun clang++ "$out.o" $objs $FW -o "$out"
