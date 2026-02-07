# Stage 2: DSP Implementation - Research Summary

**Date:** 2026-02-07
**Confidence:** HIGH (all JUCE APIs verified against 8.0.4 source; algorithms verified against published references)

---

## Research Scope

Three parallel investigations completed:
1. **Granular DSP Engine** — circular buffer, grain pool, PPQ sync, freeze, feedback, SmoothedValue
2. **Euclidean Rhythms + Scale Quantization** — Bjorklund algorithm, pitch ladder, lookup tables
3. **Codebase Patterns** — existing DSP in O-Bass, O-FreqPulse, O-Polystutter, O-Detune; shared modules

Detailed findings in companion files:
- `RESEARCH-granular-dsp.md` — full JUCE API analysis, code examples, pitfalls
- `RESEARCH-euclidean-scales.md` — algorithm verification, precomputed tables, thread safety

---

## Key Decisions Confirmed

| Decision | Choice | Evidence |
|----------|--------|----------|
| Circular buffer | Manual `AudioBuffer<float>` (NOT `juce::dsp::DelayLine`) | DelayLine's shared `delayFrac/delayInt` state breaks with 64 concurrent grain reads at different offsets. O-Polystutter uses same manual approach. |
| Hann window | Compute per-sample (`cos()`) | 64 voices * ~10 cycles = ~640 cycles/sample. LUT evicts cache for variable-length grains. KVR consensus confirms. |
| PPQ reading | Block-start `getPosition()` + per-sample linear interpolation | Verified in JUCE 8.0.4 `AudioPlayHead.h:578`. Proven working in O-FreqPulse. |
| Euclidean generator | `(i * pulses) % steps < pulses` one-liner | Produces canonical Bjorklund rotation (first step always a hit). Verified against Toussaint (2005) for E(3,8), E(5,8), E(4,12). |
| Scale quantization | Precomputed 12-element `constexpr` lookup tables | O(1) per grain. No searching. Tables verified for all 5 scales. |
| Thread safety (Euclidean) | `std::array<bool, 16>` + `std::atomic<int>` length | Matches CONTEXT.md. Torn read at worst = one cycle of partial pattern (musically imperceptible). |
| Feedback soft clip | `std::tanh(x * 3.0f) * 1.00497f * 0.95f` | Scale factor `1/tanh(3)` = 1.00497 (hardcode or compute once). Rational approx available if profiling shows hotspot. |
| Freeze crossfade | `(int)(sampleRate * 0.005)` samples (~220 @ 44.1kHz) | Linear crossfade sufficient. Active grains continue from delay buffer; only new grains read frozen. |
| SmoothedValue | Only for `dry_wet` and `feedback` | `reset()` in `prepareToPlay()` only (never in processBlock). `setTargetValue()` at block start. `getNextValue()` per sample. |
| DSP file organization | Header-only `.h` in `Source/dsp/` | Follows O-Bass pattern. 7 files. |

---

## Pre-Implementation Fixes Required

Stage 1 code has 2 mismatches with CONTEXT.md decisions:

1. **Rename `texture` → `spread`** — Parameter ID, display name, cached pointer, APVTS layout. Also rename the parameter group.
2. **Add `stutter_gate` parameter** — Bool, default false. Add `std::atomic<float>* stutterGateParam`. Total becomes 18 params.

These must be addressed in the first DSP task before building on them.

---

## Architecture Summary

### Component Dependencies (Build Order)

```
Layer 1: Core Grain Engine
  DelayBuffer.h          ← no deps
  GrainPool.h            ← reads from DelayBuffer + FreezeManager
  GrainScheduler.h       ← needs TempoTracker for sync mode
  processBlock integration

Layer 2: Beat Sync
  TempoTracker.h         ← no deps (reads AudioPlayHead)
  GrainScheduler sync mode ← needs TempoTracker

Layer 3: Pitch/Scale
  ScaleQuantizer.h       ← no deps (constexpr tables)
  Pitch ladder modes     ← part of ScaleQuantizer

Layer 4: Features
  EuclideanGenerator.h   ← no deps (pure function)
  FreezeManager.h        ← reads from DelayBuffer
  Spread, feedback, pan, reverse, dry/wet, repeats, stutter gate
```

### Memory Budget

| Component | Size @ 44.1kHz | Size @ 96kHz |
|-----------|---------------|--------------|
| DelayBuffer (2s stereo) | 689 KB | 1.5 MB |
| FreezeBuffer (2s stereo max) | 689 KB | 1.5 MB |
| GrainPool (64 voices) | 2.5 KB | 2.5 KB |
| Euclidean pattern | 16 B | 16 B |
| SmoothedValue x2 | 48 B | 48 B |
| **Total** | **~1.4 MB** | **~3.0 MB** |

All allocated in `prepareToPlay()`. Zero allocations in `processBlock()`.

### Per-Sample Processing Order

```
1. Read input (inL, inR)
2. Write to delay buffer (input + feedback from previous sample)
3. Check grain spawn (scheduler) → spawnGrain if triggered
4. Process all active grains → wetL, wetR
5. Compute feedback: softClip(wet * fbAmount)
6. Mix output: dry * (1 - mix) + wet * mix (SmoothedValue)
```

---

## Critical Pitfalls to Guard Against

| # | Pitfall | Guard |
|---|---------|-------|
| 1 | Lagrange wrap-around at buffer boundary | Modulo-wrap all 4 sample indices independently |
| 2 | PPQ backward jump on DAW loop | `if (newPpq < oldPpq - 0.01) return false` — skip trigger |
| 3 | Freeze click on engage | Let active grains finish from delay buffer; only new grains read frozen; linear crossfade ~5ms |
| 4 | Feedback runaway | Soft clip BEFORE writing back to delay buffer |
| 5 | SmoothedValue reset in processBlock | NEVER call `reset()` in processBlock — kills in-progress ramps. Only `setTargetValue()`. |
| 6 | Hann window phase direction | Phase = elapsed / total (not remaining / total). Phase 0→1 gives correct 0→peak→0 shape. |
| 7 | Negative modulo in pitch quantization | Use `((x % 12) + 12) % 12` for always-positive result |
| 8 | Pendulum double-hitting boundaries | Skip boundary note on reversal: `index = boundary - 1` |
| 9 | Ladder state stale after scale change | Reset `ladderIndex = 0` on scale or pitch_mode parameter change |
| 10 | Euclidean step counter overflow after steps change | Apply `euclideanStep %= newSteps` on parameter change |

---

## Reusable Codebase Patterns

| Pattern | Source | Usage |
|---------|--------|-------|
| PPQ per-sample interpolation | O-FreqPulse `PluginProcessor.cpp:484-520` | TempoTracker implementation |
| Manual circular buffer | O-Polystutter `DSP/RepeatLane.h:114-124` | DelayBuffer design reference |
| Header-only DSP structure | O-Bass `Source/DSP/*.h` | File organization model |
| SmoothedValue two-stage | O-FreqPulse v1.3.2 | If needed for any per-sample params beyond dry_wet/feedback |
| Standalone PPQ fallback | O-FreqPulse `lastPpqPosition` pattern | TempoTracker standalone mode |

No existing shared modules are reusable for granular synthesis. All 7 DSP components built from scratch.

---

## Quantization Lookup Tables (Verified)

```
Chromatic:  {0,1,2,3,4,5,6,7,8,9,10,11}
Major:      {0,0,2,2,4,5,5,7,7,9,9,11}
Minor:      {0,0,2,3,3,5,5,7,8,8,10,10}
Pentatonic: {0,0,2,2,4,4,7,7,7,9,9,9}
Whole Tone: {0,0,2,2,4,4,6,6,8,8,10,10}
```

Tie-breaking: round down (lower scale degree). All constexpr.

---

## Open Questions (Resolved)

| Question | Resolution |
|----------|-----------|
| pitchRandom at 0 in all modes? | Unity rate (1.0) — no pitch change. Correct behavior. |
| pitchRandom in ladder modes? | Acts as depth scaler: `finalSemitones = ladderSemitones * (pitchRandom / 100.0f)` |
| Ladder persistence across freeze? | Yes — persist. Only reset on mode/scale change. |
| Multi-octave ladder? | Single-octave for now. pitchRandom scaling covers range needs. |
| `std::tanh` constexpr? | Not constexpr in C++17. Hardcode `1.00497f` or `static const`. |

---

## Ready for Planning

All research questions resolved. No blocking unknowns. Proceed to plan phase.
