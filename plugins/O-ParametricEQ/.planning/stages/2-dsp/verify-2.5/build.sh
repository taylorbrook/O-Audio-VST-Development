#!/bin/bash
# Builds the 2.5 verify probe from the existing render-harness objects in build/ without touching build/ or the repo.
#   SRC_DIR  alternative Source dir (PluginProcessor.cpp + its headers); default: the repo's Source
#   OUT_DIR  where objects and the binary go; default: this script's dir
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO=/Users/taylorbrook/Dev/VST-development
BUILD=$REPO/build
REPO_SRC=$REPO/plugins/O-ParametricEQ/Source
SRC_DIR="${SRC_DIR:-$REPO_SRC}"
OUT_DIR="${OUT_DIR:-$HERE}"
mkdir -p "$OUT_DIR"
cd "$BUILD"
CMDS="$(ninja -t commands O-ParametricEQ-render-harness)"   # read-only: prints, builds nothing
MAIN_CMD="$(grep -- '-c /.*/tests/render-harness/main.cpp$' <<<"$CMDS" | head -1)"
PP_CMD="$(grep -- 'render-harness.dir/Source/PluginProcessor.cpp.o -c ' <<<"$CMDS" | head -1)"
LINK_CMD="$(grep '^: && .*-o plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts' <<<"$CMDS" | head -1)"
[ -n "$MAIN_CMD" ] && [ -n "$PP_CMD" ] && [ -n "$LINK_CMD" ] || { echo "could not extract commands"; exit 2; }
strip_tail() { sed -e 's/ -MD -MT .*$//' -e "s# -I$REPO_SRC # -I$SRC_DIR #"; }
FLAGS_MAIN="$(strip_tail <<<"$MAIN_CMD")"
FLAGS_PP="$(strip_tail <<<"$PP_CMD")"
echo "SRC_DIR=$SRC_DIR"
eval "$FLAGS_MAIN -c '$HERE/probe.cpp' -o '$OUT_DIR/probe.o'" 2> "$OUT_DIR/build-probe.log" || { cat "$OUT_DIR/build-probe.log"; exit 1; }
eval "$FLAGS_PP -c '$SRC_DIR/PluginProcessor.cpp' -o '$OUT_DIR/PluginProcessor.o'" 2> "$OUT_DIR/build-pp.log" || { cat "$OUT_DIR/build-pp.log"; exit 1; }
# link line: swap main.cpp.o and PluginProcessor.cpp.o for ours, output into OUT_DIR
LINK="$(sed -e 's/^: && //' -e 's/ && :$//' \
  -e "s# plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir/tests/render-harness/main.cpp.o # '$OUT_DIR/probe.o' #" \
  -e "s# plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir/Source/PluginProcessor.cpp.o # '$OUT_DIR/PluginProcessor.o' #" \
  -e "s# -o plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness # -o '$OUT_DIR/probe' #" <<<"$LINK_CMD")"
grep -q "$OUT_DIR/probe.o" <<<"$LINK" && grep -q "$OUT_DIR/PluginProcessor.o" <<<"$LINK" && grep -q -- "-o '$OUT_DIR/probe'" <<<"$LINK" || { echo "link rewrite failed"; exit 2; }
eval "$LINK"
echo "built $OUT_DIR/probe"
