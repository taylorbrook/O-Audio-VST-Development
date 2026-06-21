# O-simpleFM — Stage 0 Context (Discuss Findings)

**Date:** 2026-06-20
**Stage:** 0 (Ideation / Research & Planning) — complete

This file captures the discuss-phase decisions, constraints, and rationale that shaped `research/ARCHITECTURE.md` and `ROADMAP.md`. It is the narrative companion to those documents.

---

## What this plugin is

A deliberately simple **2-operator FM (really phase-modulation) synthesizer built for teaching**. Pedagogy is the north star: every gesture must have a visible/audible consequence within five minutes, no manual. The DSP is intentionally minimal (one carrier, one modulator, ratio, index, feedback, two ADSRs); the *real* engineering weight is the stable feedback + anti-aliasing + lock-free dual-domain real-time visualization stack.

---

## Key decisions (and why)

1. **Phase modulation, not true FM.** PM adds the modulator after phase integration, so DC offset (feedback/asymmetry/bias) is a fixed inaudible phase offset rather than integrated pitch drift. This is what makes feedback stable. Every commercial "FM" synth is PM. Commit to a **radians** phase convention internally (1:1 with the Bessel/Chowning math the plugin teaches) — never mix with normalized-turns.

2. **Modulation index = raw radian index `I`, 0–20, perceptual taper, displayed linearly.** It *is* the Bessel argument; maps 1:1 to the sideband theory. Carrier null at I≈2.405 is a marquee teaching annotation. Offer `Δf = I·f_m` as a secondary read-only readout. (Chose this over Hz-deviation or modulator-level exposures for transparency.)

3. **Mod envelope → index is multiplicative, depth default 1.0.** `I_inst = baseIndex·((1−depth)+depth·modEnv)`. At depth 1.0 + sustain 0, the tail is pure sine and the carrier null is reachable — the coupling *is* the teaching payload. Amp ADSR is fully independent and governs voice lifetime.

4. **DX7 self-feedback with two-sample average.** The average is the load-bearing anti-hunting filter (Tomisawa, US Pat 4,249,447) — without it the modulator hunts into a Nyquist limit cycle (screech). Clamp the **history**, scrub NaN at source, reset on note-on. Max coeff ≈ π with an `x^1.5` taper so the bottom 60% stays gentle.

5. **Polyphony = 16 voices.** Suite norm; comfortable for chordal classroom use; cheap at 2× oversampling. Voice lifetime gated on **amp** envelope activity only (so a long mod release can't keep a silent voice alive).

6. **Anti-aliasing floor = sine LUT + key-tracked index ceiling + 2× polyphase-IIR oversampling (always-on).** For a teaching tool, transparency and CPU predictability beat brute oversampling. Index ceiling (`(0.9·Nyq−fc)/fm − 1`) is near-free and textbook. Escalate to 4× + band-limited additive wavetables only when non-sine operators are enabled (opt-in `should`). **Do NOT use PolyBLEP** — it doesn't compose with hard FM. This is the single HIGH-risk area; ARCHITECTURE.md documents a tiered fallback.

7. **Visualization is real-time-safe by construction.** Audio thread does copy-only into a pre-allocated `juce::AbstractFifo` ring (no alloc, no FFT, no locks). The editor `Timer` (30 Hz) runs the FFT (**4096 / Blackman-Harris** for crisply separated sidebands) and builds the scope frame on the message thread, then pushes via `emitEventIfBrowserIsVisible`. Intentional deviation from O-MultiBandCompressor (which FFTs in `processBlock`). Critical gotcha: `performFrequencyOnlyForwardTransform` overwrites in place — copy the scope window before the FFT.

8. **Architecture reuses the suite.** `juce::Synthesiser` + custom `FMVoice : SynthesiserVoice` (O-Bassoon skeleton); APVTS + `NormalisableRange`/skews + WebView relays (O-Prism); `WaveformFifo` + native-fn scope (O-Marimba); cross-platform WebView2 CMake (O-AnalogEQ).

---

## Constraints carried into implementation

- **PERF-01:** zero allocations in `processBlock`; lock-free FIFO feeds viz.
- **COMPAT-02:** `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder(tempDir)` on Windows, or the UI blanks (silent IE fallback).
- **QUAL-01:** no clicks/aliasing/instability across the full range including high index/feedback — the AA stack + denormal/NaN hygiene must hold.
- **JUCE 8:** `SynthesiserVoice` has no virtual `prepareToPlay`; `getLatencySamples()` is non-virtual (use `setLatencySamples()`).
- **CMake:** `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` or the plugin is silent in the DAW (critical-pattern #22).

---

## Open questions for the user (non-blocking — confirm at mockup)

1. **DSP-06 optional params** — adopt or drop `modFixedMode`/`modFixedHz` (fixed-freq modulator), `fineCents`, `masterTune`? They're documented but excluded from the core 17-param count. Recommendation: defer `modFixedMode` + tuning to v1.1 unless the mockup wants them; keep the synth lean for teaching.
2. **Non-sine operator waveforms (DSP-04)** — ship in v1.0 or defer? They're a `should` and carry the heaviest AA cost (band-limited wavetables + 4× OS). Recommendation: ship **sine-only** at v1.0 if schedule-constrained; add waveforms as a fast follow.
3. **Index ceiling visibility** — should the key-tracked dulling-at-high-pitch be surfaced to the student (it's itself teachable) or silent? Recommendation: surface it as an optional "anti-alias" annotation in the spectrum.

None of these block Stage 1.

---

## Status

- ARCHITECTURE.md: complete (`research/ARCHITECTURE.md`)
- ROADMAP.md: complete, complexity 5.0 (raw 11.0), staged
- Ready for Stage 1 (Foundation).
