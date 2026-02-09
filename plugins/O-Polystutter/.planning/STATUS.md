## Continuation Context (migrated from .continue-here.md)

---
plugin: OuariconPolystutter
stage: complete
status: installed
version: 1.2.1
last_updated: 2026-01-17
issue: none
---

# OuariconPolystutter - Continue Here

## Current State
- **Version:** 1.2.0
- **Status:** DSP reverted to v1.1.4 stable baseline
- **Date:** 2026-01-17

## What Changed in v1.2.0

### Revert to Stable Beat-Sync

Reverted RepeatLane.cpp and RepeatLane.h to v1.1.4 tag to restore stable beat-sync stutter.

**Rationale:**
- Complex crossfade/retrigger logic added in v1.1.5-v1.1.19 attempted to fix ENV/Sidechain artifacts
- These fixes introduced regressions in normal beat-sync stutter mode
- v1.2.0 prioritizes stable, click-free beat-sync operation

**File changes:**
- RepeatLane.cpp: 745 lines -> 542 lines
- RepeatLane.h: reduced to match v1.1.4 state

### Known Issues

- ENV trigger mode: May have graininess/clicks
- Sidechain trigger mode: May have graininess/clicks
- Workaround: Use beat-sync, manual, or MIDI trigger modes

## To Test

1. Open DAW and load Ouaricon Polystutter
2. Enable beat-sync mode (default)
3. Play audio and trigger stutters
4. Verify clean, click-free stutter operation
5. Optional: Test ENV/Sidechain to confirm known artifacts

---
*Last updated: 2026-01-17*
