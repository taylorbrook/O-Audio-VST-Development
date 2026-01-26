---
status: testing
phase: 07-oversampling-adaptive-harmonics
source: 07-01-SUMMARY.md, 07-02-SUMMARY.md, 07-03-SUMMARY.md
started: 2026-01-26T19:00:00Z
updated: 2026-01-26T19:00:00Z
---

## Current Test

number: 1
name: 4x Oversampling Active
expected: |
  In the Standalone app or DAW, the plugin should process audio without aliasing artifacts. Play bass content through OBass with Enhance at 75%+ and listen for clean harmonics without "digital grit" or harsh artifacts on transients.
awaiting: user response

## Tests

### 1. 4x Oversampling Active
expected: Play bass content through OBass with Enhance at 75%+ - harmonics should sound clean without aliasing artifacts or harsh "digital grit" on transients
result: [pending]

### 2. Adaptive Harmonics by Pitch
expected: Sub-bass content (30-40Hz) should produce fuller, richer harmonics than upper bass (100-150Hz). Compare a low synth bass note vs a higher bass note - the low one should have more harmonic content.
result: [pending]

### 3. DAW Latency Compensation
expected: In a DAW project with OBass inserted, phase alignment should be preserved. Record or compare a dry track vs processed track - they should stay in phase (no flamming on transients).
result: [pending]

### 4. No Dead Code Comments
expected: If you opened HarmonicGenerator.cpp, there should be no comments saying "no oversampling" or "bypassed" - all comments should accurately describe the active processing.
result: [pending]

## Summary

total: 4
passed: 0
issues: 0
pending: 4
skipped: 0

## Gaps

[none yet]
