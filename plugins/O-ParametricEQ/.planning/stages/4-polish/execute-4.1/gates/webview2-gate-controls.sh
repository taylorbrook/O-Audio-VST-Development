#!/bin/bash
# PLAN-4.1 task 20: negative controls for tests/windows-webview2-gate.sh. Each
# control breaks one pinned line in a scratch copy of CMakeLists.txt +
# Source/PluginEditor.cpp; the gate must FAIL on every one.
set -u
P=/Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ
GATE=$P/tests/windows-webview2-gate.sh
T=$(mktemp -d); trap 'rm -rf $T' EXIT
fired=0; n=0
run() {  # name, file (CMakeLists.txt | Source/PluginEditor.cpp), sed expression
  rm -rf $T/r; mkdir -p $T/r/Source; cp $P/CMakeLists.txt $T/r/; cp $P/Source/PluginEditor.cpp $T/r/Source/
  sed -i '' -e "$3" "$T/r/$2"
  if cmp -s "$T/r/$2" "$P/$2"; then echo "  C$n $1: MUTATION DID NOT APPLY"; return; fi
  n=$((n+1))
  if $GATE $T/r > $T/out 2>&1; then echo "  C$n $1: gate PASSED (control did not fire)"; else echo "  C$n $1: fired ($(grep MISS $T/out | head -1 | sed 's/^ *//'))"; fired=$((fired+1)); fi
}
run "NEEDS_WEB_BROWSER FALSE" CMakeLists.txt 's/NEEDS_WEB_BROWSER TRUE/NEEDS_WEB_BROWSER FALSE/'
run "NEEDS_WEBVIEW2 dropped"  CMakeLists.txt '/NEEDS_WEBVIEW2 TRUE/d'
run "static linking =0"       CMakeLists.txt 's/JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1/JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=0/'
run "withBackend dropped"     Source/PluginEditor.cpp '/\.withBackend (juce::WebBrowserComponent::Options::Backend::webview2)/d'
run "withUserDataFolder dropped" Source/PluginEditor.cpp 's/\.withUserDataFolder (webView2DataFolder)//'
run "SafePointer init-capture" Source/PluginEditor.cpp 's/\.withResourceProvider (\[this\]/.withResourceProvider ([safe = juce::Component::SafePointer<Editor> (this), this]/'
run "withResourceProvider dropped" Source/PluginEditor.cpp 's/\.withResourceProvider/.withXResourceProvider/'
echo "controls: $fired / $n fired"
[ $fired -eq $n ] && [ $n -eq 7 ]
