#!/bin/bash
# Build the smoke harness against the O-Strata-param-dump objects already in build/.
set -euo pipefail
ROOT=/Users/taylorbrook/Dev/VST-development
B=$ROOT/build
S=$(cd "$(dirname "$0")" && pwd)
cd "$B"

RULE_LINE=$(grep -n "^build plugins/O-Strata/O-Strata-param-dump_artefacts/Release/O-Strata-param-dump:" build.ninja | cut -d: -f1)
OBJS=$(sed -n "${RULE_LINE}p" build.ninja | tr ' ' '\n' | grep '\.o$' | grep -v 'param-dump/main.cpp.o' | tr '\n' ' ')
LIBS=$(sed -n "$((RULE_LINE+1)),$((RULE_LINE+9))p" build.ninja | grep 'LINK_LIBRARIES' | sed 's/^ *LINK_LIBRARIES = //')

MAIN_LINE=$(grep -n "^build plugins/O-Strata/CMakeFiles/O-Strata-param-dump.dir/.*param-dump/main.cpp.o:" build.ninja | cut -d: -f1)
DEFS=$(sed -n "$((MAIN_LINE+1)),$((MAIN_LINE+7))p" build.ninja | grep ' DEFINES = ' | sed 's/^ *DEFINES = //')
INCS=$(sed -n "$((MAIN_LINE+1)),$((MAIN_LINE+7))p" build.ninja | grep ' INCLUDES = ' | sed 's/^ *INCLUDES = //')

echo "objects: $(echo $OBJS | wc -w)"
eval clang++ -O2 -std=gnu++17 -arch arm64 -mmacosx-version-min=11.0 -Wno-everything \
    $DEFS $INCS -c "$S/main.cpp" -o "$S/main.o"
eval clang++ -O2 -arch arm64 -mmacosx-version-min=11.0 -Wl,-search_paths_first -Wl,-headerpad_max_install_names \
    "$S/main.o" $OBJS $LIBS -o "$S/strata-smoke"
echo "built $S/strata-smoke"
