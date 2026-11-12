#!/usr/bin/env bash
# ==============================================================================
#   O-GranulatorInstrument — Windows WebView2 grep gate (PLAN-4 PD18 / D54)
#
#   No Windows machine is in the loop: this gate pins the source lines a
#   Windows build needs, and the CI validate run (build-and-release.yml,
#   validate_only) proves the compile. Checks:
#     CMakeLists.txt (code lines, not comments):
#       1  NEEDS_WEB_BROWSER TRUE
#       2  NEEDS_WEBVIEW2 TRUE
#       3  JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
#     Source/PluginEditor.cpp, between "#if JUCE_WINDOWS" and its #else / #endif:
#       4  .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
#       5  .withUserDataFolder (
#     Source/PluginEditor.cpp, anywhere:
#       6  .withResourceProvider
#       7  0 SafePointer init-captures ("= juce::Component::SafePointer" inside a
#          lambda capture list "[", the MSVC C3493 / C3615 trap)
#
#   usage: tests/windows-webview2-gate.sh [PLUGIN_ROOT]   (default: this plugin)
#   Exit: 0 = all present; 1 = a miss (each printed).
# ==============================================================================
set -uo pipefail

ROOT="${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
CM="${ROOT}/CMakeLists.txt"
ED="${ROOT}/Source/PluginEditor.cpp"
fail=0
miss() { echo "  MISS: $1"; fail=1; }
ok()   { echo "  ok:   $1"; }

code() { grep -v '^[[:space:]]*#' "$1"; }   # CMake: drop comment lines

[ -f "${CM}" ] && [ -f "${ED}" ] || { echo "windows-webview2-gate: ${ROOT} lacks CMakeLists.txt / Source/PluginEditor.cpp"; exit 1; }

code "${CM}" | grep -Eq '^[[:space:]]*NEEDS_WEB_BROWSER[[:space:]]+TRUE'                         && ok "NEEDS_WEB_BROWSER TRUE"                        || miss "NEEDS_WEB_BROWSER TRUE (CMakeLists.txt)"
code "${CM}" | grep -Eq '^[[:space:]]*NEEDS_WEBVIEW2[[:space:]]+TRUE'                            && ok "NEEDS_WEBVIEW2 TRUE"                           || miss "NEEDS_WEBVIEW2 TRUE (CMakeLists.txt)"
code "${CM}" | grep -Eq '^[[:space:]]*JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1'                && ok "JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1"   || miss "JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 (CMakeLists.txt)"

# The #if JUCE_WINDOWS block (first one; up to its #else or #endif).
WIN="$(awk '/^[[:space:]]*#[[:space:]]*if[[:space:]]+JUCE_WINDOWS/{f=1; next} f && /^[[:space:]]*#[[:space:]]*(else|endif)/{exit} f' "${ED}")"
printf '%s\n' "${WIN}" | grep -Fq '.withBackend (juce::WebBrowserComponent::Options::Backend::webview2)' \
    && ok "withBackend (webview2) inside #if JUCE_WINDOWS" || miss "withBackend (webview2) inside #if JUCE_WINDOWS (PluginEditor.cpp)"
printf '%s\n' "${WIN}" | grep -Fq '.withUserDataFolder (' \
    && ok "withUserDataFolder ( inside #if JUCE_WINDOWS" || miss "withUserDataFolder ( inside #if JUCE_WINDOWS (PluginEditor.cpp)"

grep -Fq '.withResourceProvider' "${ED}" && ok "withResourceProvider present" || miss "withResourceProvider (PluginEditor.cpp)"

INIT="$(grep -nE '\[[^]]*=[[:space:]]*juce::Component::SafePointer' "${ED}" | wc -l | tr -d ' ')"
[ "${INIT}" = "0" ] && ok "SafePointer init-captures: 0" || miss "SafePointer init-captures: ${INIT} (hoist to a local)"

[ ${fail} -eq 0 ] && echo "windows-webview2-gate: PASS" || echo "windows-webview2-gate: FAIL"
exit ${fail}
