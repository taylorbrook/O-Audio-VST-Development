---
phase: 04-controls-refinement
plan: 02
subsystem: dsp
tags: [juce, gain, softclip, smoothedvalue, apvts]

# Dependency graph
requires:
  - phase: 04-01
    provides: intensity tuning and frequency-dependent scaling
provides:
  - Output parameter for gain control (-18dB to +18dB)
  - SmoothedValue gain transitions (click-free)
  - Tanh soft clipper at 0.95 (defense-in-depth)
  - Limit indicator for UI feedback
affects: [05-ui-polish, 06-testing]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Multiplicative SmoothedValue for dB gain"
    - "Defense-in-depth soft clipping (processor + output)"
    - "Atomic limit indicator for thread-safe UI feedback"

key-files:
  created: []
  modified:
    - "plugins/OBass/Source/PluginProcessor.h"
    - "plugins/OBass/Source/PluginProcessor.cpp"

key-decisions:
  - "Output range -18dB to +18dB with 0.1dB resolution"
  - "Multiplicative SmoothedValue for perceptually linear dB transitions"
  - "Soft clip threshold 0.95 (~-0.5dB) distinct from processor limit (~-2dB)"
  - "20ms ramp time matches other SmoothedValue settings"

patterns-established:
  - "Defense-in-depth limiting: processor limits enhancement, output limits final gain"
  - "Limit indicator pattern: atomic float + smoothed value for UI display"

# Metrics
duration: 4min
completed: 2026-01-25
---

# Phase 4 Plan 02: Output Gain Control Summary

**Output parameter (-18dB to +18dB) with 20ms smoothed transitions and tanh soft clipper at 0.95 for click-free gain control and digital clipping prevention**

## Performance

- **Duration:** 4 min
- **Started:** 2026-01-25T10:00:00Z
- **Completed:** 2026-01-25T10:04:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Output parameter in APVTS with -18dB to +18dB range, default 0dB
- SmoothedValue with Multiplicative type for perceptually linear dB transitions
- Tanh soft clipper at 0.95 threshold (defense-in-depth, distinct from processor limiting)
- Limit indicator (atomic float + SmoothedValue) for UI feedback
- Full build with both VST3 and AU formats passing AU validation

## Task Commits

Each task was committed atomically (both tasks in single commit due to tight coupling):

1. **Task 1-2: Output parameter and smoothed gain with soft clipping** - `cec0c52` (feat)

## Files Created/Modified
- `plugins/OBass/Source/PluginProcessor.h` - Added outputGainSmooth, limitIndicator, limitIndicatorSmooth members and getLimitIndicator() accessor
- `plugins/OBass/Source/PluginProcessor.cpp` - Added Output parameter to APVTS, initialization in prepareToPlay, output gain processing with soft clipping in processBlock

## Decisions Made
- **Output range:** -18dB to +18dB provides ample headroom adjustment in both directions
- **Multiplicative smoothing:** Ensures perceptually linear transitions when changing gain in dB
- **Soft clip threshold 0.95:** Distinct from processor limiting (~-2dB) - this catches final gain stage peaks only
- **Limit indicator:** Added for potential UI feedback showing when output soft clipping is active

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Completed incomplete limitIndicator implementation**
- **Found during:** Task 2 (build verification)
- **Issue:** Header had limitIndicator and limitIndicatorSmooth declared but uninitialized - causing build failure
- **Fix:** Added initialization in prepareToPlay() and proper update in processBlock()
- **Files modified:** plugins/OBass/Source/PluginProcessor.cpp
- **Verification:** Build succeeds, AU validation passes
- **Committed in:** cec0c52 (part of task commit)

---

**Total deviations:** 1 auto-fixed (blocking)
**Impact on plan:** Pre-existing incomplete code was completed as part of this plan. No scope creep.

## Issues Encountered
- Pre-existing uncommitted changes in header included incomplete limitIndicator implementation - resolved by completing the implementation

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All 4 core parameters now implemented: Crossover, Enhance, Mode, Output
- Ready for Phase 5 (UI polish) or Phase 6 (testing)
- Limit indicator ready for UI visualization when UI is implemented

---
*Phase: 04-controls-refinement*
*Completed: 2026-01-25*
