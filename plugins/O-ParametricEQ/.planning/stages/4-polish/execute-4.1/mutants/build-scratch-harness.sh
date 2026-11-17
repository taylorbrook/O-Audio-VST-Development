#!/bin/bash
# PLAN-4.1 task 27 (harness half): builds the render harness from a SCRATCH
# tree (SRC_DIR = mutated Source, MAIN = mutated tests/render-harness/main.cpp)
# with the compile and link commands ninja uses, without writing to build/ or
# the repo. Shape of stages/3-gui/research-3.3/probe/build.sh.
#   SRC_DIR  scratch Source dir      MAIN  scratch main.cpp      OUT_DIR  objects + binary
set -euo pipefail
REPO=/Users/taylorbrook/Dev/VST-development
BUILD=$REPO/build
REPO_SRC=$REPO/plugins/O-ParametricEQ/Source
: "${SRC_DIR:?}" "${MAIN:?}" "${OUT_DIR:?}"
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
eval "$FM -I'$SRC_DIR' -c '$MAIN' -o '$OUT_DIR/main.o'" 2> "$OUT_DIR/build-main.log" || { tail -20 "$OUT_DIR/build-main.log"; exit 1; }
for tu in PluginProcessor analysis/SpectrumAnalyzer analysis/DisplayModel; do
  o="$OUT_DIR/$(basename $tu).o"
  eval "$FP -I'$SRC_DIR' -c '$SRC_DIR/$tu.cpp' -o '$o'" 2> "$OUT_DIR/build-$(basename $tu).log" || { tail -20 "$OUT_DIR/build-$(basename $tu).log"; exit 1; }
done
LINK="$(sed -e 's/^: && //' -e 's/ && :$//' \
  -e "s# $D/tests/render-harness/main.cpp.o # '$OUT_DIR/main.o' #" \
  -e "s# $D/Source/PluginProcessor.cpp.o # '$OUT_DIR/PluginProcessor.o' #" \
  -e "s# $D/Source/analysis/SpectrumAnalyzer.cpp.o # '$OUT_DIR/SpectrumAnalyzer.o' #" \
  -e "s# $D/Source/analysis/DisplayModel.cpp.o # '$OUT_DIR/DisplayModel.o' #" \
  -e "s# -o plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness # -o '$OUT_DIR/harness' #" <<<"$LINK_CMD")"
for x in main.o PluginProcessor.o SpectrumAnalyzer.o DisplayModel.o harness; do grep -q "$OUT_DIR/$x" <<<"$LINK" || { echo "link rewrite failed: $x"; exit 2; }; done
eval "$LINK"
echo "$OUT_DIR/harness"
