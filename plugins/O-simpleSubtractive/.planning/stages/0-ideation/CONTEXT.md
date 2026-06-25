# Stage 0 (Ideation / Research & Planning) — CONTEXT

**Plugin:** O-simpleSubtractive
**Date:** 2026-06-25
**Outcome:** Stage 0 complete — ARCHITECTURE.md + ROADMAP.md produced. Ready for Stage 1 (Foundation).

---

## What this plugin is

Pedagogical subtractive synthesizer — the subtractive sibling of O-simpleFM and O-simpleAdditive. The irreducible spine: **oscillator (+sub +noise) → multimode filter → VCA**, with **two independent ADSRs** (one on the filter cutoff, one on the amp). North star: a student reaches the "oh, that's how subtractive works" moment in five minutes, and can build + save a bass and a lead. WebView UI, JUCE 8, 16-voice poly / mono / legato.

---

## Resolved decisions (the discuss → research payoff)

### 1. Filter topology — RESOLVED: custom Cytomic ZDF state-variable filter (NOT Moog ladder)
The brief's headline open question. **Decision: a custom zero-delay-feedback TPT state-variable filter** (Andrew Simper / Zavalishin), cascade ×2 for 24 dB, 1-pole for 6 dB, with a **tanh nonlinearity** for self-oscillation.

Why SVF over ladder:
- **All four modes from one structure** (LP/BP/HP simultaneous; Notch = input − k·BP). One filter, four shapes = the pedagogical narrative.
- **Exact closed-form magnitude response** (`Ω = tan(π·f/fs)/g`, prewarped → 1.0 at cutoff with no warping error) that matches the running digital filter sample-for-sample → the headline "filter-curve-over-spectrum" visual is truthful by construction (QUAL-02). **This is the deciding factor for a teaching tool.**
- **Clean slope story:** 6/12/24 dB = 1/2/4 poles of the same SVF.

Why not Moog ladder (`juce::dsp::LadderFilter` — it DOES exist in `juce_dsp/widgets/`): fundamentally a lowpass; HP/BP need the Oberheim tap-and-sum trick; JUCE exposes only fixed `LPF/HPF/BPF 12/24` modes with **no notch**; per-slope/per-mode magnitude curves are less clean for the headline figure.

Built-in `juce::dsp::StateVariableTPTFilter` is **linear → cannot self-oscillate** (DSP-03 is a MUST), so it's retained only as **Fallback A** (linear LP/BP/HP + a faked self-osc sine for that one preset).

### 2. Self-oscillation + gain compensation
`tanh` on the resonant integrator path bounds the would-be unbounded pole-on-axis growth into a **clean limit-cycle sine at cutoff**; the nonlinearity IS the gain compensation (resonance sweeps can't blow up). Plus a gentle resonance-dependent make-up trim so sweeps don't jump in level.

### 3. Anti-aliasing — PolyBLEP (NOT wavetable mipmaps, NOT oversampling)
PolyBLEP (saw/square) + polyBLAMP (triangle) + LUT sine. Composes correctly here because the subtractive osc runs at a **steady phase increment** (the exact case PolyBLEP assumes) — unlike O-simpleFM where hard phase-modulation forced rejection of PolyBLEP. Near-zero CPU, zero latency. A teaching tool must not buzz at high notes.

### 4. Cutoff key-tracking — ADD `keyTrack` param (default 0%)
`fcEff *= 2^(keyTrack·(note−60)/12)`. At 100% the self-oscillating filter plays chromatically in tune (the "Self-Osc Sine" lesson). Default 0% keeps the first sweep lesson simple (cutoff = absolute Hz).

### 5. Bipolar filter-env → cutoff in OCTAVES (not linear Hz)
`fcEff *= 2^(filterEnvAmount·eF·ENV_OCT)`, `filterEnvAmount ∈ [−1,+1]`, `ENV_OCT ≈ 6–8`. Exponential = constant perceptual sweep at any base cutoff; positive opens, negative closes.

### 6. Dual independent ADSRs, zipper-free
Two `juce::ADSR` per voice (filter→cutoff, amp→VCA+lifetime). Per-sample sampling; `SmoothedValue` on UI cutoff/res; per-sample (or ≤16-sample) `g` recompute. Independence is the teaching payload (UI-02 dual display).

### 7. Voice management — `juce::Synthesiser` + processor-side MonoController
Reuse the proven `Synthesiser`/`SynthesiserVoice` skeleton (O-simpleFM/O-Bassoon). Poly is free; a thin processor-side held-note-stack controller adds Mono (retrigger) / Legato (slur, no retrigger) + multiplicative glide. **This is the one genuinely-new area vs O-simpleFM (poly-only)** → MEDIUM risk; fallback = Poly+Mono in v1.0, refine Legato in polish.

### 8. Headline visualization pipeline (lock-free)
Audio thread copy-only into `VizRing` + `displayCutoffHz`/`displayK` atomics (from the lead voice). Message-thread Timer (30 Hz): FFT spectrum (Blackman-Harris, 4096) + closed-form filter curve (same g/k as audio) + scope + dual-ADSR values → `emitEventIfBrowserIsVisible`. Reuses O-simpleFM `VizRing`/`FmVizAnalyzer` near-verbatim. FFT on message thread, never the audio thread (PERF-01).

### 9. Parameter resolution (draft open items)
- **ADD:** `keyTrack`.
- **DEFER to v1.1:** `velToFilterEnv`/`velToAmp` as params (velocity→amp is always-on, non-param), PWM (`pulseWidth`; square fixed 50%), master tune/octave.
- **Final v1.0 core = 20 parameters** (see ARCHITECTURE.md Parameter Mapping).

---

## Complexity & strategy
- **Tier:** 4 (synth + MIDI + oscillators) escalated toward 6 by first-class real-time viz → MODERATE→DEEP.
- **Complexity score: 5.0** (capped; raw 11.0 = params 2.0 + 5 algorithms + 4 features). Mirrors O-simpleFM.
- **Strategy: phased** — Stage 2 DSP in 3 phases, Stage 3 GUI in 3 phases.
- **Highest risk:** self-oscillating multimode SVF (~50% of risk) → build linear filter + curve match FIRST, add self-osc second.

## Constraints honored
- JUCE 8.0.9; CMake+Ninja; macOS VST3+AU + Windows VST3.
- `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE`; `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`.
- One binary-data target in v1.0; distinct `NAMESPACE` if a 2nd is added (O-simpleGrain lesson).
- RT-safe processBlock (no alloc/lock/file-IO); lock-free viz handoff.
- `setLatencySamples(0)` (zero added latency, no oversampling); `getLatencySamples()` non-virtual.

## Stage-2 correctness gate
Port O-simpleFM offline DSP render-harness: per-mode magnitude (closed-form vs measured), high-key aliasing budget, self-osc-in-tune (keyTrack=100%), dual-ADSR independence, note lifecycle.

## Sibling references (read in-repo)
O-simpleFM (primary template — voice/ADSR/VizRing/CMake/harness), O-simpleAdditive (WebView template + QUAL-02 discipline), O-Prism (SVF usage notes), O-Bassoon (JUCE 8 voice patterns), O-simpleGrain (BinaryData namespace + harness).

## Files produced this stage
- `plugins/O-simpleSubtractive/.planning/research/ARCHITECTURE.md`
- `plugins/O-simpleSubtractive/.planning/ROADMAP.md`
- `plugins/O-simpleSubtractive/.planning/stages/0-ideation/CONTEXT.md` (this file)
- `plugins/O-simpleSubtractive/.planning/STATUS.md` (updated → stage 0 complete)
