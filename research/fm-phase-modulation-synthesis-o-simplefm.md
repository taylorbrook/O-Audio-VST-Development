---
title: "FM / Phase-Modulation Synthesis — Research for O-simpleFM"
created: 2026-06-20
last_verified: 2026-06-20
juce_version: "8.0.9"
summary: "Level-3 deep research on 2-operator phase-modulation synthesis for a pedagogical FM synth. Covers PM-vs-FM math, modulation-index/Bessel semantics, DX7 self-feedback anti-hunting, anti-aliasing strategy, and JUCE 8 Synthesiser/FFT/oversampling architecture grounded in the Ouaricon suite."
domain: dsp
type: algorithm
keywords:
  - fm-synthesis
  - phase-modulation
  - 2-operator
  - modulation-index
  - bessel-sidebands
  - dx7-feedback
  - anti-aliasing
  - juce-synthesiser
  - lookup-table
  - oversampling
  - carson-bandwidth
  - chowning
stages: [0, 2]
agents: [dsp, research]
---

# FM / Phase-Modulation Synthesis — Research for O-simpleFM

**Plugin:** O-simpleFM (pedagogical 2-operator FM synth)
**Research level:** Level 3 (parallel deep investigation, 3 subagents)
**Date:** 2026-06-20
**Confidence:** HIGH (strong cross-source agreement; CCRMA/Chowning + DX7 reverse-engineering + JUCE suite conventions)

> Feeds Stage 0 planning (`/plan O-simpleFM`). Covers core DSP math, stability/anti-aliasing,
> and JUCE 8 implementation architecture grounded in the existing Ouaricon suite.

---

## TL;DR — Decisions to carry into planning

1. **Build phase modulation (PM), not true FM.** Every commercial "FM" synth (DX7, Operator, FM8) is PM. PM avoids the DC-offset → pitch-drift failure that makes true FM feedback/cascade unstable, and decouples the modulation index from modulator frequency.
2. **Expose the raw modulation index `I`** (the radian phase-deviation multiplier), range **0–20**, perceptual/skewed taper, **displayed linearly**. Annotate the first **carrier null at I ≈ 2.405** as a marquee teaching moment ("watch the fundamental vanish").
3. **C:M ratio** with optional **integer-snap** (applied at the read site, not the param range). Integer = harmonic, irrational = inharmonic/bell.
4. **Modulator ADSR scales the index multiplicatively** (`I_inst = I · ((1−depth) + depth·modEnv)`), default depth = 1.0. This is the headline expressive feature; amp ADSR is independent and governs voice lifetime.
5. **DX7-style feedback** = `sin(modPhase + coeff·(prev1+prev2)/2)` — the two-sample average is the load-bearing anti-hunting filter, not optional. Max coeff ≈ **π radians** with a gentle taper; NaN-scrub + clamp history every sample.
6. **Anti-aliasing stack:** sine LUT operators + **key-tracked index ceiling** (Carson's rule) + **2× oversampling always-on** is enough for QUAL-01 at sine defaults. Escalate to **4× + band-limited additive wavetables** only when the user enables saw/square/tri (do **not** use PolyBLEP — it doesn't compose with hard FM).
7. **Sine via `juce::dsp::LookupTableTransform`**, 1024 pts, linear interp (~97 dB floor), with explicit `floor`-modulo phase wrap (the PM argument swings far outside [0,2π) at high index).
8. **Architecture = `juce::Synthesiser` + `FMVoice : SynthesiserVoice`**, 16 voices, two `juce::ADSR`. Read APVTS once per block in the processor → push to voices (the O-Bassoon `setExpression` pattern). `SmoothedValue` on ratio/index/feedback.
9. **Visualizations real-time-safe:** audio thread only copies output into a lock-free `AbstractFifo` ring (no alloc, no FFT); editor `Timer` (30 Hz) runs the FFT (**4096 / Blackman-Harris**) + builds scope window + pushes both to WebView via `emitEventIfBrowserIsVisible`.

⚠️ **Cross-report consistency gotcha — pick ONE phase convention:** the math report uses **radians** (`phase ∈ [0,2π)`, `sin(carPhase + I·modOut)`, feedback max = π). The architecture skeleton uses **normalized turns** (`phase ∈ [0,1)`, divides index/feedback by 2π). Both are correct but **must not be mixed**. If you adopt the turns convention, feedback max becomes **0.5 turns** (= π rad), and index is applied as `I·modOut/(2π)`. Recommend committing to **radians** internally for 1:1 correspondence with the Bessel/Chowning math the plugin teaches.

---

## 1. Core DSP — PM math & algorithm

### PM vs FM
For a sinusoidal modulator, FM and PM produce identical spectra (90° modulator phase shift aside). The implementation difference is *when* the modulator is applied relative to phase integration:

```cpp
// (a) TRUE FM — modulator added to frequency BEFORE integration (fragile)
double instFreq = fc + index * fm * modOut;
carrierPhase   += 2.0 * M_PI * instFreq / fs;
double out      = std::sin(carrierPhase);

// (b) PHASE MODULATION — modulator added to phase AFTER integration  ← BUILD THIS
carrierPhase   += 2.0 * M_PI * fc / fs;
double out      = std::sin(carrierPhase + index * modOut);
```

**Why PM:** in true FM the modulator is *integrated* by the phase accumulator, so any DC component (asymmetric waveform, feedback, numerical bias) integrates into a steadily accumulating phase ramp = audible pitch drift that worsens with depth and cascading. PM adds the modulator *after* integration → DC produces only a fixed (inaudible) phase offset. This is what makes feedback and cascaded operators stable. (Chowning 1973; musicdsp.org #160; CCRMA/J.O. Smith.)

### Drop-in per-sample core (radians convention)
```cpp
// Per voice, per sample. PM engine. Phases in radians, wrapped [0,2π).
double fm     = fc * ratio;                                   // C:M ratio
double modEnv = modEnvelope.getNextSample();                  // ADSR ∈ [0,1]
double I_inst = baseIndex * ((1.0 - envDepth) + envDepth * modEnv);  // mod env → index

double modOut = std::sin(modPhase + feedback * 0.5*(prev1+prev2)); // self-feedback (see §2)
prev2 = prev1; prev1 = modOut;
modPhase += 2.0*M_PI*fm/fs;  if (modPhase >= 2*M_PI) modPhase -= 2*M_PI;

double carOut = std::sin(carPhase + I_inst * modOut);         // index = radian phase-dev
carPhase += 2.0*M_PI*fc/fs;  if (carPhase >= 2*M_PI) carPhase -= 2*M_PI;

double sample = carOut * ampEnvelope.getNextSample();         // independent amp ADSR
```

### Modulation-index semantics
Three equivalent exposures: (a) **raw index I** (radian multiplier — *is* the Bessel argument), (b) peak deviation Δf in Hz (`I = Δf/fm`), (c) modulator output level. **Use (a)** — it maps 1:1 to the spectrum theory the plugin teaches. Optionally show `Δf = I·fm` as a read-only secondary readout. Range **0–20**, perceptual taper, displayed linearly:
```cpp
double I = 20.0 * std::pow(k, 1.7);   // k = normalized knob; finer control low-end
```

### Bessel sidebands (practical)
PM of a sine by `I·sin(ω_m t)` expands (Jacobi–Anger) to **sideband pairs at `f_c ± n·f_m`, amplitudes `Jₙ(I)`**:
- Significant sideband pairs ≈ **I + 1**.
- **Carson's rule:** `BW ≈ 2·(I+1)·f_m` (≈98% power) → grounds analyzer scaling AND the aliasing budget; highest significant component ≈ `f_c + (I+1)·f_m`.
- **Carrier nulls** (J₀ zeros) at **I ≈ 2.405, 5.520, 8.654…** — the fundamental fully disappears. Annotate I≈2.405; it's the most memorable FM demo.

### C:M ratio → harmonic vs inharmonic
Sidebands land at `f_c·(1 ± n·ratio)`. Integer/simple-rational C:M → integer multiples of a common fundamental → **harmonic**. Irrational C:M → **inharmonic/bell**. Negative-frequency lower sidebands fold through 0 Hz with **phase inversion** (enriches low ratios like 1:1 → sawtooth-ish).

| C:M | Character |
|-----|-----------|
| 1:1 | Sawtooth-like, full harmonic series |
| 1:2 | Odd-harmonic, hollow / clarinet-ish |
| 2:1 | Brighter harmonic, brass/reed |
| 3:2 | Gapped harmonic, nasal/voweled |
| 1:4 | Sparse odd-emphasis, woody/clarinet |
| 1:√2 (1:1.414) | Inharmonic, metallic |
| 1:3.5 | Canonical FM tubular bell |

### Classic patch recipes (preset tour — FUNC-06)
ADSR = (A, D, S 0–1, R). **Star** = the concept each isolates.

| Preset | C:M | Index | Mod ADSR | Amp ADSR | Star |
|--------|-----|-------|----------|----------|------|
| **E-Piano (Rhodes)** | 1:1 (or 1:2) | ~2–3 | 0, 0.4, 0.0, 0.2 | 0.002, 1.5, 0.3, 0.4 | Mod env decays index → bright ping + sine body |
| **Tubular Bell** | 1:3.5 | ~4–6 | 0, 2.5, 0.0, 2.0 | 0.001, 5, 0.0, 3 | Inharmonic ratio |
| **Brass** | 1:1 | swell 0→~5 | 0.1, 0.2, 0.8, 0.2 | 0.1, 0.1, 0.9, 0.2 | Index env tracks amp env (brightness = loudness) |
| **Clarinet** | 1:2 (or 1:4) | ~1.5–2 static | 0.15, 0.1, 1.0, 0.1 | 0.15, 0, 1.0, 0.15 | Odd-harmonic ratio + low steady index |
| **Clang Bell** | 1:1.414 | ~8–12 | 0, 0.5, 0.0, 1.5 | 0.001, 4, 0.0, 2 | High index → wide BW (aliasing demo) + max inharmonicity |

### Mod env → index (headline feature)
**Multiplicative, default.** `I_inst = baseIndex · modEnv` → endpoints unambiguous (sustain 0 ⇒ pure-sine tail; knob = max brightness; carrier-null reachable). Offer an **"Env → Index Amount"** blend (`baseIndex·((1−depth)+depth·modEnv)`) if a brightness floor is wanted. Additive muddies the "knob = max brightness" model — avoid as default.

---

## 2. Feedback, aliasing & stability

### Modulator self-feedback (DX7)
```cpp
float avg   = 0.5f * (fbPrev1 + fbPrev2);                 // two-sample average
float fbOut = sineLUT(modPhase + feedbackCoeff * avg);
if (!std::isfinite(fbOut)) fbOut = 0.0f;                  // kill NaN at source
fbOut = juce::jlimit(-1.0f, 1.0f, fbOut);                 // clamp HISTORY (not just out)
fbPrev2 = fbPrev1; fbPrev1 = fbOut;
```
The two-sample average is Tomisawa's **anti-hunting filter** (US Patent 4,249,447). A single-sample feedback term hunts into a Nyquist-rate `+x,−x,…` limit cycle (screech/instability); averaging is a cheap LP that suppresses it while keeping the musical low spectrum. Rising feedback drives the modulator sine → sawtooth → noise/chaos. Map UI 0–100% → coeff with **max ≈ π radians** and a square/`x^1.5` taper so the bottom 0–60% stays gentle. Reset `fbPrev1=fbPrev2=0` on note-on.

### Aliasing — the #1 quality risk (QUAL-01)
Sidebands above Nyquist fold back inharmonically (metallic grit). Driven combinatorially by **index × ratio × pitch (× feedback)**. Aliasing on when `f_c + (I+1)·f_m > fs/2`. Examples (44.1k): C:M 1:1 @440 Hz aliases around I≈40+; C:M 1:8 @440 aliases at I≈4; same patch two octaves up quadruples `f_highest`. Feedback→sawtooth aliases readily at high pitch even at modest settings.

### Anti-aliasing strategy (recommended stack)
1. **Key-tracked index ceiling** (nearly free, per-note): `indexCeil = (0.9·Nyquist − f_c)/f_m − 1`, `effIndex = min(userIndex, indexCeil)`. Textbook CCRMA fix. Caveat: it's a timbral change (bright patches dull when played high) — crossfade/smooth the ceiling.
2. **2× oversampling, polyphase-IIR (low-latency)**, always-on, around the whole voice render — the catch-all for residual index/feedback aliasing. `juce::dsp::Oversampling<float>` with `filterHalfBandPolyphaseIIR`.
3. **Non-sine operators:** generate saw/square/tri as **additive band-limited wavetables (one table per octave band)**, read by phase, and **bump to 4× oversampling** while active. **Do NOT use PolyBLEP/BLIT** — their discontinuity correction assumes a steady phase increment, which hard FM violates; they don't compose with FM.

> For a pedagogical synth, **sine defaults + index ceiling + 2× OS** satisfies QUAL-01 with minimal CPU. The harder waveforms are an opt-in `should` (DSP-04) with a defined escalation.

### Sine generation
Use **`juce::dsp::LookupTableTransform`**, single-cycle, **1024 pts, linear interp** (~97 dB SNR — inaudible noise floor; cubic unnecessary). The PM argument can be tens of × 2π at high index → **must `floor`-modulo wrap before lookup**:
```cpp
inline float fastSine(float phase) noexcept {
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    phase -= twoPi * std::floor(phase / twoPi);   // essential for large index swings
    return sineTable[phase];
}
```
Reserve `std::sin` for the one-time table fill only.

### Stability hygiene
- `juce::ScopedNoDenormals` at top of `processBlock` (denormals stall 10–100× in decaying feedback loops).
- Per-voice NaN guard + history clamp (above); block-level `std::isfinite` scrubber after summing voices as insurance against suite-wide NaN poisoning.
- **Zipper noise:** `SmoothedValue` on index/feedback (~20 ms linear); ratio uses **Multiplicative** smoothing OR integer-snap with short ramp (linear smoothing of a frequency factor glides pitch oddly). Never reset phases mid-note.

---

## 3. JUCE 8 architecture (grounded in the suite)

**Conventions confirmed by reading:** `O-Bassoon` (voice/synthesiser/sound + `setExpression` block-param push; `setSampleRate` before `setParameters`; `SynthesiserVoice` has no virtual `prepareToPlay` in JUCE 8 — use a custom `prepare()` calling `setCurrentPlaybackSampleRate`), `O-Prism` (APVTS `ParameterID{id,1}` + `NormalisableRange(min,max,step,skew)`, skews 0.35 time / 0.3 depth; WebView relay/attachment order; resource provider receives bare paths), `O-MultiBandCompressor`/`O-Prism` (FFT), `O-GrainScatter` (triple-buffered viz snapshot + `emitEventIfBrowserIsVisible` + 30 Hz timer).

### File layout
```
Source/
  PluginProcessor.{h,cpp}   APVTS, Synthesiser owner, FFT/scope FIFO, processBlock
  PluginEditor.{h,cpp}      WebBrowserComponent, relays/attachments, Timer push
  FMSound.h                 trivial SynthesiserSound (copy BassoonSound.h)
  FMSynthesiser.h           Synthesiser subclass, 16-voice cap (copy BassoonSynthesiser.h)
  FMVoice.{h,cpp}           the FM voice
  FMParamIds.h              param-id constants (PrismParamIds.h convention)
  Operator.h                phase-accumulator helper (header-only)
  ui/public/{index.html, css/, js/app.js, js/juce/...}
```

### Voice
`FMVoice : juce::SynthesiserVoice` — carrier + modulator phase accumulators, two `juce::ADSR` (amp on output, mod scaling index), `fbPrev1/2` history, `SmoothedValue` for ratio/index/feedback. Custom `prepare(sr)` calls `setCurrentPlaybackSampleRate` + `ampEnv/modEnv.setSampleRate` (**before** `setParameters`) + smoothing resets. Processor reads APVTS once/block and calls `voice->setParams(...)`. **Voice lifetime = `ampEnv.isActive()`** (so a long mod release won't keep a silent voice alive). **16 voices** (suite norm; brief's 8–16). velocity→amplitude always; **velocity→index** as opt-in param (default 0).

### APVTS parameters
Core: `ratio` (0.5–16, +`ratioSnap` bool), `index` (0–20, skew 0.3), `feedback` (0–1, skew 0.5), mod ADSR ×4 + amp ADSR ×4 (times 0.001–5 skew 0.35; sustains 0–1), `carWave`/`modWave` choice (Sine/Tri/Saw/Square), `output` dB. Additions (DSP-06): `modEnvToIndex` (default **1.0** — headline feature on), `velToIndex` (default 0), `modFixedMode`+`modFixedHz`, `fineCents`, `masterTune`. Integer-snap at read site: `if (snap) ratio = round(ratio)`. Cache `apvts.getRawParameterValue()` atomics in `prepareToPlay`.

### Visualization data path (PERF-01)
- **Audio thread:** mono-sum rendered output → `juce::AbstractFifo`-backed `std::vector<float>` ring. **No alloc, no FFT.**
- **Editor Timer (30 Hz):** copy a scope window, then run `juce::dsp::FFT` (**FFT_ORDER 12 = 4096** for crisp discrete sidebands) with **Blackman-Harris** window (~−92 dB sidelobes keep adjacent sidebands distinct — Hann is blurrier), `performFrequencyOnlyForwardTransform`, map to log-freq dB bins with rise-fast/fall-slow decay smoothing, then `emitEventIfBrowserIsVisible("spectrumUpdate"/"scopeUpdate", json)`.
- ⚠️ `performFrequencyOnlyForwardTransform` **overwrites the work buffer in place** — copy the scope window **before** the FFT call.
- This is the one intentional deviation from O-MultiBandCompressor (which runs FFT in `processBlock`): move FFT to the message thread per the brief.

### WebView wiring
Member order is load-bearing (Prism): **relays first, WebView second, attachments last.** Knobs use `WebSliderRelay`/`WebComboBoxRelay`/`WebToggleButtonRelay` + matching attachments (auto two-way APVTS↔JS). Live viz is push-only via events. JS: `import * as Juce from './js/juce/index.js'` for `getSliderState`/native functions; `window.__JUCE__.backend.addEventListener` for spectrum/scope events. Minimal native-fn surface (tooltips can be a JS const; presets via suite preset manager or relay `setNormalisedValue`). Windows: `withUserDataFolder(tempDir)`.

### CMake
`juce_add_plugin(... IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_WEB_BROWSER TRUE NEEDS_WEBVIEW2 TRUE FORMATS VST3 AU Standalone ...)`; link `juce_dsp` (FFT + WindowingFunction) + `juce_gui_extra` (WebBrowserComponent); `target_compile_definitions(... JUCE_WEB_BROWSER=1 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 ...)` (COMPAT-02). Build: `ninja O-simpleFM_VST3 O-simpleFM_AU` then `./scripts/build-and-install.sh O-simpleFM`; verify `auval -a | grep -i simplefm`.

---

## Requirement coverage map

| Req | Covered by |
|-----|-----------|
| FUNC-01/02 | §3 voice + 16-voice Synthesiser |
| FUNC-03 | §2 DX7 two-sample-average feedback, max≈π, NaN guard |
| FUNC-04 | §1 multiplicative mod-env→index (default depth 1.0), independent of amp env |
| FUNC-05 | §3 amp ADSR governs voice lifetime |
| FUNC-06 | §1 preset recipes |
| DSP-01 | §1 C:M ratio table + integer-snap |
| DSP-02 | §1 index 0–20 + §2 SmoothedValue (no zipper) |
| DSP-03 | §1 PM core + §2 feedback averaging |
| DSP-04 | §2 band-limited wavetables + 4× OS for non-sine |
| DSP-05 | §3 integer-snap at read site |
| DSP-06 | §3 modEnvToIndex / velToIndex / fixed-freq / detune / tuning |
| UI-01/02 | §3 4096/Blackman-Harris FFT + scope, lock-free |
| UI-03/04/05 | §3 JS routing diagram / tooltips / single-page |
| PERF-01 | §3 audio thread copy-only, FFT on timer |
| QUAL-01 | §2 index ceiling + 2× OS + LUT + denormal/NaN hygiene |
| COMPAT-02 | §3 CMake WebView2 statics |

---

## Sources
- Chowning, "Synthesis of Complex Audio Spectra by Means of Frequency Modulation," JAES 1973 — ccrma.stanford.edu
- J.O. Smith III (CCRMA), "Sinusoidal Frequency Modulation (FM)"; "An Introduction to FM" (Carson/index cap)
- musicdsp.org #160 "Phase modulation Vs. Frequency modulation II"
- CMU/Dannenberg "FM Synthesis"; CCRMA CLM FM tutorial; UVic MU307 FM tutorial (brass/clarinet/bell recipes)
- Carson bandwidth rule (Wikipedia / All About Circuits)
- ajxs.me "Yamaha DX7 Technical Analysis"; righto.com "DX7 chip reverse-engineering pt.4"; US Patent 4,249,447 (Tomisawa, anti-hunting)
- EarLevel "Wavetable signal-to-noise ratio"; JUCE docs `dsp::Oversampling` / `LookupTableTransform` / `ScopedNoDenormals`; JUCE forum (antialiasing a synth, SmoothedValue zipper)
- DX7 patch refs: deepsonic "Basic FM Synthesis", bobbyblues DX7 examples
- Suite conventions: O-Bassoon, O-Prism, O-MultiBandCompressor, O-GrainScatter (read in-repo)
