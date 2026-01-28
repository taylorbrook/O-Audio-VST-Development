---
phase: 01-core-dsp-foundation
plan: 03
subsystem: dsp-mono-summing
tags: [dsp, stereo, mono, bass, phase-coherence]
depends_on:
  requires: [01-01]
  provides: [MonoSummer class, stereo-to-mono conversion, balance preservation]
  affects: [02-harmonic-generation]
tech_stack:
  added: []
  patterns: [buffer-pair processing, per-sample balance capture]
key_files:
  created:
    - plugins/OBass/Source/DSP/MonoSummer.h
    - plugins/OBass/Source/DSP/MonoSummer.cpp
  modified:
    - plugins/OBass/CMakeLists.txt
decisions: []
metrics:
  duration: 1m 25s
  completed: 2026-01-23
---

# Phase 01 Plan 03: Mono Summing Summary

**One-liner:** MonoSummer utility for (L+R)/2 bass conversion with optional original-balance stereo expansion

## What Was Built

### MonoSummer Class
- **Header:** `plugins/OBass/Source/DSP/MonoSummer.h`
- **Implementation:** `plugins/OBass/Source/DSP/MonoSummer.cpp` (98 lines)

### Key Methods
| Method | Purpose |
|--------|---------|
| `prepare(int maxBlockSize)` | Pre-allocate balance ratios buffer |
| `reset()` | Clear balance state |
| `sumToMono()` | Convert stereo to mono via (L+R)/2 |
| `captureBalance()` | Store per-sample L/(L+R) ratio |
| `expandToStereo()` | Expand mono back to stereo |

### Stereo Modes
| Mode | Behavior |
|------|----------|
| `StereoMode::Mono` | Output identical signal to L/R (default) |
| `StereoMode::MatchOriginal` | Restore captured L/R balance ratio |

## Technical Details

### Signal Flow
```
Stereo Input → captureBalance() → sumToMono() → [Mono Processing] → expandToStereo() → Stereo Output
```

### Memory Safety
- `balanceRatios` vector pre-allocated in `prepare()` to max block size
- No allocations in `sumToMono()` or `expandToStereo()` processing methods
- Only fallback resize in `captureBalance()` if block exceeds prepared size

### Balance Capture Algorithm
```cpp
// For each sample:
float balance = absLeft / (absLeft + absRight);  // 0.5 = centered
// Expansion:
left = mono * balance * 2.0f;
right = mono * (1.0f - balance) * 2.0f;
```

## Commits

| Hash | Type | Description |
|------|------|-------------|
| 5dacd5a | feat | Create MonoSummer header |
| c9d3537 | feat | Implement MonoSummer |
| e431d64 | chore | Add MonoSummer to build |

## Verification Results

- [x] MonoSummer.h and MonoSummer.cpp exist in DSP directory
- [x] StereoMode enum declared with Mono and MatchOriginal
- [x] sumToMono, expandToStereo, captureBalance methods implemented
- [x] No runtime allocations in processing methods
- [x] Plugin builds successfully

## Deviations from Plan

None - plan executed exactly as written.

## Next Phase Readiness

MonoSummer is ready for integration with:
- **Phase 2 (Harmonic Generation):** Mono bass signal from `sumToMono()` feeds into harmonic processor
- **Integration:** After harmonics added, use `expandToStereo()` to output with user-selected stereo mode

### Integration Pattern
```cpp
// In processBlock:
monoSummer.captureBalance(bassInput);  // If MatchOriginal mode
monoSummer.sumToMono(bassInput, monoBass);
// ... process monoBass with harmonics ...
monoSummer.expandToStereo(enhancedMono, bassOutput);
```
