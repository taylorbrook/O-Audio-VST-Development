#!/bin/bash
set -e

# ==============================================================================
# apply-juce-patches.sh
# Idempotent applier for Ouaricon JUCE local-fork patches.
#
# Behavior:
#   1. Fail loudly if JUCE_DIR (default /Users/taylorbrook/JUCE) is missing.
#   2. Skip application if the JUCE-NE-PATCH marker is already present.
#   3. Apply scripts/juce-patches/note-expression-juce-8.0.9.patch otherwise.
# ==============================================================================

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

JUCE_DIR="${JUCE_DIR:-/Users/taylorbrook/JUCE}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PATCH_DIR="$SCRIPT_DIR/juce-patches"
PATCH_FILE="$PATCH_DIR/note-expression-juce-8.0.9.patch"
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

# Step 3.5: normalize CRLF → LF on the two target files. The Windows JUCE zip
# distribution ships with CRLF, but the patch context lines are LF; without
# this, GNU patch rejects every hunk with "different line endings".
for f in "$HEADER_FILE" "$CPP_FILE"; do
  if [[ -f "$f" ]] && head -c 4096 "$f" | grep -q $'\r'; then
    echo -e "${YELLOW}[apply-juce-patches] Normalizing CRLF → LF in $(basename "$f")${NC}"
    # Portable sed-in-place: -i.bak works on both BSD/macOS sed and GNU sed.
    sed -i.bak 's/\r$//' "$f" && rm -f "${f}.bak"
  fi
done

# Step 4: apply (forward-only, self-verifying).
# --forward skips already-applied hunks instead of reverse-applying them, so a
# partial prior application is neither re-applied nor reversed. The script runs
# under `set -e`; on an already-applied hunk Apple patch prints "Ignoring
# previously applied (or reversed) patch." and exits 1, so guard with `|| true`
# and depend on the authoritative marker recount below, not patch's exit code.
echo -e "${YELLOW}[apply-juce-patches] Preflighting ${PATCH_FILE} (dry-run)...${NC}"
( cd "$JUCE_DIR" && patch -p1 --forward --dry-run < "$PATCH_FILE" ) || true

echo -e "${YELLOW}[apply-juce-patches] Applying ${PATCH_FILE} (--forward)...${NC}"
( cd "$JUCE_DIR" && patch -p1 --forward < "$PATCH_FILE" ) || true

# Step 5: authoritative post-apply verification — recount the marker in BOTH
# target files. Correctness depends on this recount, not on patch's exit code.
POST=0
for f in "$HEADER_FILE" "$CPP_FILE"; do
  if [[ -f "$f" ]] && grep -q "$MARKER" "$f"; then
    POST=$((POST + 1))
  fi
done

if [[ "$POST" -ge 2 ]]; then
  echo -e "${GREEN}[apply-juce-patches] Patch verified — ${MARKER} present in both target files.${NC}"
  echo -e "  grep -rn \"$MARKER\" $JUCE_DIR/modules/juce_audio_processors/utilities/ $JUCE_DIR/modules/juce_audio_plugin_client/"
else
  echo -e "${RED}[apply-juce-patches] Verification FAILED — ${MARKER} found in $POST/2 target files.${NC}"
  exit 1
fi
