---
status: complete
phase: 23-extract
source:
  - 23-01-module-scaffolding-SUMMARY.md
  - 23-02-juce-patch-tooling-SUMMARY.md
  - 23-03-olyrica-consume-refactor-SUMMARY.md
  - 23-04-version-readme-dorico-smoketest-SUMMARY.md
  - 23-05-fix-au-link-steinberg-symbols-SUMMARY.md
started: 2026-04-26T00:55:00Z
updated: 2026-04-26T01:01:00Z
---

## Current Test

[testing complete]

## Tests

### 1. Module Discovery via /module-list
expected: Run `/module-list` (or open modules/registry.yaml). The `note-expression` module appears in the **tuning** category at version 1.0.0, with `used_by: [{plugin: OLyrica, version: 2.3.0}]`.
result: pass

### 2. JUCE Patch Tooling Idempotency
expected: Run `./scripts/apply-juce-patches.sh` twice. Both runs print a green "skipping" line and exit 0 — never re-applies, never errors. Then run `JUCE_DIR=/tmp/nope-not-here ./scripts/apply-juce-patches.sh` — fails with a red error pointing at the missing JUCE_DIR.
result: pass

### 3. CMake Marker Check Fires on O-Lyrica Configure
expected: From a clean state (`rm -rf build && mkdir build && cd build && cmake ..`), the configure log includes the line `[note-expression] JUCE-NE-PATCH markers verified in /Users/taylorbrook/JUCE` while configuring O-Lyrica, and configure exits 0.
result: pass

### 4. AU Verification Gate Runs Cleanly
expected: Run `./scripts/verify-au-link.sh O-Lyrica`. Script clears the AU cache, invokes `auval -v aumu OLyr OuDv`, the AU bundle loads, RENDER tests PASS at every sample rate, MIDI test PASS. (The pre-existing APVTS Meta-Flag warning on parameter ID 1275870432 is documented as deferred — not a regression.)
result: pass

### 5. O-Lyrica AU Loads in a Non-Dorico DAW
expected: Open a fresh AU host (Logic Pro / GarageBand / Ableton in AU mode). O-Lyrica-dev appears in the instrument list, instantiates without crashing, plays a plain MIDI note at 12-TET pitch (no Dorico microtonal NE events sent — the AU code path was previously unreachable; this confirms the new two-TU split runs correctly in a real host).
result: pass

### 6. Dorico Microtonal Regression (VST3)
expected: Open the existing Dorico project that exercises quarter-sharp C4. Playback still produces ~269 Hz (+50¢ above 261.6 Hz), no attack zipper, multi-note chord isolates the detuning to one voice, retrigger after release returns to 12-TET. No regression vs the Plan 23-05 re-pass.
result: pass

## Summary

total: 6
passed: 6
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

[none yet]
