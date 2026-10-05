# O-simpleWavetable — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simpleWavetable
created: 2026-10-05
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are open questions Stage 0 should resolve and fold into the final spec.

## Bank + Oscillator

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Bank | `bank` | choice | Sine→Saw / Sine→Square / Pulse Width / Formant / Drive / Imported | Sine→Saw | — | "Imported" active only after an import *(research: behaviour when selected with no import)* |
| Position | `position` | float | 0–100 | 0 | % | Scan position, frame 1 → frame N. Smoothed. |
| Interpolation | `interp` | bool | Off/On | On | — | Frame-to-frame only: Off = nearest frame, On = crossfade. |
| Band-limiting | `bandlimit` | bool | Off/On | On | — | On = per-octave mipmaps; Off = raw frames (intended aliasing). |
| Bit Depth | `bit_depth` | choice | Full, 16, 15 … 3 | Full | bits | Post-interpolation quantize; Full = bypass. *(research: choice vs stepped int)* |

## Movement — LFO (global, → Position)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| LFO Rate | `lfo_rate` | float | 0.01–20 | 0.5 | Hz | Free mode. Skewed taper *(research)*. |
| LFO Sync Division | `lfo_div` | choice | note divisions | 1/4 | — | Used when Sync = Tempo *(research: division list; separate param vs shared knob)* |
| LFO Sync | `lfo_sync` | choice | Free / Tempo | Free | — | |
| LFO Shape | `lfo_shape` | choice | Sine / Triangle / Saw / Square / S&H | Triangle | — | |
| LFO Depth | `lfo_depth` | float | 0–100 | 0 | % | Bipolar sweep around Position. |

## Movement — Mod Envelope (per voice, → Position)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Mod Env Attack | `menv_attack` | float | 0–10 | 0.5 | s | Skewed taper. |
| Mod Env Decay | `menv_decay` | float | 0–10 | 1.0 | s | Skewed taper. |
| Mod Env Sustain | `menv_sustain` | float | 0–100 | 0 | % | |
| Mod Env Release | `menv_release` | float | 0–10 | 0.5 | s | Skewed taper. |
| Env Amount | `env_amount` | float | −100–+100 | 0 | % | Bipolar. |

## Amp + Output

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Amp Attack | `amp_attack` | float | 0–5 | 0.005 | s | Skewed taper. |
| Amp Decay | `amp_decay` | float | 0–5 | 0.3 | s | Skewed taper. |
| Amp Sustain | `amp_sustain` | float | 0–100 | 80 | % | |
| Amp Release | `amp_release` | float | 0–5 | 0.2 | s | Skewed taper. |
| Voice Mode | `voice_mode` | choice | Poly / Mono | Poly | — | Poly = 16 voices; Mono = last-note priority. |
| Output Level | `output_level` | float | −inf–+6 | −6 | dB | −inf floor representation *(research)*. |

## Non-parameter state

- Imported bank: frame data (≤ 256 × 2048) + source filename, saved in plugin state *(research: compact encoding)*.

## Parameter Count Summary

- **Float:** 14
- **Choice:** 6 (bank, bit_depth, lfo_div, lfo_sync, lfo_shape, voice_mode)
- **Bool:** 2 (interp, bandlimit)
- **Total:** 22 (21 from the brief + `lfo_div`, implied by "note divisions (sync)")
