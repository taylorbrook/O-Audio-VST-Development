# O-MicrotonalSampler — Creative Brief

## Overview

**Type:** Synth (Sampler Instrument)
**Core Concept:** A user-loaded sample-based instrument that retunes user samples in real time using the suite's microtonal infrastructure (VST3 note expression + internal Scala/Dorico-compatible tuning module), optimized for sustained microtonal long-tones with up to 4 velocity layers per pitch.
**Status:** 💡 Ideated
**Created:** 2026-04-27

## Vision

O-MicrotonalSampler is the suite's sample-engine companion to its physical-model synths (O-Bassoon, O-Lyrica, O-Wind, O-Reed, O-Bowed, O-IntonationPad). Where the others synthesize tone from scratch, O-MicrotonalSampler lets the composer load any user-recorded acoustic instrument or sustained timbre — one sample per semitone, optionally up to 4 velocity layers — and play it back with full microtonal precision driven by the same VST3 note-expression / internal tuning system that powers the rest of the suite (Scala-compatible internally, Dorico-compatible externally).

The design priority is **sustained microtonal long-tones**: looped sustains, smooth crossfades, and per-note tuning that flows seamlessly from notation hosts (Dorico) and microtonal controllers. Because no sample is ever retuned by more than ±50 cents (each semitone has its own source sample), retuning can be done with simple **varispeed playback** — fast, precise, low-CPU, and inaudibly compressing/expanding sample length by ≤3%. No phase vocoder, no formant correction, no resampling overhead.

The plugin ships as a **pure sample engine** in v1.0 — no onboard filter, no reverb, no effects. Any tone-shaping happens upstream (note expression) or downstream (the user's effects chain). This keeps the engine minimal, the CPU low, and the role of the plugin unambiguous: load samples, retune them precisely, play them back.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Attack | 0–10 s | 0.005 s | Amplitude envelope attack time |
| Decay | 0–10 s | 0.1 s | Amplitude envelope decay time |
| Sustain | 0–1 | 1.0 | Amplitude envelope sustain level |
| Release | 0–10 s | 0.3 s | Amplitude envelope release time |
| Polyphony | 1–16 | 16 | Max simultaneous voices (fixed cap; user-trim down for CPU) |
| Velocity Crossfade | 0–100% | 100% | Width of equal-power crossfade between adjacent velocity layers (0% = hard switch, 100% = full overlap) |
| Output Gain | -24 to +12 dB | 0 dB | Master output trim |

Tuning behavior is driven by VST3 note expression and the suite's internal tuning module — **not** exposed as plugin parameters.

## Sample Loading & Mapping

**Two parallel workflows (both supported):**

1. **Drag-drop folder + filename convention** (fast path) — User drops a folder of `.wav` / `.aif` files; plugin auto-parses pitch + velocity layer from filenames (e.g. `C4_v1.wav`, `A2_v3.aif`, `Eb5_v4.wav`). Tolerant of common conventions: `note_velocity`, `note-velocity`, sharps/flats, MIDI note numbers, optional layer suffixes (`_p`, `_mp`, `_mf`, `_f`).
2. **Per-note manual assignment grid** (override path) — UI grid of pitch × velocity-layer slots; user can drag-drop individual files to fix or replace any cell.

**Note range:** auto-detected from loaded samples — no fixed boundary. Notes outside the loaded range produce silence.

**Velocity layer mapping:** equal split across the supplied number of layers (1, 2, 3, or 4). 4 layers → ranges 1–31 / 32–63 / 64–95 / 96–127. Adjacent layers crossfade with **equal-power** weighting near the boundaries (width controlled by Velocity Crossfade parameter).

**Round-robin:** not supported in v1.0. Strictly one sample per (pitch × velocity-layer) cell.

## Sustain & Looping

**Auto-detect loop points** by default — engine scans each sample for a low-energy zero-crossing region in the latter portion of the file and loops there during sustain.

**Manual loop-point override** — per-sample UI control for users who want to set loop start / end explicitly (essential for difficult material).

**One-shot fallback** — samples without a discoverable loop region play through once and rely on the release stage of the envelope.

## Microtonal Retuning

**Algorithm:** Varispeed playback. Each voice reads its sample at a fractional rate determined by `2^(cents/1200)` to achieve the target pitch. Fractional sample reads use cubic-Hermite (or comparable) interpolation. Anti-aliasing for upward varispeed (≤+50c → ~3% speedup) is handled by the interpolator's natural rolloff; further filtering is unnecessary at this small a shift.

**Tuning source:** VST3 note expression (per-note pitch) + the suite's internal tuning module (Scala-compatible, Dorico-compatible). The plugin does **not** define its own tuning system; it consumes the suite-wide one.

**Retune budget:** ±50 cents per voice (since every semitone has its own sample). Larger shifts (e.g. unloaded notes, hardware mismatches) fall back to the nearest sample but are explicitly out-of-scope for v1.0 quality guarantees.

## Voice Architecture

- **16-voice polyphony** (fixed maximum)
- Per-voice state: source sample reference, current playback position (fractional), current pitch in cents, ADSR stage, velocity-layer crossfade weights
- Voice-stealing strategy: oldest-released voice first; oldest-held voice if all are held (TBD in Stage 0 research)
- **Equal-power crossfade** between adjacent velocity layers when a note's velocity falls in a boundary region

## UI Concept

UI design is deferred to the dedicated mockup phase (`/start O-MicrotonalSampler` → option 3). Captured intent only:
- Sample-mapping grid (pitch × velocity layer) is the core surface
- Drag-drop area for bulk folder loading
- Per-cell click-to-replace for manual override
- Loop-point editor (waveform view + draggable loop markers) on demand
- ADSR + Velocity Crossfade + Output Gain in a compact control strip
- Visual indication of the active tuning system (read-only — driven by the suite tuning module)

## Use Cases

- **Sustained microtonal long-tones** — pads, drones, slow-attack bowed/wind/vocal samples played in just intonation, equal divisions, or any Scala scale
- **User-sampled acoustic instruments** — composer records their own instrument (or commissions samples), loads it, gets full microtonal playback
- **Dorico-driven composition** — notation host sends per-note pitch via the suite's tuning module; sampler plays back deterministically
- **Custom timbres for microtonal works** — anything that can be sampled per semitone becomes a microtonally-precise instrument

## Inspirations

- **O-Bassoon, O-Lyrica, O-Wind, O-Reed, O-Bowed, O-IntonationPad** — defines the microtonal infrastructure this plugin plugs into
- **Decent Sampler** — minimal sampler UX, drag-drop folder workflow
- **Kontakt / EXS24 / TX16Wx** — velocity-layer / sample-map conventions
- **Sustained-tone microtonal repertoire** (Tenney, Lamb, Wyschnegradsky-style microtonal pads) — drives the v1.0 design priority

## Technical Notes

- **DSP path:** Per-voice `processBlock` reads source sample at fractional rate (cubic-Hermite interp), applies ADSR, sums into voice mix bus, applies velocity-layer crossfade weights, sums to output
- **Memory:** Samples stored in RAM (not streamed) for v1.0 — typical orchestral instrument library at 24-bit / 48 kHz, 88 notes × 4 velocity layers ≈ 100–500 MB depending on sample length
- **Real-time safety:** Sample loading happens on a background thread; `processBlock` only reads pre-loaded buffers
- **CPU target:** 16 voices × varispeed read + ADSR + crossfade weights → expected well under 5% CPU on modern hardware
- **Latency:** Zero added latency (no FFT, no lookahead)
- **Tuning interface:** Consumes VST3 note expression events + suite's internal tuning module; no per-plugin tuning UI
- **Sample formats:** AIF and WAV (16/24/32-bit, mono/stereo, any sample rate — converted on load)

## Out of Scope (v1.0)

| Feature | Reason | Future |
|---------|--------|--------|
| Round-robin / multi-take per cell | Brief explicitly scopes to one sample per cell | v1.1+ |
| Onboard filter / EQ / reverb | Pure sample engine in v1.0 — keeps role unambiguous | v1.1+ |
| Phase vocoder / formant preservation | Unnecessary at ±50c | n/a |
| Pitch wheel / mod wheel routing | Tuning is driven entirely by suite note-expression system | TBD |
| Streaming from disk | RAM-only sufficient for typical use | v2.0 |
| Sample browser / preset library UI | Out of scope — files-on-disk + folder-drop is enough | v1.1+ |
| Mono / legato mode | Conflicts with sustained long-tone use case | TBD |

## Next Steps

- [ ] Create UI mockup (`/start O-MicrotonalSampler` → option 3)
- [ ] Stage 0 research: voice-stealing strategy, loop-point auto-detection algorithm, anti-aliasing margin for cubic-Hermite at ±50c
- [ ] Start implementation (`/plugin-research O-MicrotonalSampler`)
