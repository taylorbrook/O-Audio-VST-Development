#!/bin/bash
# Builds the 2.6 verify probe from the render-harness compile and link commands in build/, without
# writing to build/ or the repo.
#   SRC_DIR  Source dir to compile PluginProcessor.cpp and analysis/*.cpp from (default: the repo's Source)
#   OUT_DIR  objects and binary (default: this script's dir)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO=/Users/taylorbrook/Dev/VST-development
BUILD=$REPO/build
REPO_SRC=$REPO/plugins/O-ParametricEQ/Source
SRC_DIR="${SRC_DIR:-$REPO_SRC}"
OUT_DIR="${OUT_DIR:-$HERE}"
mkdir -p "$OUT_DIR"
cd "$BUILD"
CMDS="$(ninja -t commands O-ParametricEQ-render-harness)"
D=plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir
MAIN_CMD="$(grep -- '-c /.*/tests/render-harness/main.cpp$' <<<"$CMDS" | head -1)"
PP_CMD="$(grep -- "$D/Source/PluginProcessor.cpp.o -c " <<<"$CMDS" | head -1)"
LINK_CMD="$(grep '^: && .*-o plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts' <<<"$CMDS" | head -1)"
[ -n "$MAIN_CMD" ] && [ -n "$PP_CMD" ] && [ -n "$LINK_CMD" ] || { echo "could not extract commands"; exit 2; }
strip_tail() { sed -e 's/ -MD -MT .*$//' -e "s# -I$REPO_SRC # -I$SRC_DIR #g"; }
FM="$(strip_tail <<<"$MAIN_CMD")"; FP="$(strip_tail <<<"$PP_CMD")"
eval "$FM -I'$SRC_DIR' -I$HERE -c '$HERE/probe33.cpp' -o '$OUT_DIR/probe.o'" 2> "$OUT_DIR/build-probe.log" || { cat "$OUT_DIR/build-probe.log"; exit 1; }
for tu in PluginProcessor analysis/SpectrumAnalyzer analysis/DisplayModel; do
  o="$OUT_DIR/$(basename $tu).o"
  eval "$FP -c '$SRC_DIR/$tu.cpp' -o '$o'" 2> "$OUT_DIR/build-$(basename $tu).log" || { cat "$OUT_DIR/build-$(basename $tu).log"; exit 1; }
done
LINK="$(sed -e 's/^: && //' -e 's/ && :$//' \
  -e "s# $D/tests/render-harness/main.cpp.o # '$OUT_DIR/probe.o' #" \
  -e "s# $D/Source/PluginProcessor.cpp.o # '$OUT_DIR/PluginProcessor.o' #" \
  -e "s# $D/Source/analysis/SpectrumAnalyzer.cpp.o # '$OUT_DIR/SpectrumAnalyzer.o' #" \
  -e "s# $D/Source/analysis/DisplayModel.cpp.o # '$OUT_DIR/DisplayModel.o' #" \
  -e "s# -o plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness # -o '$OUT_DIR/probe33' #" <<<"$LINK_CMD")"
for x in probe.o PluginProcessor.o SpectrumAnalyzer.o DisplayModel.o; do grep -q "$OUT_DIR/$x" <<<"$LINK" || { echo "link rewrite failed: $x"; exit 2; }; done
eval "$LINK"
echo "$OUT_DIR/probe33"
