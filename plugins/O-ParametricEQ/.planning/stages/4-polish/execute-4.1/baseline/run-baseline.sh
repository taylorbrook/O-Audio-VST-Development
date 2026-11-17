#!/bin/bash
# Phase 4.1 W0: 0.9.0 baselines (verify-3.3 recipe). Writes next to this script.
set -u
S="$(cd "$(dirname "$0")" && pwd)"
REPO=/Users/taylorbrook/Dev/VST-development
P=$REPO/plugins/O-ParametricEQ
cd $REPO/build
ninja OuariconParametricEQ_VST3 OuariconParametricEQ_AU OuariconParametricEQ_Standalone O-ParametricEQ-render-harness O-ParametricEQ-designer-harness > $S/build.log 2>&1; echo "build rc $?" >> $S/build.log
R=$REPO/build/plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness
D=$REPO/build/$(find plugins/O-ParametricEQ -type f -perm +111 -name O-ParametricEQ-designer-harness | head -1)
cd $P
SD=$S/sessions-090
rm -rf $SD; mkdir -p $SD
$R --dump-sessions $SD > $S/dump.log 2>&1; echo "dump rc $?" >> $S/dump.log
(cd $SD && shasum -a 256 *.f32) > $S/sessions-090.sha
diff $S/sessions-090.sha $P/.planning/stages/3-gui/verify-3.3/sessions-090.sha > $S/sessions-diff.txt 2>&1; echo "diff rc $?" >> $S/sessions-diff.txt
$R --dump-params > $S/params.tsv 2> $S/params.err; echo "rc $?" >> $S/params.err
diff $S/params.tsv $P/tests/golden/params.tsv > $S/params-diff.txt 2>&1; echo "diff rc $?" >> $S/params-diff.txt
$D > $S/designer.log 2>&1; echo "rc $?" >> $S/designer.log
$R --lid-dir $SD > $S/render.log 2>&1; echo "rc $?" >> $S/render.log
OUT_DIR=$S/probe $P/.planning/stages/3-gui/research-3.3/probe/build.sh > $S/probe-build.log 2>&1; echo "probe build rc $?" >> $S/probe-build.log
$S/probe/probe33 n > $S/probe-n.txt 2>&1; echo "rc $?" >> $S/probe-n.txt
echo DONE > $S/done
