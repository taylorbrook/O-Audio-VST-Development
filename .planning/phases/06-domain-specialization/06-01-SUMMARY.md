---
phase: 06-domain-specialization
plan: 01
subsystem: dsp
tags: [real-time-safety, std-function, audio-thread, juce, dsp-critic, processBlock]

# Dependency graph
requires:
  - phase: 05-quality-gates
    provides: Critic template infrastructure, gate integration
  - phase: 04-verification-standards
    provides: DSP critic scoring structure (threshold 8/10 for realtime_safety)
provides:
  - Exhaustive real-time safety rules in DSP agent
  - std::function zero-tolerance detection
  - Lambda capture analysis guidance
  - MessageManager communication patterns
  - Detection regex for automated scanning
affects: [06-02-thread-safety, 06-03-music-theory, all-future-dsp-implementations]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Zero-tolerance enforcement for processBlock violations"
    - "Regex-based automated detection patterns"
    - "Decision matrix for audio->GUI communication"

key-files:
  created: []
  modified:
    - .claude/agents/dsp-agent.md
    - .claude/critics/critic-dsp.md

key-decisions:
  - "std::function rejected entirely in processBlock path (no SBO threshold exception)"
  - "Capture-less lambdas allowed, through std::function rejected"
  - "MessageManager::callAsync rejected from audio thread, atomic+timer suggested"
  - "Detection regex patterns added for automated violation scanning"

patterns-established:
  - "Real-time safety rules as formal section with 9 categories"
  - "Cross-reference between agent rules and critic checklist"
  - "Scoring updated for zero-tolerance: 4-5 for std::function, 1-3 for locks/MessageManager"

# Metrics
duration: 3min
completed: 2026-02-01
---

# Phase 6 Plan 1: Real-Time Safety Rules Summary

**Zero-tolerance real-time safety rules encoded in DSP agent and critic with std::function rejection, lambda capture analysis, and MessageManager communication patterns**

## Performance

- **Duration:** 3 min
- **Started:** 2026-02-01T06:35:51Z
- **Completed:** 2026-02-01T06:38:06Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added comprehensive `<realtime_safety_rules>` section to DSP agent with 9 violation categories
- Expanded DSP critic Real-Time Safety category with std::function, lambda capture, and MessageManager detection
- Created detection regex table for automated violation scanning
- Updated scoring criteria to reflect zero-tolerance approach
- Established cross-reference pattern between agent rules and critic checklist

## Task Commits

Each task was committed atomically:

1. **Task 1: Enhance DSP agent with exhaustive real-time safety rules** - `6df4ee2` (feat)
2. **Task 2: Enhance DSP critic with expanded violation checklist** - `a113cf3` (feat)

## Files Created/Modified

- `.claude/agents/dsp-agent.md` - Added 334-line `<realtime_safety_rules>` section with:
  - Memory allocation patterns (new, malloc, vector ops, strings)
  - std::function zero-tolerance policy with rationale
  - Lambda capture rules (allowed vs rejected patterns)
  - Locks and synchronization detection
  - System calls and I/O prohibitions
  - Exception handling restrictions
  - Unbounded operations detection
  - MessageManager communication patterns with decision matrix
  - Pre-allocation requirements
  - Enforcement summary table

- `.claude/critics/critic-dsp.md` - Expanded Real-Time Safety category (128 lines added) with:
  - std::function Analysis (Zero Tolerance) subsection
  - Lambda Capture Rules subsection
  - Audio Thread to GUI Communication subsection
  - Detection Regex Patterns table
  - Updated scoring criteria for zero-tolerance

## Decisions Made

1. **std::function rejected entirely** - No SBO threshold exception. Type erasure allocation risk is implementation-dependent and unpredictable. Safer to reject all std::function in processBlock path.

2. **Capture-less lambdas allowed** - `[]` lambdas compile to function pointers with no allocation. But even capture-less lambdas are rejected if passed through std::function.

3. **MessageManager::callAsync rejected from audio thread** - Atomic + Timer polling is the recommended pattern for audio->GUI communication. Lock-free FIFO for complex data. AsyncUpdater acceptable for infrequent updates only.

4. **Detection regex added** - Enables automated scanning in future tooling. Patterns cover allocation, std::function, lambdas with captures, locks, MessageManager, exceptions, file I/O.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - both files edited cleanly with no conflicts.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Real-time safety rules comprehensive and ready for enforcement
- Cross-reference established between dsp-agent.md and critic-dsp.md
- Ready for 06-02 (Thread Safety Patterns) which will add GUI agent enhancements
- Pattern of encoding domain rules in agent + critic pair is established for reuse

---
*Phase: 06-domain-specialization*
*Completed: 2026-02-01*
