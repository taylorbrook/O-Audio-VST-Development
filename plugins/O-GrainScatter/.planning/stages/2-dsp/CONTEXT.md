# Stage 2: DSP Implementation - Context

## Discussion Summary

**Date:** 2026-02-06
**Participants:** User, Claude

## Requirements Confirmed

- All 7 DSP components from ARCHITECTURE.md are confirmed: DelayBuffer, GrainPool, GrainScheduler, TempoTracker, FreezeManager, ScaleQuantizer, EuclideanGenerator
- 64-voice grain pool with Lagrange3rd interpolation and Hann window envelopes
- Beat sync via PPQ subdivision crossing detection with Euclidean gating
- Freeze captures 4x grain size from delay buffer, loops indefinitely
- 5 scales (Chromatic, Major, Minor, Pentatonic, Whole Tone) with 4 pitch modes
- Feedback path with soft clipping safety

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| DSP file organization | Header-only (.h) in `Source/dsp/` | Fewer files, all classes are small-medium. Inline benefits for per-sample code. |
| "Texture" parameter | **Renamed to "Spread"** — both ID (`spread`) and display name. Position spread only. | User preference. Clearer name. Density stays independent. |
| Feedback safety | Soft clip at 0.95 | Prevents runaway gain while adding subtle warmth at high settings. Apply `tanh`-style saturation to feedback signal before writing back to delay buffer. |
| Latency tolerance | Accept small latency (~5ms OK) | Allows better grain scheduling accuracy and anti-click crossfading if needed. |
| Repeat gating | **New parameter: "Stutter Gate"** (Bool, default off). Off = dry signal always passes through (transparent). On = full signal gated between repeat bursts (dramatic stutter mute). | User wants both behaviors via toggle. |
| Repeat behavior | Dry-pass vs Full-gate controlled by Stutter Gate toggle | Adds 18th parameter to APVTS |

## Parameter Changes from ARCHITECTURE.md

1. **Renamed:** `texture` (ID + display) -> `spread` / "Spread"
2. **Added:** `stutter_gate` (Bool, default false) — "Stutter Gate"
3. **Total parameters:** 18 (was 17)

## Constraints Identified

- All DSP must be real-time safe (no allocations in processBlock)
- Feedback soft clip: `output = std::tanh(input * 3.0f) / std::tanh(3.0f)` scaled to 0.95 ceiling
- Euclidean pattern stored as `std::array<bool, 16>` + atomic length (thread-safe, no allocation)
- Freeze engage/release happens on audio thread (no thread crossing needed)
- Voice stealing: oldest-first round-robin (no priority system needed)

## Implementation Layers (from ROADMAP.md)

### Layer 1: Core Grain Engine
- DelayBuffer (circular buffer + Lagrange3rd read)
- GrainVoice struct + GrainPool (spawn, process, Hann window)
- GrainScheduler — Free mode (density-based timer)
- Core processBlock integration (delay -> scheduler -> pool -> mix)

### Layer 2: Beat Sync
- TempoTracker (PPQ reading, manual fallback)
- GrainScheduler — Sync mode (subdivision crossing)

### Layer 3: Pitch/Scale
- ScaleQuantizer (5 scales, root note, pitch -> rate)
- PitchLadder modes (Random, Up, Down, Pendulum)

### Layer 4: Features
- Spread control (grain position scatter, formerly "Texture")
- EuclideanGenerator + scheduler integration
- FreezeManager (capture, read, engage/release)
- Feedback path with soft clip
- Pan randomization + reverse grain playback
- Dry/wet mixing with smoothed crossfade
- Repeat count logic + Stutter Gate toggle

## Open Questions

- None — all decisions resolved.

## Next Phase

Ready for: **research** phase (then plan, execute, verify)
