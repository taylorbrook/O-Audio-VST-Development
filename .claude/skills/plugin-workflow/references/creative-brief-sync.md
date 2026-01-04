# Creative Brief Sync from Mockup

## Purpose

Ensure creative brief reflects finalized mockup before implementation begins.

## When

BEFORE dispatching Stage 1 (foundation-shell-agent), IF mockup exists.

## Implementation

### 1. Check for finalized mockup

```bash
if [ -d "plugins/${PLUGIN_NAME}/.ideas/mockups" ] && [ -f "plugins/${PLUGIN_NAME}/.ideas/parameter-spec.md" ]; then
  # Mockup finalized, proceed to step 2
else
  # No mockup or not finalized, skip sync and proceed to Stage 1
fi
```

### 2. Verify brief is current

```bash
# Read .continue-here.md for brief_updated_from_mockup flag
if grep -q "brief_updated_from_mockup: true" plugins/${PLUGIN_NAME}/.continue-here.md; then
  # Skip sync (already done during finalization)
  # Proceed to Stage 1 dispatch
else
  # Present info message
  echo "Notice: Mockup finalized but brief not yet updated."
  echo "This should have happened during mockup finalization."
  echo "Updating now to ensure alignment..."
  # Proceed to step 3
fi
```

### 3. Update creative brief from mockup

```bash
# Find latest mockup version
LATEST_MOCKUP=$(find plugins/${PLUGIN_NAME}/.ideas/mockups -name "v*-ui.yaml" | sort -V | tail -n 1)
VERSION=$(basename "$LATEST_MOCKUP" | sed 's/v\([0-9]*\)-.*/\1/')

# Execute sync script
.claude/utils/sync-brief-from-mockup.sh "${PLUGIN_NAME}" "${VERSION}"

# Update state
echo "brief_updated_from_mockup: true" >> plugins/${PLUGIN_NAME}/.continue-here.md
echo "mockup_version_synced: ${VERSION}" >> plugins/${PLUGIN_NAME}/.continue-here.md
echo "brief_update_timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> plugins/${PLUGIN_NAME}/.continue-here.md

# Commit
git add plugins/${PLUGIN_NAME}/.ideas/creative-brief.md
git add plugins/${PLUGIN_NAME}/.continue-here.md
git commit -m "docs(${PLUGIN_NAME}): sync creative brief with finalized mockup v${VERSION}"
```

### 4. Confirm and continue

```
✓ Creative brief updated from mockup v${VERSION}

Contracts aligned. Proceeding to Stage 1 (Foundation)...
```

## Notes

- No user interaction required - automatic sync replaces validation gate
- Mockup is source of truth for UI decisions after finalization
- Design iteration (v1, v2, v3) is normal workflow, not drift
- Automatic update removes decision gates and streamlines flow
- Preserves conceptual content while syncing UI-specific sections
