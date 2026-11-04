# O-Granulator - Creative Brief

## Overview

**Type:** Audio Effect (Live-Input Granular Engine)
**Core Concept:** A deep, full-featured granular processor for live audio — four independent grain streams reading one continuously-recording buffer, with per-grain FX, a feedback network, and a deep modulation matrix.
**Status:** 💡 Ideated
**Created:** 2026-11-03

## Vision

O-Granulator is the flagship granular *effect* of the Ouaricon suite. Where O-GrainScatter is a focused stutter→cloud performer and O-Freeze is about holding a moment, O-Granulator is the open-ended sound-design engine: everything a granular processor can reasonably expose, organised so it stays playable.

Live input is written continuously into a long ring buffer. Four **grain streams** (layers) read from that shared buffer, each with its own position, pitch, density, size, window, direction, spray and per-grain FX — so one stream can be a slow, octave-up shimmer cloud while another chops tight, reversed, bit-crushed fragments in time with the host. Streams are mixed with level/pan/mute/solo.

Grain output can be **fed back into the buffer** through a tone filter, soft-clip saturation and a diffuser, producing self-evolving, smeared textures; because grains are already pitch-shifted, feedback naturally yields shimmer-style pitch cascades. A **Freeze** latch stops writing so the buffer can be explored indefinitely.

A **modulation matrix** ties it together: 4 LFOs, an envelope follower, pitch and onset followers on the input, and 4 macros, routable to any grain, stream, feedback or FX destination with bipolar depth.

Sonic goals: from transparent, pitch-quantised harmonisation through dense ambient clouds to aggressive, rhythmic, mangled fragments — all from live input, with no clicks, aliasing or runaway feedback.

## Architecture Summary

```
Input ──► [Input gain] ──► (+) ──► Ring buffer (≤30 s, Freeze latch)
                            ▲              │
                            │     ┌────────┼────────┬────────┐
                            │  Stream 1  Stream 2  Stream 3  Stream 4
                            │  (scheduler → grains → per-grain FX → pan)
                            │     └────────┴────┬───┴────────┘
                            │             Stream mixer
                            │                  │
              Feedback loop ◄─ [HP/LP tone → soft-clip → diffuser] ◄┤
                                               │
                                     Output reverb/diffuser
                                               │
                                    Dry/Wet ──► Output gain
```

## Parameters

### Global / Buffer

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| inputGain | -24 to +12 dB | 0 dB | Level into the buffer |
| bufferLength | 1 – 30 s | 8 s | Active ring-buffer span that position scans across |
| freeze | Off/On | Off | Stop writing; buffer contents held |
| syncMode | Free / Tempo | Free | Global: density/size/LFO tempo sync availability |
| mix | 0 – 100 % | 50 % | Dry/wet |
| outputGain | -24 to +12 dB | 0 dB | Final level |

### Per Stream (×4: s1…s4)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| sN_enable | Off/On | s1 On, others Off | Stream active |
| sN_level | -inf – +6 dB | 0 dB | Stream output level |
| sN_pan | L100 – R100 | C | Stream pan |
| sN_position | 0 – 100 % | 0 % | Read point behind the write head (0 = most recent) |
| sN_spray | 0 – 100 % | 10 % | Random position jitter per grain |
| sN_size | 5 – 2000 ms (or sync division) | 100 ms | Grain length |
| sN_sizeRand | 0 – 100 % | 0 % | Grain-length randomisation |
| sN_density | 0.5 – 200 grains/s (or sync division) | 10 /s | Grain rate |
| sN_jitter | 0 – 100 % | 20 % | Onset timing randomisation (sync ↔ async) |
| sN_pitch | -24 – +24 st | 0 st | Grain transposition |
| sN_pitchRand | 0 – 24 st | 0 st | Random pitch spread |
| sN_scale | Chromatic / Major / Minor / Pentatonic / Whole Tone / Octaves / Fifths / Off | Off | Quantise pitch spread |
| sN_reverseProb | 0 – 100 % | 0 % | Probability a grain plays backwards |
| sN_window | Continuous morph Hann → Tukey → Trapezoid → Exponential (perc) | Hann | Window shape |
| sN_windowSkew | -100 – +100 % | 0 | Attack/decay skew of window |
| sN_stereoSpread | 0 – 100 % | 30 % | Per-grain pan randomisation |
| sN_gainRand | 0 – 100 % | 0 % | Per-grain level jitter |

### Per-Grain FX (per stream)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| sN_filterType | Off / LP / BP / HP | Off | Per-grain SVF mode |
| sN_filterCutoff | 20 Hz – 20 kHz | 20 kHz | Base cutoff |
| sN_filterRand | 0 – 100 % (± octaves) | 0 % | Per-grain cutoff randomisation |
| sN_filterRes | 0 – 100 % | 20 % | Resonance |
| sN_drive | 0 – 100 % | 0 % | Per-grain saturation |
| sN_crush | 0 – 100 % | 0 % | Bit/sample-rate reduction amount |
| sN_crushProb | 0 – 100 % | 100 % | Probability a grain is crushed |
| sN_ringFreq | 20 Hz – 5 kHz | 200 Hz | Ring-mod carrier base frequency |
| sN_ringRand | 0 – 100 % | 0 % | Per-grain carrier randomisation |
| sN_ringMix | 0 – 100 % | 0 % | Ring-mod amount |

### Feedback Network

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| feedback | 0 – 100 % (safety-limited) | 0 % | Grain output written back into buffer |
| fbLowCut | 20 – 2000 Hz | 80 Hz | HP in loop |
| fbHighCut | 1 – 20 kHz | 12 kHz | LP in loop |
| fbSaturation | 0 – 100 % | 20 % | Soft-clip in loop (also runaway guard) |
| fbDiffusion | 0 – 100 % | 0 % | Allpass diffuser smearing in loop |

### Output FX

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| reverbMix | 0 – 100 % | 0 % | Post-mixer reverb/diffuser amount |
| reverbSize | 0 – 100 % | 50 % | Decay/size |
| reverbDamp | 0 – 100 % | 50 % | HF damping |

### Modulation

| Source / Control | Range | Default | Description |
|------------------|-------|---------|-------------|
| LFO 1–4 shape | Sine / Tri / Saw / Square / S&H / Smooth Random | Sine | Waveform |
| LFO 1–4 rate | 0.01 – 20 Hz or sync division | 0.5 Hz | Rate |
| LFO 1–4 phase | 0 – 360° | 0° | Start phase (retrigger on transport) |
| Env follower attack/release | 1 – 500 ms / 10 – 2000 ms | 10 / 200 ms | Input level follower |
| Env follower gain | -24 – +24 dB | 0 dB | Sensitivity |
| Pitch follower | — | — | Tracked input pitch as mod source (+ optional "follow" for stream pitch) |
| Onset follower threshold | -60 – 0 dB | -30 dB | Transient detection → mod pulse / grain-burst trigger |
| Macro 1–4 | 0 – 100 % | 0 % | Performance knobs, multi-destination |
| Matrix slots | ~16 slots: source → destination, bipolar depth | empty | Routes any source to any continuous parameter |

## UI Concept

Not specified in ideation (Tier 4). Implied needs: stream tab/strip selector for 4 layers, buffer waveform display with write head and per-stream position/spray regions + live grain dots, modulation matrix view. Follow the suite's EB Garamond / paper aesthetic family unless a mockup decides otherwise.

## Use Cases

- Ambient/drone: slow, frozen multi-stream clouds with feedback diffusion on pads, vocals, field recordings
- Live performance: macros + envelope/onset followers make grains react to the player in real time
- Harmonisation: streams at scale-quantised intervals (e.g. 0 / +7 / +12) for granular chords from a single line
- Rhythmic mangling: tempo-synced, reversed, crushed, ring-modded fragments on drums and loops
- Sound design: feedback cascades and per-grain filter randomisation for evolving textures

## Inspirations

- Mutable Instruments Clouds / Beads (feedback, freeze, texture-from-live-input)
- Ableton Granulator III, Output Portal, Arturia Efx Fragments (multi-layer granular effects)
- Ableton's / Bitwig's modulator-centric workflow (mod matrix, followers)
- Internal: O-GrainScatter, O-Freeze, O-simpleGrain (shared `grain-engine` module)

## Technical Notes

- **Build on `modules/synthesis/grain-engine` (v1.1.0):** WindowLut (window morph + normalisation), GrainScheduler (fractional onsets, block-size invariant), GrainPool (fade-on-steal), CounterRng (deterministic per-grain randomness), BandlimitedReader/MipRing (anti-aliased pitched-up reads), MipSnapshot (freeze).
- Shared grain pool across 4 streams (~128 grains); per-grain FX state (SVF, crush, ring osc) must live in the grain slot — no allocation on spawn.
- Feedback write into the ring that the grains read: define the read/write ordering explicitly (see memory: read-before-capture-write block-size pattern). Saturation + level limiter guard against runaway; `feedback` capped below unity loop gain.
- Freeze must interact correctly with feedback (frozen buffer + feedback = ignore write, or write-overdub mode — decide in planning).
- Pitch follower (YIN/MPM on a decimated input) and onset detector run on the input path; their latency should not affect the dry path.
- Many parameters (~150+) — mod matrix destinations should be continuous params only; per-sample smoothing where audible.
- Per-grain amplitude/density normalisation across overlapping grains (see grain density-norm memory notes).
- CPU is the main risk: per-grain SVF + ring mod + band-limited reads × 128 grains. Planning should set a budget and degrade (voice cap) gracefully.

## Next Steps

- [ ] Create UI mockup (`/start O-Granulator` → option 3)
- [ ] Stage 0 planning (`/plan O-Granulator`)
