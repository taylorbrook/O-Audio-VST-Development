---
title: "O-MicrotonalSampler Stage 2 (DSP) — Context"
created: 2026-04-27
stage: 2-dsp
phase: discuss
status: complete
---

# Stage 2 (DSP) — Context

## Discussion Summary

**Date:** 2026-04-27
**Participants:** User, Claude
**Inputs reviewed:** BRIEF.md, REQUIREMENTS.md, STATUS.md, Stage 1 RESEARCH.md / SUMMARY.md / VERIFICATION.md

## Inheritance from Stage 1 (frozen surface)

- 16 `MicrotonalSamplerVoice` instances pre-allocated; four setters wired (`setAPVTS`, `setTuningEngine`, `setPendingTuningSource`, `setSampleMapSource`)
- `vst3Extensions.drainAndUpdate()` runs **before** `synthesiser.renderNextBlock` in `processBlock` (PluginProcessor.cpp:156)
- `SampleMap` POD with `findSlot` returning `nullptr` (Stage 2.2 fills body)
- `SampleLoader : juce::Thread` skeleton — empty `run()`, deterministic failure dispatch via `MessageManager::callAsync`
- 7-parameter APVTS frozen: `attack`, `decay`, `sustain`, `release`, `polyphony`, `velocity_crossfade`, `output_gain`
- `juce_audio_formats` linked → zero CMake churn for loader work
- `TuningEngine` in global namespace (D-4); voice consumes via `tuningEngine->getFrequency(midiNote)` and `Ouaricon::NoteExpression::applyPendingTuning(...)`

## Requirements Confirmed (Stage 2 scope)

Per REQUIREMENTS.md §Traceability, Stage 2 verifies 15 requirements:

- **FUNC:** FUNC-01 (sample playback), FUNC-02 (4 vel layers), FUNC-03 (16-voice poly), FUNC-04 (auto-detect range), FUNC-07 (voice-steal)
- **DSP:** DSP-01 (varispeed ±50c), DSP-02 (cubic-Hermite interp), DSP-03 (ADSR), DSP-04 (equal-power crossfade), DSP-05 (loop auto-detect), DSP-07 (NE consumption), DSP-08 (TuningEngine consumption)
- **PERF:** PERF-01 (RT-safe processBlock), PERF-02 (≤5% CPU @ 16 voices), PERF-03 (background loader), PERF-04 (zero latency)
- **COMPAT:** COMPAT-02 (16/24/32-bit AIF/WAV, any SR)
- **QUAL:** QUAL-01 (no clicks / aliasing / zipper / boundary discontinuity)

Out of scope this stage: FUNC-05/06 (drag-drop UI, manual cell assignment) and DSP-06 (manual loop override) — Stage 3.

## Approach Decisions (Locked)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D2-1 | Interpolator quality | **Cubic-Hermite (4-pt)** with conditional 1st-order tilt LPF pre-filter | Cheapest 3rd-order; matches DSP-02. ±50c → ~3% speedup; foldback risk only for content >~10 kHz. Pre-filter only inserted if Stage 2.1 sine-sweep null test shows audible aliasing (Q3 from Stage 1 RESEARCH.md). |
| D2-2 | Voice-stealing strategy | **Override `juce::Synthesiser::findVoiceToSteal`** — oldest-released → oldest-held fallback | Matches BRIEF.md + FUNC-07 acceptance. Default JUCE picks oldest-active regardless of release state. Implemented in 2.4 alongside ADSR (release stage is meaningless until ADSR exists). |
| D2-3 | Voice-steal release ramp | **5 ms linear** (240 samples @ 48 kHz) before reuse | Fast enough to feel instant; slow enough to prevent click even when user release=0. Matches O-Lyrica `HarpSynthVoice` steal pattern. Replacement note's attack stage masks any residual artifact. |
| D2-4 | Sustain-loop auto-detect | **RMS scan + zero-crossing snap** | 1024-sample RMS window over latter 60% of sample → pick lowest-RMS region → snap loop start/end to nearest zero-crossing within ±64 samples → 8-sample equal-power crossfade at loop boundary. Fallback to one-shot if RMS variance high (transient material). |
| D2-5 | ADSR implementation | **`juce::ADSR`** (linear segments) | Matches every other suite synth (O-Lyrica, O-Wind, O-Bassoon). Sample-accurate, RT-safe, plays cleanly with steal ramp logic. Custom exponential rejected — adds bug surface for inaudible benefit on sustained sample material. |
| D2-6 | Sub-stage execution order | **2.1 voice DSP → 2.2 loader → 2.3 vel crossfade → 2.4 voice-steal → 2.5 loop-detect** | DSP-first: makes sound on day 1 (hardcoded test fixture, then real loader). Voice-stealing & loop-detect trail because they need ADSR + real samples respectively. Standard Ouaricon pattern (matches O-Bassoon Stage 2). |
| D2-7 | Filename parsing | **Tolerant per BRIEF.md** (case-insensitive, multi-convention) | Accept: `note_velocity`, `note-velocity`, sharps (`C#4`) and flats (`Db4`), MIDI numbers (`60_v2`), layer suffixes (`_p`/`_mp`/`_mf`/`_f` → v1..v4). Silently skip + log unparseable files. Matches FUNC-05 spec. |
| D2-8 | MIDI notes outside loaded range | **Silence** | Matches BRIEF.md exactly. No nearest-fallback varispeed (would break QUAL-01 beyond ±50c). Editor-side greyout deferred to Stage 3. |
| D2-9 | Source SR ≠ host SR | **`juce::LagrangeInterpolator` at load time** (one-time) | 5th-order resample once into host-SR buffer. Per-voice playback then runs simple Hermite at ±50c only. Matches O-TextureForge `CorpusLoader` precedent. One-time CPU on background thread; zero runtime cost. |
| D2-10 | Mono → stereo render | **Duplicate mono to L/R at unity gain** | Matches Decent Sampler / Kontakt default. No phase issues, predictable loudness, no plugin in the suite uses -3 dB convention. |
| D2-11 | Parameter smoothing | **`output_gain` + `velocity_crossfade`** via `juce::SmoothedValue` | ADSR params use `juce::ADSR`'s own handling. Polyphony is integer-quantized (no smoothing applies). The two continuous mod-targets get smoothing as cheap zipper-noise insurance. |
| D2-12 | NE pitch update granularity | **Once at `startNote()`** | Read `pendingTuningTable` in `startNote`, bake into voice's read rate via `Ouaricon::NoteExpression::applyPendingTuning(...)`. Mid-note pitch changes wait for next note. Matches O-Lyrica `HarpSynthVoice` pattern. Sufficient for "sustained microtonal long-tones" use case. |

## Constraints Identified

- **RT-safety (PERF-01):** `processBlock` must be allocation-free, lock-free, I/O-free. Voice grabs `std::shared_ptr<SampleMap>` snapshot via `std::atomic_load` at `startNote` (refcount inc only) and holds for note duration.
- **CPU budget (PERF-02):** ≤5% on Apple Silicon @ 48 kHz / 256-buffer for 16 held voices. Cubic-Hermite + ADSR + crossfade is well within budget; conditional pre-filter is the only watch-item.
- **Zero added latency (PERF-04):** No FFT, no lookahead. All processing is per-sample causal.
- **Sample-rate floor:** Stage 2.1 must work at 44.1 / 48 / 88.2 / 96 / 176.4 / 192 kHz host SR. Source samples can be any SR; LagrangeInterpolator at load handles the conversion.
- **Stage 1 surface invariant:** No method-signature churn on `MicrotonalSamplerVoice` setters. New private members only.
- **D-4 (TuningEngine global namespace):** No `Ouaricon::` prefix; voice calls `tuningEngine->getFrequency(...)`.

## Edge Cases to Handle

| # | Case | Stage 2 behavior |
|---|---|---|
| EC-1 | `findSlot` returns `nullptr` (note out of range, or velocity layer empty) | Voice clears note immediately; no audio produced. |
| EC-2 | `sampleMapSource` itself is nullptr (no samples loaded yet) | Voice clears note immediately. |
| EC-3 | User loads new sample folder while notes are sounding | Active voices keep their old `shared_ptr<SampleMap>` snapshot; new notes pick up the new map. No mid-note swap. |
| EC-4 | Sample shorter than ADSR attack | Sample plays to end, voice holds at last sample value (or zero if loop disabled), ADSR continues. |
| EC-5 | Velocity exactly at layer boundary | Equal-power crossfade weights both adjacent layers (DSP-04). |
| EC-6 | Voice-steal arrives during attack stage of stolen voice | 5 ms linear ramp from current envelope value to zero, then voice reused. |
| EC-7 | Loop region detect fails (high RMS variance / no zero-crossing) | Fall back to one-shot — sample plays through, ADSR release governs end. |
| EC-8 | Source SR ratio causes >2× resample factor | LagrangeInterpolator handles up to ~8× cleanly; flag any sample with extreme ratio in load-time log. |
| EC-9 | `polyphony` param reduced mid-performance from 16 → N | Excess voices steal-released on next note-on (not killed mid-note). |
| EC-10 | NE pitch delta exceeds ±50c | Voice still plays; quality degrades (aliasing risk). Out-of-spec but doesn't crash. |

## Performance Targets

- **CPU:** ≤5% @ 16 voices, 48 kHz, 256-sample buffer on Apple Silicon (PERF-02)
- **Allocation:** zero in `processBlock` and `renderNextBlock` (verified via allocation guard or strict pluginval pass)
- **Load time:** background thread, no audio-thread blocking — typical 88-note × 4-layer library should load in <5 s on SSD

## Open Questions for Research Phase

| # | Question | Why research-stage |
|---|---|---|
| RQ-1 | Cubic-Hermite aliasing measurement at +50c | Need a sine-sweep null test plan; if foldback >-60 dB above 8 kHz, insert 1st-order tilt LPF ahead of interpolator. Result is binary (filter / no filter); doesn't change voice surface. |
| RQ-2 | RMS window size + search range tuning | 1024-sample / latter-60% are starting points from research-lead's brief. Validate against typical sustained-tone material (bowed strings, woodwinds, vocals) — may need adaptive window. |
| RQ-3 | Voice-steal ramp implementation site | Inside `MicrotonalSamplerVoice` (envelope override) or at `findVoiceToSteal` level (caller initiates ramp)? Latter is cleaner and matches O-Lyrica. Confirm by inspection. |
| RQ-4 | `juce::Synthesiser::findVoiceToSteal` override hook | Subclass `juce::Synthesiser` and override, or use `setNoteStealingEnabled(true)` + per-voice `getCurrentlyPlayingNote()` discrimination? Likely subclass — cleaner. |
| RQ-5 | Filename parser pattern set | Concrete regex / glob list for the tolerant parser. Reference: O-TextureForge filename conventions (if any), Decent Sampler/EXS24 conventions. |
| RQ-6 | LagrangeInterpolator sample-rate ratio limits | Confirm clean range; document fallback behavior for extreme ratios (e.g., 8 kHz source on 192 kHz host). |
| RQ-7 | Equal-power crossfade math for both vel-layer crossfade and loop-boundary crossfade | Same `cos(θ)/sin(θ)` formulation? Yes — confirm and reuse helper. |

## Next Phase

Ready for: **research** phase

`/plugin-research O-MicrotonalSampler 2-dsp`
