# O-simpleAdditive — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simpleAdditive
created: 2026-06-22
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are likely additions/confirmations Stage 0 should resolve and fold into the final spec.

## Additive Spectrum — Frame A (the 16 harmonic drawbars)

THE defining additive control surface. Each drawbar sets one partial's amplitude; the bars double as the live spectrum display (UI-01).

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Partial 1 Level | `partial1` | float | 0–100 | 100 | % | Fundamental. |
| Partial 2 Level | `partial2` | float | 0–100 | 0 | % | 2nd harmonic. |
| Partial 3 Level | `partial3` | float | 0–100 | 0 | % | 3rd harmonic. |
| Partial 4 Level | `partial4` | float | 0–100 | 0 | % | 4th harmonic. |
| Partial 5 Level | `partial5` | float | 0–100 | 0 | % | 5th harmonic. |
| Partial 6 Level | `partial6` | float | 0–100 | 0 | % | 6th harmonic. |
| Partial 7 Level | `partial7` | float | 0–100 | 0 | % | 7th harmonic. |
| Partial 8 Level | `partial8` | float | 0–100 | 0 | % | 8th harmonic. |
| Partial 9 Level | `partial9` | float | 0–100 | 0 | % | 9th harmonic. |
| Partial 10 Level | `partial10` | float | 0–100 | 0 | % | 10th harmonic. |
| Partial 11 Level | `partial11` | float | 0–100 | 0 | % | 11th harmonic. |
| Partial 12 Level | `partial12` | float | 0–100 | 0 | % | 12th harmonic. |
| Partial 13 Level | `partial13` | float | 0–100 | 0 | % | 13th harmonic. |
| Partial 14 Level | `partial14` | float | 0–100 | 0 | % | 14th harmonic. |
| Partial 15 Level | `partial15` | float | 0–100 | 0 | % | 15th harmonic. |
| Partial 16 Level | `partial16` | float | 0–100 | 0 | % | 16th harmonic. |

> Default H1=100% / rest=0% yields a pure sine on load — the cleanest pedagogical starting point (FUNC-01 acceptance: fundamental-only = sine).

## Wavetable Dimension — Scan / Morph (Frame A → Frame B)

The wavetable is an *added dimension* on the always-additive engine, not a separate mode.

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Frame B Source | `frameBSource` | choice | sine / saw / square / odd | saw | — | Target spectrum the scan morphs toward. **RESOLVED (Stage 0): preset-only — NO second drawbar set** (keeps 33 params + one readable spectrum). Editable "capture as B" deferred to v1.1. |
| Scan Position | `scanPosition` | float | 0–100 | 0 | % | Morph pointer A↔B. 0 = Frame A, 100 = Frame B (FUNC-02). |
| Scan LFO Rate | `scanLfoRate` | float | 0.01–20 | 0.5 | Hz | LFO speed driving scan (log skew). **RESOLVED (Stage 0): sine shape only, one global LFO (all notes morph in phase). Shape selector + per-voice retrigger deferred to v1.1.** |
| Scan LFO Depth | `scanLfoDepth` | float | 0–100 | 0 | % | How far the LFO sweeps scan. |
| Scan Env Amount | `scanEnvAmount` | float | -100–100 | 0 | % | Mod-envelope amount routed to scan (once-per-note evolution, FUNC-03). Bipolar. |

## Spectral Shaping

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Spectral Decay | `spectralDecay` | float | 0–100 | 0 | % | Macro: how much faster higher partials decay than lower ones over the note (DSP-04). **RESOLVED (Stage 0): per-partial multiplier `D_k = exp(−rate·k·tau)`, `k=0..15` (fundamental never decays), `rate = norm·RATE_MAX`, `tau` = internal 0→1 ramp over note length so the spectrum visibly tilts. At 0, all D_k=1.** |
| Bit Depth | `bitDepth` | choice | off / 12 / 10 / 8 / 6 / 4 / 2 | off | bits | Amplitude-resolution quantization of the summed waveform — early-digital grit (DSP-05). **RESOLVED (Stage 0): discrete `AudioParameterChoice`; "off" = clean passthrough sentinel; else `round(x·L)/L`, `L=2^(bits−1)` at read time, no dither.** |
| Velocity → Decay | `velToDecay` | float | 0–100 | 0 | % | **NEW (Stage 0):** velocity → spectral decay amount, opt-in. Velocity → amplitude is always-on (not a param). |

## Amplitude Envelope (ADSR → per-voice output)

| Param | ID | Type | Range | Default | Unit |
|-------|----|------|-------|---------|------|
| Amp Attack | `ampAttack` | float | 0–5 | 0.005 | s |
| Amp Decay | `ampDecay` | float | 0–5 | 0.3 | s |
| Amp Sustain | `ampSustain` | float | 0–100 | 80 | % |
| Amp Release | `ampRelease` | float | 0–5 | 0.1 | s |

## Modulation Envelope (ADSR → scan position)

Feeds `scanEnvAmount` for once-per-note spectral evolution.

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Mod Attack | `modAttack` | float | 0–5 | 0.005 | s | |
| Mod Decay | `modDecay` | float | 0–5 | 0.3 | s | |
| Mod Sustain | `modSustain` | float | 0–100 | 80 | % | |
| Mod Release | `modRelease` | float | 0–5 | 0.1 | s | **RESOLVED (Stage 0): mod-env routes to SCAN only in v1.0; mod-env→spectral-decay deferred to v1.1.** |

## Output

| Param | ID | Type | Range | Default | Notes |
|-------|----|------|-------|---------|-------|
| Output Level | `outputLevel` | float | -inf–0 dB | 0 | Master output gain. |

## Resolved Decisions (Stage 0 Research & Planning — 2026-06-22)

The 8 open questions are resolved below. Full rationale in `research/ARCHITECTURE.md`
(§ "Resolved research decisions") and `stages/0-ideation/CONTEXT.md`.

1. **Render strategy → precompute a band-limited single-cycle wavetable per note, read by phase.**
   Not per-sample sum-of-16-sines. Fill one 2048-pt table from the current 16 amplitudes when the
   spectrum changes (note-on / scan / decay / drawbar move, at control rate), read with linear interp.
   Real-time safe (fixed buffer, bounded refills) and faithful (the table IS the visible waveshape).
2. **Anti-aliasing → per-note `Kmax = floor(0.5·fs/f0)`; omit partials k>Kmax; raised-cosine taper on top 2.**
   Exact band-limit (don't write the partial), zero filter cost, no oversampling. Taper avoids boundary clicks.
3. **Morph → linear *spectral* (per-partial amplitude lerp), `active_k = lerp(A_k,B_k,scan)`.**
   Phase-coherent, zipper-free with smoothed scan; truthful "the spectrum morphs" picture.
4. **Frame B → PRESET-ONLY (4-way choice: sine/saw/square/odd). NO second drawbar set** (stays 33 params).
   Editable "capture current drawbars as B" deferred to v1.1.
5. **Spectral-decay → `D_k = exp(−rate·k·tau)`** (k=0..15, fundamental never decays; `tau` ramps 0→1 over the note).
6. **Bit depth → discrete `AudioParameterChoice {off,12,10,8,6,4,2}`**, "off" = clean passthrough; `round(x·L)/L`, no dither.
7. **Polyphony → 16 voices** (matches O-simpleFM).
8. **Velocity → amp always-on; velocity→spectral-decay opt-in (`velToDecay`, default 0).**
9. **Visualization → reuse O-simpleFM `VizRing` + `FmVizAnalyzer` (lock-free ring + message-thread 4096 FFT).**
   Drawbar bars driven from a lock-free **active-spectrum snapshot** (morphed + decayed amplitudes), exact.

**Also locked:** LFO is **sine-only, one global LFO** in v1.0 (shape selector + per-voice retrigger → v1.1);
mod-env routes to **scan only** in v1.0 (mod-env→decay → v1.1). **No oversampling, zero added latency.**

**Final v1.0 core parameter count: 33** (16 drawbars + frameBSource + scanPosition + scanLfoRate +
scanLfoDepth + scanEnvAmount + spectralDecay + bitDepth + velToDecay + amp ADSR×4 + mod ADSR×4 + outputLevel).

---
*Draft generated from BRIEF.md on 2026-06-22; Stage 0 decisions folded in 2026-06-22. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
