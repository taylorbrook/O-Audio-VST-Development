# Stage 2 (DSP) — CONTEXT

**Plugin:** O-simpleFM · **Stage:** 2 DSP · **Date:** 2026-06-20
**Source:** Auto-derived from locked contracts (ARCHITECTURE.md, ROADMAP.md, parameter-spec) — express mode, no interactive session. The DSP design was fully locked at Stage 0; nothing in this stage is open for re-decision.

## Goal

Turn the silent Stage 1 shell into a **polyphonic, MIDI-playable 2-operator phase-modulation synth** that renders audio through the locked DSP architecture, with a real-time-safe visualization tap feeding the (Stage 3) WebView spectrum + scope. After Stage 2 the plugin makes sound and is musically expressive; only the WebView UI remains.

## Scope (3 DSP sub-phases — ROADMAP)

- **Phase 2.1 — Core PM voice** (FUNC-01/02/05, DSP-01/02/03): `FMSound`, `FMSynthesiser` (16-voice cap), `FMVoice : juce::SynthesiserVoice`; `Operator.h` phase-accumulator + `fastSine` LUT; radian PM core (`carOut = fastSine(carPhase + I·modOut)`); ratio vs fixed-Hz modulator mode; index perceptual taper `I = 20·norm^1.7`; amp ADSR governing voice lifetime; velocity→amplitude; block param-push from processor; `SmoothedValue` on index/output; Multiplicative (or snap+ramp) smoothing on ratio.
- **Phase 2.2 — Mod env → index + feedback** (FUNC-03/04, DSP-05/06): independent mod ADSR; multiplicative `I_inst = baseIndex·((1−depth)+depth·modEnv)`, depth default 1.0; `velToIndex`; DX7 self-feedback `fbOut = fastSine(modPhase + coeff·½(prev1+prev2))` with history clamp + `isfinite` scrub + note-on reset; `feedback` 0–1 → coeff max ≈ π with `x^1.5` taper; `SmoothedValue` on feedback.
- **Phase 2.3 — Anti-aliasing + viz tap** (DSP-03, PERF-01, QUAL-01): key-tracked index ceiling (Carson) crossfaded; `juce::dsp::Oversampling` 2× polyphase-IIR always-on around the voice sum, latency reported via `setLatencySamples()`; output gain (`SmoothedValue` dB→lin, 20 ms) + block `isfinite` scrub; lock-free `AbstractFifo` ring tap (audio thread copy-only); message-thread editor `Timer` (30 Hz) → 4096 Blackman-Harris FFT (`performFrequencyOnlyForwardTransform`) + downsampled scope window (copy BEFORE in-place FFT).

## Hard Constraints (load-bearing — from ARCHITECTURE.md)

1. **Radians convention internally**, 1:1 with Bessel/Chowning math. Never mix with normalized-turns.
2. **PM, not true FM** — modulator added to phase after integration (stability under feedback).
3. **fastSine MUST `floor`-modulo wrap** before LUT lookup — the PM argument swings to many multiples of 2π at high index.
4. **Two-sample-average feedback** is mandatory (Tomisawa anti-hunting); clamp the *history*, scrub NaN at source, reset history on note-on. A single-sample feedback term screeches.
5. **Voice lifetime gated on AMP envelope only** — a long mod release must not keep a silent voice alive.
6. **Never reset carrier/modulator phase mid-note** (zipper/click). Reset only feedback history on note-on.
7. **Index ceiling applied AFTER modEnvToIndex scaling, BEFORE carrier modulation.**
8. **Oversampling wraps the summed voice render**, not per-voice (CPU).
9. **Audio thread is allocation-free and FFT-free** — copy-only into pre-allocated `AbstractFifo`; FFT on the message-thread Timer.
10. **Copy the scope window BEFORE the FFT** — `performFrequencyOnlyForwardTransform` overwrites its work buffer in place.
11. `juce::ScopedNoDenormals` at top of `processBlock`; ADSR `setSampleRate` in prepare BEFORE first `setParameters`.
12. `getLatencySamples()` is **non-virtual** in JUCE 8 — use `setLatencySamples()` in `prepareToPlay`, do NOT override.

## Parameter contract (unchanged from Stage 1 — 17 params)

All 17 APVTS IDs already exist in `OSimpleFM::ParamIDs` (PluginProcessor.h). Stage 2 *consumes* them — no new params, no ID changes. Mapping/skews per ARCHITECTURE.md Parameter Mapping table. Note: `modIndex` stored 0–20 (skew 0.3) then perceptual taper applied in DSP; `feedback`/`modSustain`/`ampSustain`/`modEnvToIndex`/`velToIndex` stored 0–1; ADSR times 0.001–5 s (skew 0.35); `outputLevel` −60→0 dB.

## Reference implementations (read in-repo)

- **O-Bassoon** — `SynthesiserVoice`/`Synthesiser`/`Sound` skeleton; JUCE 8 custom `prepareToPlay` (no virtual); `setExpression`/block param-push. **Voice architecture source.**
- **O-Marimba** — `WaveformFifo` atomic-overwrite ring + 30 Hz editor Timer + `emitEventIfBrowserIsVisible`. **Primary viz/scope reference.**
- **O-Prism / O-MultiBandCompressor** — `dsp::FFT` + `WindowingFunction`. (We intentionally move FFT to the message thread vs MBC's processBlock FFT.)
- **O-AnalogEQ** — cross-platform WebView2 CMake + level push (Stage 3 relevance).

## Out of scope (deferred to v1.1)

Non-sine operator waveforms (`carWave`/`modWave`, DSP-04), 4× oversampling, `fineCents`, `masterTune`. The WebView UI itself is Stage 3 — Stage 2 keeps the GenericAudioProcessorEditor but adds the viz **data path** (FIFO + Timer + emit) so Stage 3 only has to draw.

## Success criteria (from ROADMAP test criteria)

- Loads in DAW as an instrument; MIDI routes; plays polyphonically without crash.
- Index 0 = pure sine; raising index audibly adds sidebands (no zipper).
- Integer ratio = harmonic; non-integer = inharmonic/bell.
- Fixed-mode modulator holds constant Hz while carrier tracks pitch; Ratio mode key-tracks.
- Amp ADSR shapes notes; long sustains hold; release tails clean; no stuck/silent voices.
- Mod env sweeps timbre over a held note independent of amplitude; depth 1.0 + sustain 0 → pure-sine tail; carrier null reachable near I≈2.405.
- Feedback enriches modulator → sawtooth/noise smoothly; no screech/limit-cycle, no NaN.
- No clicks on note-on/off; no denormal CPU stalls; no audible aliasing across index/feedback/pitch (2× OS + ceiling).
- Viz tap delivers spectrum + scope frames to the editor (validated headless via auval render + frame counters; visual confirm at Stage 3).

## Decisions deferred to mockup/Stage 3 (non-blocking for DSP)

- Read-only `Δf = I·f_m` secondary readout (UI only).
- Optional on-screen keyboard MIDI injection (`nice`-only).
