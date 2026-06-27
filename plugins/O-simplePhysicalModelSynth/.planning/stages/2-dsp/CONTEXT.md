# Stage 2 (DSP) — CONTEXT

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 2 of 4 — DSP Implementation
**Phase:** discuss ✓
**Date:** 2026-06-26
**Mode:** manual (interactive discuss)

---

## Goal

Implement the audio engine: `EXCITATION → RESONATOR → MATERIAL/DAMPING`. Parameters
(wired silent in Stage 1) now control real DSP. Produce a tunable, decaying, dynamically
responsive physical-modeling synth across String (Karplus-Strong) and Modal engines, with
three exciters (Pluck/Strike/Bow). Gate correctness with the offline render-harness
(autocorrelation pitch probe).

The DSP design is already fully specified and immutable in
`research/ARCHITECTURE.md` (all 8 open questions resolved). This discuss phase only
settles **Stage 2 execution scope** — it does NOT re-litigate DSP decisions.

---

## Decisions (this discuss phase)

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D1 | Waveguide `nice` phase (2.4) scope | **DEFER to v1.1** | KS is the v1.0 String engine; Position realized via exciter comb. `stringModel` choice param (wired in Stage 1) exposes **KS only** in v1.0 — no contract break. Lower risk, faster to v1.0. Stage 2 = **3 must-phases only** (2.1, 2.2, 2.3). |
| D2 | Bow exciter approach (HIGH risk) | **Memoryless STK bow-table first, filtered-noise drive as fallback** | As architected. Implement memoryless friction, validate a basic sustained tone BEFORE refining. Fall back to sustained band-limited friction-noise drive ONLY if the table reads poorly or destabilizes the KS loop. Hard-clamp output; epsilon-guard divisions. |
| D3 | Execute granularity | **Phased, harness-gated per phase** | Run 2.1 → gate on harness, then 2.2 → gate, then 2.3 → gate. Each phase validated before the next. Safest for the HIGH-risk Bow (2.2) and KS tuning accuracy (2.1). |

---

## Stage 2 Scope (LOCKED for this pass)

**IN (3 must-phases):**
- **Phase 2.1 — Core KS String + Tuning + Pluck:** `StringResonator` (Thiran `DelayLine` + `OnePoleLPF` loop filter + clamped feedback, group-delay-compensated length), `PluckExciter` (filtered noise burst + brightness LPF + position comb), Damping/Decay mapping, Amp ADSR, 16-voice poly, output gain, coarse/fine tune. Verifies FUNC-01, DSP-01, DSP-02, DSP-03 (partial), FUNC-02 (Pluck).
- **Phase 2.2 — Strike + Bow + Material + Velocity + Position:** `StrikeExciter` (band-limited raised-cosine impulse + hardness LPF), `BowExciter` (memoryless STK friction junction drive, sustains), Material macro (co-moves Damping+Decay, log-cutoff + linear-feedback), Velocity→amp+brightness (`velToBrightness`), Excitation Position comb on KS. Verifies FUNC-02 (all exciters), FUNC-06, DSP-07, DSP-08, QUAL-01.
- **Phase 2.3 — Modal resonator + cross-driving:** `ModalResonator` (8 parallel resonant bandpass biquads, custom direct-form, driven by `e[n]`), Inharmonicity stretch `f_k = f0·k·√(1+B·k²)`, per-mode T60 + `DECAY_MULTIPLIERS`, Mode Brightness tilt, `resonatorType` switch, modal-stem viz snapshot. Verifies FUNC-03, FUNC-04, DSP-04, DSP-05.

**Viz wiring (through Stage 2, consumed in Stage 3):** `VizRing` lock-free ring + atomic
loop-energy scalar + modal-stem array, lead-voice tap (O-simpleFM `FmVizAnalyzer` reuse).
Audio-thread copy-only (PERF-01). Confirmed running by end of Stage 2.

**OUT (deferred):**
- **Phase 2.4 — Waveguide string (`nice`, DSP-06):** dual-rail traveling-wave string + true
  pickup Position → **v1.1**. `stringModel` switch ships exposing KS only.
- Bow Speed control (v1.1); blow/tube resonator (v1.1+); elasto-plastic friction tier (out of scope).

---

## Binding Constraints (from ARCHITECTURE.md / memory — carry into research + plan)

- **Render-harness gate:** offline console app; **autocorrelation** single-note pitch probe
  (NOT spectral — the KS loop comb fools spectral probes; O-simpleGrain lesson). Assert measured
  f0 within a few cents at C1/C3/C5/C7 (target **±5 cents**). Compile under `JUCE_WEB_BROWSER=0`;
  `PluginEditor.cpp` dropped from harness sources; `createEditor` guarded `#if JUCE_WEB_BROWSER`.
- **Feedback safety:** loop feedback `g` hard-clamped `[0.80, 0.999]` (<1, QUAL-01/DSP-03).
  DC-blocker on String output. Bow friction output clamped; divisions epsilon-guarded.
- **Naming:** voice classes `StringVoice`/`ModalVoice` or one `PhysicalModelVoice` — NEVER
  `SamplerVoice`/`SamplerSound` (shadows `juce::` types). No bare `end`/`begin` APVTS param-ID
  C++ symbols under `using namespace ...ParamIDs`.
- **Real-time safety:** all DSP in `processBlock`; `juce::ScopedNoDenormals`; param reads via
  `getRawParameterValue()->load()`; no alloc/lock/FFT on audio thread; coefficient changes
  crossfaded ~64 samples (O-Lyrica shadow-filter) to avoid zipper clicks.
- **Modal = resonant biquad bank** driven by the exciter (NOT triggered sinusoids) → cross-driving
  (FUNC-04) for free. Custom direct-form arrays (O-Bassoon), NOT `juce::dsp::IIR::Filter` at 128-instance scale.
- **No oversampling** in v1.0 (harness audits Strike top-octave aliasing; add only if flagged).
- **Latency 0** (`setLatencySamples(0)` — getter non-virtual in JUCE 8). `getTailLengthSeconds() ≈ 5`.

---

## In-House Reference Implementations (direct reuse)

- **O-Lyrica** — `WaveguideString.h`/`PluckExciter.h`: KS loop, `OnePoleLPF`, fractional-delay
  tuning, `calculateFilterGroupDelay`, position comb, feedback-from-decay, shadow-filter crossfade.
- **O-Bells** — `BellVoice.h`: modal partial bank, ratio/`DECAY_MULTIPLIERS` tables, `StrikeExciter`.
- **O-Bowed** — `BowedStringVoice`: memoryless friction junction (Bow reference).
- **O-simpleFM** — `FmVizAnalyzer.h`, render-harness `tests/render-harness/main.cpp` (adapt
  DFT→autocorrelation), 16-voice `juce::Synthesiser` structure.

---

## Success Criteria (Stage 2 exit)

- [ ] Note-on produces a plucked tone that rings and decays; f0 within ±5 cents at C1/C3/C5/C7 (autocorrelation harness)
- [ ] Pluck=plucked, Strike=mallet, Bow=sustained (no decay while held); swapping exciter changes only attack/drive
- [ ] Material knob sweeps steel→nylon in one gesture; Damping+Decay visibly co-move; harder velocity = brighter/stronger
- [ ] Modal = inharmonic struck-bar/bell; each exciter drives Modal (cross-driving); Inharmonicity 0%≈bar, high%≈bell
- [ ] Feedback clamped <1 — bounded/finite at max Decay AND max Bow Force (QUAL-01); no click/DC/buzz/alias
- [ ] Viz taps running (loop-energy + modal-stem snapshots), audio-thread copy-only
- [ ] Render-harness re-buildable + ALL PASS; pluginval clean

---

## Open Items for Research Phase

1. Exact O-Lyrica `calculateFilterGroupDelay` + Thiran phase-delay compensation math to port (KS tuning accuracy).
2. STK `BowTable` memoryless friction curve coefficients + stable injection point into the KS junction.
3. O-Bells `DECAY_MULTIPLIERS` / per-mode T60→Q mapping to reuse for the 8-mode bank.
4. Render-harness autocorrelation pitch-probe implementation (port O-simpleGrain/O-simpleFM harness pattern).
5. Material macro curve constants (log-frequency cutoff endpoints, feedback lerp endpoints) tuned to read steel↔nylon.
