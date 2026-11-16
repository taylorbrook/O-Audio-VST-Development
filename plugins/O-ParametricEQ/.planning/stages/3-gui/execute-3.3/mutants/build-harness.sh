#!/bin/bash
# PLAN-3.3 T11: builds the render harness from the repo build's own compile and
# link commands, with Source/ swapped for SRC_DIR (a mutant's scratch copy).
# Writes nothing to build/ or the repo. (From research-3.3/probe/build.sh.)
#   SRC_DIR  scratch Source dir      OUT_DIR  objects and binary
set -euo pipefail
REPO=/Users/taylorbrook/Dev/VST-development
BUILD=$REPO/build
REPO_SRC=$REPO/plugins/O-ParametricEQ/Source
: "${SRC_DIR:?}" "${OUT_DIR:?}"
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
MAIN_SRC="$(grep -o -- '-c /[^ ]*/tests/render-harness/main.cpp$' <<<"$MAIN_CMD" | sed 's/^-c //')"
eval "$FM -I'$SRC_DIR' -c '$MAIN_SRC' -o '$OUT_DIR/main.o'" 2> "$OUT_DIR/build-main.log" || { cat "$OUT_DIR/build-main.log"; exit 1; }
for tu in PluginProcessor analysis/SpectrumAnalyzer analysis/DisplayModel; do
  o="$OUT_DIR/$(basename $tu).o"
  eval "$FP -c '$SRC_DIR/$tu.cpp' -o '$o'" 2> "$OUT_DIR/build-$(basename $tu).log" || { cat "$OUT_DIR/build-$(basename $tu).log"; exit 1; }
done
LINK="$(sed -e 's/^: && //' -e 's/ && :$//' \
  -e "s# $D/tests/render-harness/main.cpp.o # '$OUT_DIR/main.o' #" \
  -e "s# $D/Source/PluginProcessor.cpp.o # '$OUT_DIR/PluginProcessor.o' #" \
  -e "s# $D/Source/analysis/SpectrumAnalyzer.cpp.o # '$OUT_DIR/SpectrumAnalyzer.o' #" \
  -e "s# $D/Source/analysis/DisplayModel.cpp.o # '$OUT_DIR/DisplayModel.o' #" \
  -e "s# -o plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness # -o '$OUT_DIR/harness' #" <<<"$LINK_CMD")"
for x in main.o PluginProcessor.o SpectrumAnalyzer.o DisplayModel.o; do grep -q "$OUT_DIR/$x" <<<"$LINK" || { echo "link rewrite failed: $x"; exit 2; }; done
eval "$LINK"
echo "$OUT_DIR/harness"
