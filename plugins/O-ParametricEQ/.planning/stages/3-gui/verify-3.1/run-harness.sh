#!/bin/bash
set -u
S=/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/df84d4ad-9353-4fda-8d61-f12d47ce23bf/scratchpad/v31
cd /Users/taylorbrook/Dev/VST-development/build
# force recompile of the harness TUs (independent rebuild)
find plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-render-harness.dir plugins/O-ParametricEQ/CMakeFiles/O-ParametricEQ-designer-harness.dir -name '*.o' \( -path '*tests*' -o -path '*Source*' \) -delete
ninja O-ParametricEQ-render-harness O-ParametricEQ-designer-harness > $S/build.log 2>&1; echo "build rc $?" >> $S/build.log
R=plugins/O-ParametricEQ/O-ParametricEQ-render-harness_artefacts/Release/O-ParametricEQ-render-harness
D=$(find plugins/O-ParametricEQ -type f -perm +111 -name O-ParametricEQ-designer-harness | head -1)
cd /Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ
rm -rf $S/sessions-070; mkdir -p $S/sessions-070
$OLDPWD/$R --dump-sessions $S/sessions-070 > $S/dump.log 2>&1; echo "dump rc $?" >> $S/dump.log
(cd $S/sessions-070 && shasum -a 256 *.f32) > $S/sessions-070.sha
$OLDPWD/$D > $S/designer.log 2>&1; echo "rc $?" >> $S/designer.log
$OLDPWD/$R --lid-dir $S/sessions-070 > $S/render.log 2>&1; echo "rc $?" >> $S/render.log
$OLDPWD/$R --long --lid-dir $S/sessions-070 > $S/render-long.log 2>&1; echo "rc $?" >> $S/render-long.log
echo DONE > $S/done
