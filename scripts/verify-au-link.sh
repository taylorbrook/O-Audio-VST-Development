#!/bin/bash
# ==============================================================================
# verify-au-link.sh — AU link + load verification gate
#
# Usage:
#   ./scripts/verify-au-link.sh <PluginName>
#
# Examples:
#   ./scripts/verify-au-link.sh OLyrica
#   ./scripts/verify-au-link.sh O-Bells
#
# Requires:
#   - macOS (auval is macOS-only)
#   - Plugin already built and installed at:
#       ~/Library/Audio/Plug-Ins/Components/<PluginName>-dev.component
#       (or <PluginName>.component for non-dev builds)
#
# What it does:
#   1. Parses PLUGIN_CODE, PLUGIN_MANUFACTURER_CODE, and AU main type from
#      plugins/<PluginName>/CMakeLists.txt.
#   2. Resolves OUARICON_MANUFACTURER_CODE from the root CMakeLists.txt
#      (dev-suffix branch — matches the installed -dev .component).
#   3. Invokes `auval -v <type> <subtype> <manuf>`.
#   4. Exits 0 on auval success (AU loads, validates), non-zero otherwise.
#
# Reusable verbatim by every Phase 24 propagation plan (D-30, D-31).
# Plan 23-05 (note-expression module AU-link defect fix) is the inaugural
# consumer of this gate.
# ==============================================================================

set -e

# Color codes (match scripts/verify-backup.sh style)
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

if [[ $# -lt 1 ]]; then
    echo -e "${RED}Usage: $0 <PluginName>${NC}" >&2
    echo "Example: $0 OLyrica" >&2
    exit 2
fi

PLUGIN="$1"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLUGIN_CMAKE="$REPO_ROOT/plugins/$PLUGIN/CMakeLists.txt"
ROOT_CMAKE="$REPO_ROOT/CMakeLists.txt"

# 1. Preflight: macOS only
if [[ "$(uname)" != "Darwin" ]]; then
    echo -e "${YELLOW}[verify-au-link] auval is macOS-only — skipping (uname=$(uname))${NC}"
    exit 0
fi

# 2. Preflight: plugin CMakeLists exists
if [[ ! -f "$PLUGIN_CMAKE" ]]; then
    echo -e "${RED}[verify-au-link] Plugin CMakeLists not found: $PLUGIN_CMAKE${NC}" >&2
    exit 3
fi

# 3. Parse PLUGIN_CODE (literal, e.g. "OLyr")
PLUGIN_CODE=$(grep -E '^[[:space:]]*PLUGIN_CODE[[:space:]]+' "$PLUGIN_CMAKE" \
    | head -1 \
    | sed -E 's/^[[:space:]]*PLUGIN_CODE[[:space:]]+([A-Za-z0-9]+).*/\1/')

if [[ -z "$PLUGIN_CODE" || ${#PLUGIN_CODE} -ne 4 ]]; then
    echo -e "${RED}[verify-au-link] Could not parse 4-char PLUGIN_CODE from $PLUGIN_CMAKE${NC}" >&2
    echo -e "Got: '$PLUGIN_CODE'" >&2
    exit 4
fi

# 4. Parse PLUGIN_MANUFACTURER_CODE — may be a literal (4 chars) or a CMake variable reference
MANUF_RAW=$(grep -E '^[[:space:]]*PLUGIN_MANUFACTURER_CODE[[:space:]]+' "$PLUGIN_CMAKE" \
    | head -1 \
    | sed -E 's/^[[:space:]]*PLUGIN_MANUFACTURER_CODE[[:space:]]+([^[:space:]]+).*/\1/')

if [[ -z "$MANUF_RAW" ]]; then
    echo -e "${RED}[verify-au-link] Could not parse PLUGIN_MANUFACTURER_CODE from $PLUGIN_CMAKE${NC}" >&2
    exit 5
fi

# Resolve ${OUARICON_MANUFACTURER_CODE} from root CMakeLists.txt (dev-suffix branch).
# Pattern: lines like `set(OUARICON_MANUFACTURER_CODE OuDv)` (dev) and
# `set(OUARICON_MANUFACTURER_CODE OuAu)` (release). Dev .component is what's
# installed via the build-and-install scripts; prefer the dev value.
if [[ "$MANUF_RAW" == "\${OUARICON_MANUFACTURER_CODE}" || "$MANUF_RAW" == "\$(OUARICON_MANUFACTURER_CODE)" ]]; then
    if [[ ! -f "$ROOT_CMAKE" ]]; then
        echo -e "${RED}[verify-au-link] Root CMakeLists not found: $ROOT_CMAKE${NC}" >&2
        exit 6
    fi
    # Pick the LAST set(...) line — the dev branch follows the release branch in root CMakeLists.txt
    MANUF=$(grep -E '^[[:space:]]*set\(OUARICON_MANUFACTURER_CODE[[:space:]]+' "$ROOT_CMAKE" \
        | tail -1 \
        | sed -E 's/^[[:space:]]*set\(OUARICON_MANUFACTURER_CODE[[:space:]]+([A-Za-z0-9]+).*/\1/')
else
    MANUF="$MANUF_RAW"
fi

if [[ -z "$MANUF" || ${#MANUF} -ne 4 ]]; then
    echo -e "${RED}[verify-au-link] Could not resolve 4-char manufacturer code (raw='$MANUF_RAW' resolved='$MANUF')${NC}" >&2
    exit 7
fi

# 5. Determine AU type (4-char code).
#    - Explicit PLUGIN_AU_MAIN_TYPE wins (rare; not present in O-Lyrica).
#    - IS_SYNTH TRUE -> kAudioUnitType_MusicDevice -> aumu
#    - Otherwise default to kAudioUnitType_Effect -> aufx
AU_MAIN_TYPE_RAW=$(grep -E '^[[:space:]]*PLUGIN_AU_MAIN_TYPE[[:space:]]+' "$PLUGIN_CMAKE" \
    | head -1 \
    | sed -E 's/^[[:space:]]*PLUGIN_AU_MAIN_TYPE[[:space:]]+([A-Za-z0-9_]+).*/\1/')

if [[ -n "$AU_MAIN_TYPE_RAW" ]]; then
    case "$AU_MAIN_TYPE_RAW" in
        kAudioUnitType_MusicDevice)  AU_TYPE="aumu" ;;
        kAudioUnitType_Effect)       AU_TYPE="aufx" ;;
        kAudioUnitType_MusicEffect)  AU_TYPE="aumf" ;;
        kAudioUnitType_Generator)    AU_TYPE="augn" ;;
        kAudioUnitType_MIDIProcessor) AU_TYPE="aumi" ;;
        *) echo -e "${RED}[verify-au-link] Unknown PLUGIN_AU_MAIN_TYPE: $AU_MAIN_TYPE_RAW${NC}" >&2; exit 8 ;;
    esac
elif grep -qE '^[[:space:]]*IS_SYNTH[[:space:]]+TRUE' "$PLUGIN_CMAKE"; then
    AU_TYPE="aumu"
else
    AU_TYPE="aufx"
fi

# 6. Run auval. Default: -v aumu OLyr OuDv
echo -e "${YELLOW}[verify-au-link] Plugin:        $PLUGIN${NC}"
echo -e "${YELLOW}[verify-au-link] AU codes:      type=$AU_TYPE  subtype=$PLUGIN_CODE  manuf=$MANUF${NC}"
echo -e "${YELLOW}[verify-au-link] Running:       auval -v $AU_TYPE $PLUGIN_CODE $MANUF${NC}"

# Clear the AU registrar cache so a fresh load is forced (matches CLAUDE.md
# Plugin Cache Clearing protocol — verifies the on-disk artefact, not a cached
# image of an older one).
killall -9 AudioComponentRegistrar 2>/dev/null || true

# Invoke auval. We do NOT use `set -e` short-circuit here — capture exit code
# explicitly so we can emit a clear error.
set +e
auval -v "$AU_TYPE" "$PLUGIN_CODE" "$MANUF"
RC=$?
set -e

if [[ $RC -eq 0 ]]; then
    echo -e "${GREEN}[verify-au-link] PASS: auval accepted $PLUGIN ($AU_TYPE $PLUGIN_CODE $MANUF)${NC}"
    exit 0
else
    echo -e "${RED}[verify-au-link] FAIL: auval rejected $PLUGIN (exit code $RC)${NC}" >&2
    echo -e "${RED}[verify-au-link] Confirm the .component is installed:${NC}" >&2
    echo -e "  ls -l ~/Library/Audio/Plug-Ins/Components/${PLUGIN}*.component" >&2
    echo -e "${RED}[verify-au-link] Then re-run, or rebuild + install per CLAUDE.md.${NC}" >&2
    exit $RC
fi
