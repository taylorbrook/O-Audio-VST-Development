#!/bin/bash
set -u
S=/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/3a5be7e5-d0b5-4f84-abfe-33acf7dd6d92/scratchpad/v41
cd /Users/taylorbrook/Dev/VST-development/build
find plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-designer-harness.dir -name '*.o' \( -path '*tests*' -o -path '*Source*' \) -delete
ninja O-ParametricEQ-render-harness O-ParametricEQ-designer-harness > $S/build.log 2>&1; echo "build rc $?" >> $S/build.log
R=plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness
D=$(find plugins/O-ParametricEQ -type f -perm +111 -name O-ParametricEQ-designer-harness | head -1)
cd /Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ
rm -rf $S/sessions-0100; mkdir -p $S/sessions-0100
$OLDPWD/$R --dump-sessions $S/sessions-0100 > $S/dump.log 2>&1; echo "dump rc $?" >> $S/dump.log
(cd $S/sessions-0100 && shasum -a 256 *.f32) > $S/sessions-0100.sha
$OLDPWD/$D > $S/designer.log 2>&1; echo "rc $?" >> $S/designer.log
$OLDPWD/$R --lid-dir $S/sessions-0100 > $S/render.log 2>&1; echo "rc $?" >> $S/render.log
$OLDPWD/$R --dump-params > $S/params.tsv 2>$S/params.err; echo "rc $?" >> $S/params.err
echo DONE > $S/done
OUT_DIR=$S/probe /Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ/.planning/stages/3-gui/research-3.3/probe/build.sh > $S/probe-build.log 2>&1; echo "probe build rc $?" >> $S/probe-build.log
$S/probe/probe33 n > $S/probe-n.txt 2>&1; echo "rc $?" >> $S/probe-n.txt
$OLDPWD/$R --long --lid-dir $S/sessions-0100 > $S/render-long.log 2>&1; echo "rc $?" >> $S/render-long.log
echo DONE2 > $S/done2
