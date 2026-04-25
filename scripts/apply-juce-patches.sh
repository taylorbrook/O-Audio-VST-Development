#!/bin/bash
set -e

# ==============================================================================
# apply-juce-patches.sh
# Idempotent applier for Ouaricon JUCE local-fork patches.
#
# Behavior:
#   1. Fail loudly if JUCE_DIR (default /Users/taylorbrook/JUCE) is missing.
#   2. Skip application if the JUCE-NE-PATCH marker is already present.
#   3. Apply scripts/juce-patches/note-expression-juce-8.0.4.patch otherwise.
# ==============================================================================

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

JUCE_DIR="${JUCE_DIR:-/Users/taylorbrook/JUCE}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PATCH_DIR="$SCRIPT_DIR/juce-patches"
PATCH_FILE="$PATCH_DIR/note-expression-juce-8.0.4.patch"
MARKER="JUCE-NE-PATCH"

# Step 1: preflight — JUCE tree must exist
if [[ ! -d "$JUCE_DIR" ]]; then
  echo -e "${RED}[apply-juce-patches] JUCE tree not found at ${JUCE_DIR}${NC}"
  echo -e "Set JUCE_DIR env var or install JUCE at /Users/taylorbrook/JUCE (see CLAUDE.md)."
  exit 1
fi

# Step 2: patch file must exist
if [[ ! -f "$PATCH_FILE" ]]; then
  echo -e "${RED}[apply-juce-patches] Patch file not found: ${PATCH_FILE}${NC}"
  exit 1
fi

# Step 3: idempotency — skip if marker already present in the two target files
HEADER_FILE="$JUCE_DIR/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h"
CPP_FILE="$JUCE_DIR/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp"
FOUND=0
for f in "$HEADER_FILE" "$CPP_FILE"; do
  if [[ -f "$f" ]] && grep -q "$MARKER" "$f"; then
    FOUND=$((FOUND + 1))
  fi
done

if [[ "$FOUND" -ge 2 ]]; then
  echo -e "${GREEN}[apply-juce-patches] Marker ${MARKER} already present in JUCE tree — skipping.${NC}"
  exit 0
fi

# Step 4: apply
echo -e "${YELLOW}[apply-juce-patches] Applying ${PATCH_FILE}...${NC}"
( cd "$JUCE_DIR" && patch -p1 < "$PATCH_FILE" )
echo -e "${GREEN}[apply-juce-patches] Patch applied. Verify with:${NC}"
echo -e "  grep -rn \"$MARKER\" $JUCE_DIR/modules/juce_audio_processors/utilities/ $JUCE_DIR/modules/juce_audio_plugin_client/"
