# O-Tapestop - Creative Brief

## Overview

**Type:** Effect
**Core Concept:** Focused varispeed/playhead effect — tapestop/tapestart with curve control plus a drawable-envelope scratch mode, triggered by an automatable engage parameter.
**Status:** 💡 Ideated
**Created:** 2026-08-15

## Vision

A "Path C lite" playhead-speed plugin: one thing done extremely well. Engage the effect and the audio spins down like a stopped tape/turntable; release and it spins back up, resynchronizing to the live signal. A second mode replaces the fixed stop/start ramps with a user-drawn speed-vs-time envelope, turning the same engine into a scratch/varispeed gesture tool.

Full varispeed character (pitch and time move together) — that is the point of the effect. The DSP rides on the proven O-ReverseDelay grain engine (windowed grain reads over an absolute-index capture ring), not naked buffer interpolation, so direction changes and speed ramps stay click-free.

Seed research: `research/glitch-effects/README.md` (concept 4), `research/glitch-effects/multi-effect-sequencer-reuse-audit.md` §2, `research/stutter-effects/path-c-playhead-modulator.md`.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| engage | off/on | off | Automatable trigger. On = spin down; off = spin up + resync. Momentary-style; host automation or UI click |
| mode | Stop / Scratch | Stop | Stop = curve-shaped stop/start ramps; Scratch = drawable speed envelope plays per trigger |
| stopTime | sync 1/16–4 bars or 10 ms–8 s | 1/2 bar | Spin-down duration (tempo-synced or free, per syncMode) |
| startTime | sync 1/16–4 bars or 10 ms–8 s | 1/4 bar | Spin-up duration |
| syncMode | Sync / Free | Sync | Tempo divisions vs milliseconds for stop/start times |
| stopCurve | 0–100% | x² (≈50%) | Deceleration shape: morph linear ↔ exponential; default matches the x² tape-physics curve |
| startCurve | 0–100% | x² (≈50%) | Acceleration shape for spin-up |
| scratchEnvelope | drawn curve | gentle wobble | Speed-vs-time envelope (bipolar: negative = reverse) played once per engage in Scratch mode |
| envLength | sync 1/16–4 bars or 10 ms–8 s | 1 bar | Duration of one scratch-envelope pass |
| toneTrack | 0–100% | 60% | Physically-motivated LPF that darkens as speed drops (slower tape = duller); 0 = off |
| mix | 0–100% | 100% | Dry/wet. 100% for classic full-signal stop; lower for parallel textures |
| outputGain | -24 to +12 dB | 0 dB | Output trim |

## UI Concept

**Layout:** (volunteered by research context only — to be designed in mockup phase)
**Key Elements:** Drawable envelope editor for Scratch mode (Path C §2.2 WebView editor is the reference implementation); prominent engage control.

## Use Cases

- DJ-style tapestop on a full mix or drum bus at phrase boundaries, automated from the host timeline
- Tape-start pickup into a drop, spin-up resyncing cleanly to the live signal
- Scratch gestures on melodic/vocal material via drawn speed envelopes, retriggered per section
- Sound-design varispeed swoops and reverse smears using bipolar envelopes at long envLength

## Inspirations

- Classic tapestop plugins (Kickstart-style simplicity), dBlue Glitch tapestop slot
- TimeShaper's drawable time-curve workflow (Path C research)
- Turntable/tape physics: x² deceleration, motor spin-up lag

## Technical Notes

- **Engine:** Reuse O-ReverseDelay DSP substrate — `CaptureBuffer.h` (absolute-index stereo ring), `ReverseGrain.h` (direction field, equal-power pan, 1/sqrt(overlap) gain), `GrainScheduler.h`, `WindowLut.h`. Windowed varispeed grain reads beat naked buffer reads for click-free speed/direction changes.
- **Tape-start resync:** Signalsmith fall-behind → accelerate → crossfade-skip technique (KVR t=538470). Spin-up intentionally lags real time, then crossfade-skips forward to rejoin the live playhead.
- **Stop behavior:** After spin-down completes, output is silent while engaged; release triggers spin-up.
- **Capture ring sizing:** must span max playback debt — see memory pattern (gD_max + 2·G_max static_assert).
- **Scratch envelope:** bipolar speed values allow reverse; grain direction field handles sign flips.
- **toneTrack:** one-pole/SVF lowpass with cutoff mapped from current playback ratio; RT-safe coefficients (ArrayCoefficients pattern).
- **Effort:** ~1 week (research estimate).

## Next Steps

- [ ] Create UI mockup (`/start O-Tapestop` → option 3)
- [ ] Stage 0 planning (`/plan O-Tapestop`)
