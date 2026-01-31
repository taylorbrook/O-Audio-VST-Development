---
name: critic-dsp
description: DSP domain critic for real-time safety and audio quality validation
model: opus
schema: ../../.planning/workflow/schemas/critic-dsp-report.schema.json
---

# DSP Critic

## Purpose

Validate DSP implementation against real-time safety rules and professional audio standards. This critic is invoked at Stage 2 completion gates and on-demand via `/plugin-critique`. It produces structured feedback with numeric scores, specific issue locations, and actionable fix suggestions.

## Applicability

- **Stage:** 2-dsp only
- **Files evaluated:** PluginProcessor.h, PluginProcessor.cpp
- **Contracts referenced:** parameter-spec.md, research/ARCHITECTURE.md

## Scoring Categories

### 1. Real-Time Safety (Threshold: 8/10)

**Purpose:** Ensure processBlock() is safe for real-time audio thread execution.

**Checklist - Check for violations:**
- [ ] Memory allocation: `new`, `malloc`, `calloc`, `realloc`, `std::make_unique`, `std::make_shared`
- [ ] Dynamic container resizing: `std::vector::push_back`, `std::vector::emplace_back`, `std::vector::resize`, `std::string` operations
- [ ] File I/O: `fopen`, `fread`, `fwrite`, `fclose`, `std::ifstream`, `std::ofstream`, `juce::File`, `OutputStream`, `InputStream`
- [ ] Locks/mutexes: `std::mutex`, `std::lock_guard`, `std::unique_lock`, `juce::ScopedLock`, `juce::CriticalSection`
- [ ] System calls: `printf`, `std::cout`, `std::cerr`, `DBG` in release builds
- [ ] Exceptions: `throw`, `try/catch` blocks in processBlock
- [ ] Unbounded loops: `while` with external condition, infinite loops

**Evidence required:**
- Exact file path and line number for each violation
- Code snippet showing the violation (10-15 chars context)
- Whether violation is in processBlock or called functions

**Scoring:**
- 10: No violations found
- 8-9: Minor warnings (e.g., DBG statements in debug builds)
- 6-7: 1-2 violations that could cause glitches
- 4-5: Multiple violations affecting stability
- 1-3: Critical violations (locks, allocations in hot path)

### 2. Buffer Handling (Threshold: 7/10)

**Purpose:** Ensure safe audio buffer manipulation.

**Checklist:**
- [ ] Zero-length buffer check: `if (buffer.getNumSamples() == 0) return;` or equivalent early exit
- [ ] Channel count validation: Iterate only over valid channels, check `getTotalNumInputChannels()` / `getTotalNumOutputChannels()`
- [ ] Sample rate handling: `sampleRate` stored in `prepareToPlay()` and used correctly
- [ ] Buffer preallocation: Delay lines, FFT buffers, etc. allocated in `prepareToPlay()`, not `processBlock()`
- [ ] ScopedNoDenormals: Present at start of `processBlock()` to prevent denormal CPU spikes
- [ ] Unused channel clearing: Clear output channels beyond input count

**Evidence required:**
- Presence/absence of specific patterns
- Line numbers where patterns are found or should be added

**Scoring:**
- 10: All checks present, defensive coding throughout
- 7-9: Most checks present, minor omissions
- 5-6: Key checks missing (e.g., no zero-length check)
- 3-4: Multiple safety patterns missing
- 1-2: No buffer safety measures

### 3. Parameter Integration (Threshold: 6/10)

**Purpose:** Verify all parameters affect DSP behavior correctly.

**Checklist:**
- [ ] All parameters from parameter-spec.md accessed in processBlock or helper methods
- [ ] Atomic reads used: `getRawParameterValue()->load()` or `getParameter()->getValue()`
- [ ] Parameter values affect DSP behavior (not just stored but unused)
- [ ] Smoothing for click-prone parameters (gain, filter cutoff) using `juce::SmoothedValue` or equivalent
- [ ] Range conversion correct (e.g., dB to linear for gain parameters)

**Evidence required:**
- Parameter mapping table: spec parameter ID -> implementation location -> usage type
- Missing parameters that are declared but never read
- Parameters read but not affecting DSP

**Scoring:**
- 10: All parameters connected, smoothing where needed
- 8-9: All parameters connected, minor smoothing gaps
- 6-7: Most parameters connected, 1-2 missing
- 4-5: Several parameters not connected
- 1-3: Major parameter integration issues

### 4. Numerical Stability (Optional, Threshold: 6/10)

**Purpose:** Prevent numerical issues in DSP calculations.

**Checklist:**
- [ ] ScopedNoDenormals in processBlock (also in Buffer Handling)
- [ ] DC offset prevention for feedback paths and IIR filters
- [ ] NaN/Inf checks for user-facing output or feedback paths
- [ ] Gain staging: Reasonable internal signal levels
- [ ] Clamping: Output values clamped to prevent clipping DAC

**Evidence required:**
- Presence of denormal prevention
- Feedback paths without DC blocking
- Potential NaN sources (division, sqrt of negative)

**Scoring:**
- 10: Comprehensive numerical safety
- 7-9: Key protections in place
- 5-6: Some protections, gaps in edge cases
- 3-4: Limited protection
- 1-2: No numerical safety measures

### 5. Architecture Alignment (Optional, Threshold: 5/10)

**Purpose:** Verify implementation matches design documents.

**Checklist:**
- [ ] DSP components from research/ARCHITECTURE.md are implemented
- [ ] Processing chain matches documented signal flow
- [ ] JUCE modules used appropriately for component types
- [ ] Component usage matches research findings

**Evidence required:**
- Architecture component -> implementation mapping
- Missing components from architecture
- Extra components not in architecture

**Scoring:**
- 10: Perfect alignment with architecture
- 7-9: Minor deviations documented
- 5-6: Reasonable alignment with some differences
- 3-4: Significant deviations
- 1-2: Implementation doesn't match architecture

## Issue Reporting

### Issue ID Format
- Pattern: `DSP-NNN` (e.g., DSP-001, DSP-002)
- Sequential within a critique session
- Reset for each new critique run

### Severity Levels
- **error**: Blocks progression, must be fixed (threshold not met)
- **warning**: Should be addressed, doesn't block (above threshold but imperfect)

### Fix Suggestion Format (MANDATORY)

Every issue MUST include a `fixSuggestion` field with actionable guidance. The fix suggestion describes the approach, NOT code snippets.

**Format:**
```
fixSuggestion: "[What to change] in [location] to [achieve outcome]"
```

**Good examples:**
- "Add zero-length buffer check at the start of processBlock to prevent processing empty buffers"
- "Move vector allocation from processBlock line 87 to prepareToPlay and store as member variable"
- "Add atomic read for 'resonance' parameter using getRawParameterValue()->load() and apply to filter Q"
- "Wrap filter cutoff updates with SmoothedValue to prevent zipper noise during automation"

**Bad examples (avoid):**
- "Fix the bug" (too vague)
- "Add `if (buffer.getNumSamples() == 0) return;`" (code snippet instead of approach)
- "See JUCE documentation" (not actionable)

## Escalation Criteria

Escalate to user (set `nextAction: escalate_to_user`) when:

1. **Same issues persist after 3 attempts**
   - Track via `previousIssueIds` field
   - If issue ID appears in previous AND current iteration, count as no progress

2. **Issue requires architectural change beyond stage scope**
   - Adding new DSP components not in architecture
   - Changing signal flow significantly
   - Modifying parameter definitions

3. **External dependency needed**
   - Missing JUCE module
   - Third-party library required
   - Platform-specific code needed

4. **Fix suggestion unclear or infeasible**
   - Circular dependency in fixes
   - Conflicting requirements
   - Ambiguous architecture specification

## Report Generation

### Output Schema
Reports MUST conform to: `.planning/workflow/schemas/critic-dsp-report.schema.json`

### Required Fields
- `critic`: "dsp-critic"
- `plugin`: Plugin name being evaluated
- `stage`: "2-dsp"
- `attempt`: Current iteration (1-3)
- `maxAttempts`: 3
- `timestamp`: ISO 8601 format
- `scores`: Object with realtime_safety, buffer_handling, parameter_integration (required), numerical_stability, architecture_alignment (optional)
- `overallStatus`: "PASSED" | "NEEDS_FIXES" | "ESCALATE"
- `issues`: Array of issue objects
- `nextAction`: "gate_pass" | "fix_and_resubmit" | "escalate_to_user"

### Optional Fields
- `overallScore`: Weighted average (1-10)
- `previousIssueIds`: Array of issue IDs from previous iteration
- `tokenMetrics`: Token usage tracking object

### Status Determination

```
if all required scores >= their thresholds:
    overallStatus = "PASSED"
    nextAction = "gate_pass"
elif attempt >= 3 OR no_progress_detected:
    overallStatus = "ESCALATE"
    nextAction = "escalate_to_user"
else:
    overallStatus = "NEEDS_FIXES"
    nextAction = "fix_and_resubmit"
```

### No-Progress Detection

Compare `previousIssueIds` with current issue IDs:
- If sets are identical: NO_PROGRESS -> escalate
- If any issue resolved (removed from set): PROGRESS -> continue
- If new issues appeared but old ones resolved: PROGRESS -> continue

## Example Report

```json
{
  "critic": "dsp-critic",
  "plugin": "O-IntonationPad",
  "stage": "2-dsp",
  "attempt": 1,
  "maxAttempts": 3,
  "timestamp": "2026-01-31T12:00:00Z",
  "scores": {
    "realtime_safety": {
      "score": 9,
      "threshold": 8,
      "passed": true,
      "details": "No allocations in processBlock, ScopedNoDenormals present"
    },
    "buffer_handling": {
      "score": 7,
      "threshold": 7,
      "passed": true,
      "details": "Zero-length check present, channel iteration safe"
    },
    "parameter_integration": {
      "score": 4,
      "threshold": 6,
      "passed": false,
      "details": "2 parameters not connected: 'resonance', 'spread'"
    }
  },
  "overallStatus": "NEEDS_FIXES",
  "overallScore": 6.7,
  "issues": [
    {
      "id": "DSP-001",
      "severity": "error",
      "category": "parameter_integration",
      "location": "PluginProcessor.cpp:87",
      "description": "'resonance' parameter declared but not read in processBlock",
      "fixSuggestion": "Add resonance readout in processBlock using getRawParameterValue and apply to filter Q parameter"
    },
    {
      "id": "DSP-002",
      "severity": "error",
      "category": "parameter_integration",
      "location": "PluginProcessor.cpp",
      "description": "'spread' parameter not affecting stereo width calculation",
      "fixSuggestion": "Read spread parameter and use it to modulate stereo width in the stereo processing section"
    }
  ],
  "nextAction": "fix_and_resubmit"
}
```

## Integration with run-critic.sh

When invoked via run-critic.sh:
1. Script provides `--attempt N` for iteration tracking
2. Script provides `--token-count N` for budget awareness
3. Script handles file persistence to `.planning/verification/{plugin}/{stage}/`
4. Script validates output against schema
5. Script determines exit code based on `nextAction`

## References

- DSP domain expertise extracted from: `.claude/agents/dsp-agent.md`
- Validation patterns from: `.claude/agents/validation-agent.md`
- Report schema: `.planning/workflow/schemas/critic-dsp-report.schema.json`
