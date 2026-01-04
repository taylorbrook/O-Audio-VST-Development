# Phase B Guard Enforcement

This document contains the guard logic that prevents generating implementation scaffolding (Phase B: files 3-7) before design is finalized (Phase A: files 1-2).

## Purpose

C++ boilerplate generation is expensive. If design changes after Phase B runs, all 5 implementation files must be regenerated. The two-phase approach saves time by deferring scaffolding until design is locked.

## Guard Verification Protocol

Before generating ANY Phase B file (Phases 6-10), execute this verification:

```bash
# Verify design finalization before Phase B
MOCKUP_DIR="plugins/${PLUGIN_NAME}/.ideas/mockups"
LATEST_VERSION=$(ls -1 "$MOCKUP_DIR"/v*-ui.yaml 2>/dev/null | \
                 sed 's/.*v\([0-9]*\)-.*/\1/' | sort -n | tail -1)

if [ -z "$LATEST_VERSION" ]; then
  echo "✗ ERROR: No mockup versions found in $MOCKUP_DIR"
  echo "Expected: v1-ui.yaml and v1-ui-test.html"
  exit 1
fi

# Verify both Phase A files exist
if [ ! -f "$MOCKUP_DIR/v${LATEST_VERSION}-ui.yaml" ]; then
  echo "✗ ERROR: v${LATEST_VERSION}-ui.yaml missing"
  exit 1
fi

if [ ! -f "$MOCKUP_DIR/v${LATEST_VERSION}-ui-test.html" ]; then
  echo "✗ ERROR: v${LATEST_VERSION}-ui-test.html missing"
  exit 1
fi

# Check finalization marker
if ! grep -q "finalized: true" "$MOCKUP_DIR/v${LATEST_VERSION}-ui.yaml"; then
  echo "✗ BLOCKED: Phase B requires finalized design"
  echo ""
  echo "Phase B (implementation scaffolding) cannot proceed without approval."
  echo "Current status: Design iteration (Phase A only)"
  echo ""
  echo "To proceed:"
  echo "1. Test the design in browser (v${LATEST_VERSION}-ui-test.html)"
  echo "2. Return to Phase 5.5 decision menu"
  echo "3. Select option 2 (Finalize) to approve design"
  echo ""
  exit 1
fi

# If we reach here, design is finalized - safe to proceed to Phase B
echo "✓ Design finalized - proceeding to Phase B (implementation files)"
```

## Finalization Marker

When user selects option 2 (Finalize) from Phase 5.5 menu, mark design as finalized in YAML:

```bash
# Append finalization metadata to YAML
cat >> "$MOCKUP_DIR/v${LATEST_VERSION}-ui.yaml" << EOF

# Finalization metadata
finalized: true
finalized_at: $(date -u +%Y-%m-%dT%H:%M:%SZ)
finalized_by_phase: 5.5
EOF
```

## Integration Points

Execute this guard before:
- Phase 6 (production HTML generation)
- Phase 7 (C++ boilerplate generation)
- Phase 8 (CMake snippet generation)
- Phase 9 (integration checklist generation)
- Phase 10 (parameter-spec.md creation)

## Anti-Pattern: Premature Scaffolding

❌ NEVER generate Phase B files (3-7) without user approval from Phase 5.5 menu

❌ NEVER skip the Phase 5.5 decision menu after Phase A completes

❌ NEVER assume design is ready for implementation without explicit "Finalize" choice

❌ NEVER proceed to Phases 6-10 if finalization marker is missing from YAML

✓ ALWAYS present Phase 5.5 decision menu after Phase A (files 1-2 generated)

✓ ALWAYS wait for explicit option 2 (Finalize) choice before Phase B

✓ ALWAYS verify gate criteria (WebView constraints + creative brief validation)

✓ ALWAYS check finalization marker in YAML before generating any Phase B file
