# O-Tapestop - Parameter Specification (Draft)

## Overview

Focused varispeed/playhead effect — tapestop/tapestart with curve control plus a drawable-envelope scratch mode, triggered by an automatable engage parameter. Full varispeed character (pitch and time coupled), riding on the O-ReverseDelay grain engine for click-free speed ramps and direction flips.

Source: extracted from `BRIEF.md` (2026-08-15). Refine into full `parameter-spec.md` after UI mockup.

## Parameters

### Trigger & Mode

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| ENGAGE | Engage | Bool | Off / On | Off | - | Automatable trigger. On = spin down; off = spin up + resync to live signal. Momentary-style; host automation or UI click behave identically (FUNC-01). |
| MODE | Mode | Choice | Stop / Scratch | Stop | - | Stop = curve-shaped stop/start ramps; Scratch = drawn speed envelope plays once per engage (FUNC-02). |
| SYNC_MODE | Sync Mode | Choice | Sync / Free | Sync | - | Tempo divisions vs milliseconds for all three times below (FUNC-03). |

### Stop / Start (Stop mode)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| STOP_SYNC_DIV | Stop Time | Choice | 1/16 – 4 bars | 1/2 bar | - | Spin-down duration in Sync mode (musical divisions: 1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars). |
| STOP_FREE_MS | Stop Time (Free) | Float | 10.0 - 8000.0 | 500.0 | ms | Spin-down duration in Free mode. Skewed range. |
| STOP_CURVE | Stop Curve | Float | 0.0 - 100.0 | 50.0 | % | Deceleration shape: morph linear ↔ exponential; 50% matches the x² tape-physics curve (DSP-02). |
| START_SYNC_DIV | Start Time | Choice | 1/16 – 4 bars | 1/4 bar | - | Spin-up duration in Sync mode (same division set). |
| START_FREE_MS | Start Time (Free) | Float | 10.0 - 8000.0 | 250.0 | ms | Spin-up duration in Free mode. |
| START_CURVE | Start Curve | Float | 0.0 - 100.0 | 50.0 | % | Acceleration shape for spin-up; same linear↔exponential morph. |

### Scratch (Scratch mode)

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| ENV_SYNC_DIV | Env Length | Choice | 1/16 – 4 bars | 1 bar | - | Duration of one scratch-envelope pass in Sync mode. |
| ENV_FREE_MS | Env Length (Free) | Float | 10.0 - 8000.0 | 1000.0 | ms | Envelope pass duration in Free mode. |

**scratchEnvelope** (drawn speed-vs-time curve, bipolar: negative = reverse) is NOT an APVTS parameter — it is a drawn-curve state blob persisted in plugin state and edited via the WebView envelope editor (UI-01). Default: gentle wobble. Stage 0 specifies the storage format and audio-thread handoff.

### Output

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| TONE_TRACK | Tone Track | Float | 0.0 - 100.0 | 60.0 | % | Physically-motivated LPF that darkens as speed drops (slower tape = duller); 0 = off (DSP-05). |
| MIX | Mix | Float | 0.0 - 100.0 | 100.0 | % | Dry/wet. 100% for classic full-signal stop; lower for parallel textures. |
| OUTPUT_GAIN | Output Gain | Float | -24.0 - 12.0 | 0.0 | dB | Output trim. |

## Parameter Count Summary

- Trigger & Mode: 3
- Stop/Start: 6
- Scratch: 2
- Output: 3
- **Total: 14** (+ scratchEnvelope state blob)

## Design Notes

- **Sync/free time split** — BRIEF listed single `stopTime`/`startTime`/`envLength` params with dual ranges; split into sync-division Choice + free-ms Float per time (O-Bitrot CLOCK_MODE precedent) so both modes hold stable automatable values. Stage 0 finalizes the division list.
- **ENGAGE is the performance-critical param** — rapid engage/release mid-ramp must reverse direction without clicks (FUNC-01); the current playback ratio is the ramp start point, never a jump.
- **Engine reuse:** O-ReverseDelay substrate — `CaptureBuffer.h` (absolute-index stereo ring), `ReverseGrain.h` (direction field), `GrainScheduler.h`, `WindowLut.h`. Capture ring must span max playback debt: gD_max + 2·G_max in a static_assert (repo pattern).
- **Resync:** Signalsmith fall-behind → accelerate → crossfade-skip; post-resync output null-tests against dry (DSP-03).
- **Stopped state:** output silent while fully stopped and engaged (FUNC-04).
- **toneTrack coefficients:** one-pole/SVF with cutoff mapped from current playback ratio; ArrayCoefficients-style RT-safe updates, never Coefficients::makeXXX (repo pattern).
- **Block-size invariance:** grain reads latched after the block's capture write; 512-vs-4096 renders bit-identical (QUAL-01 / DSP-01 acceptance).

## Source

Extracted from `BRIEF.md` on 2026-08-15 to unblock Stage 0 planning. Will be superseded by full `parameter-spec.md` after UI mockup phase.
