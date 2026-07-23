# O-ReverseDelay - Implementation Plan

**Date:** 2026-07-23
**Complexity Score:** 5.0 (Complex — capped)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 10 parameters (10/5 = 2.0, capped at 2.0) = **2.0**
- **Algorithms:** 4 DSP components = **4**
  - Reverse Grain Engine (custom: capture ring + scheduler + pool + Hann OLA)
  - In-loop Damping Filters (2nd-order Butterworth HP/LP, ArrayCoefficients)
  - Feedback Stability Stage (gain + tanh soft-clip + non-finite guard)
  - Width / equal-power Mix output stage
- **Features:** 1 point
  - Feedback loop (+1)
  - (No FFT, no multiband, no modulation system, no external MIDI)
- **Total:** 2.0 + 4 + 1 = 7.0 → **capped at 5.0**

Score ≥ 3.0 → **phased implementation** with per-phase commits and test criteria.

---

## Stages

- Stage 0: Research & Planning ✓ (this document + ARCHITECTURE.md)
- Stage 1: Foundation ← Next (build system, APVTS shell, pluginval gate → COMPAT-01)
- Stage 2: DSP — 3 phases
- Stage 3: GUI — 2 phases
- Stage 4: Polish/Validation — presets, pluginval ×3, changelog

---

## Stage 2: DSP Phases

### Phase 2.1: Core Reverse Wet Path (feedback OFF)

**Goal:** Granular reverse smear audible and provably correct in the offline render harness, with feedback hard-wired to 0.

**Components:**
- Offline render harness (console app, no editor) — built FIRST; this is the correctness gate for every phase
- Capture ring buffer (3.5 s stereo, alloc-free clear) — DelayBuffer pattern from O-GrainScatter
- Hann window LUT (WindowLuts pattern from O-simpleGrain, Hann only)
- Grain pool (32 slots, preallocated) + density/grainSize spawn scheduler with per-block spawn cap
- Reverse read law (offset D + 2n), per-grain latching of D/G/gain
- 1/sqrt(overlap) amplitude compensation
- Equal-power dry/wet mix (smoothed)
- Free-mode delayTime only (sync deferred to 2.3)

**Test Criteria:**
- [ ] Harness: single-grain render of a linear ramp plays as a reversed ramp (grain direction probe — FUNC-01)
- [ ] Harness: impulse input → wet envelope ramps UP toward the reversed position (reverse bloom, not gated block — FUNC-01)
- [ ] Harness: no sample-to-sample discontinuity > −60 dBFS outside signal content at defaults (DSP-01)
- [ ] Harness: density sweep 0→100% during playback — no clicks, wet loudness flat within ±1 dB (DSP-01)
- [ ] feedback=0 produces exactly one wet generation (FUNC-03 precondition)
- [ ] No allocations/locks in processBlock (code review — PERF-01)

---

### Phase 2.2: Feedback Loop + Damping + Stability

**Goal:** Damped regeneration through the shared capture buffer, loop-stable at 100% feedback.

**Components:**
- Feedback tap (post grain-sum, pre width/mix) → smoothed feedback gain
- HP (lowCut) + LP (highCut) per channel: `juce::dsp::IIR::Filter` + `ArrayCoefficients::makeHighPass/makeLowPass` assigned in place, smoothed cutoffs, `0.49·fs` clamp
- tanh soft-clip + non-finite guard (guard resets filters AND zeroes feedback source, keeps last-known-good coeffs)
- Feedback return summed into capture buffer write

**Test Criteria:**
- [ ] Successive regenerations show measurable HF loss (highCut < 20 kHz) and LF loss (lowCut > 20 Hz) (FUNC-03)
- [ ] 60 s render at feedback 100%, default damping: peak below 0 dBFS ceiling, zero NaN/Inf (DSP-03)
- [ ] Coefficient updates: ArrayCoefficients in-place, no heap allocation on audio thread (code review + allocation guard — DSP-02)
- [ ] Cutoff sweeps during playback: no zipper/clicks (QUAL-01 partial)

---

### Phase 2.3: Tempo Sync + Width + Parameter Polish

**Goal:** Complete the parameter surface; all Stage-2 requirements green.

**Components:**
- AudioPlayHead per-block BPM read; 13-entry noteDivision→beats table (incl. dotted/triplet); `delayMs = beats·60000/bpm` clamped [50, 2000]; free-time fallback when `getBpm()` empty or playhead null
- Per-grain equal-power pan spread from width (alternating-sign random bias)
- Full automated parameter-sweep pass in harness (all 10 params)

**Test Criteria:**
- [ ] Sync mode @ 120 BPM, 1/4 → 500 ms effective spacing ±1 block (FUNC-02)
- [ ] Free mode: continuous 50–2000 ms spacing (FUNC-02)
- [ ] Harness with no playhead: Sync mode falls back to delayTime, no crash/silence (COMPAT-02)
- [ ] Width 0% → centered wet; 100% → grains audibly spread; no clicks on width automation (FUNC-04)
- [ ] Automated sweeps of every parameter during playback: no clicks, zipper, NaN, Inf (QUAL-01)
- [ ] Sync↔Free mode switch mid-playback is click-free (latched-per-grain crossfade) (QUAL-01)

---

## Stage 3: GUI Phases

### Phase 3.1: Layout and Basic Controls

**Goal:** WebView UI from finalized mockup (mockup does not exist yet — must be created before Stage 3).

**Components:**
- Mockup HTML → `Source/ui/public/index.html`; WebView editor (unique_ptr order: relays → WebView → attachments; 3-arg WebSliderParameterAttachment with nullptr)
- CMake: `NEEDS_WEB_BROWSER TRUE`, `JUCE_WEB_BROWSER=1`, UI BinaryData with distinct NAMESPACE, `check_native_interop.js` served
- Explicit resource-provider URL mapping; `type="module"` script loading
- Guard `createEditor` with `#if JUCE_WEB_BROWSER` + drop PluginEditor.cpp from harness sources; re-run harness after editor lands

**Test Criteria:**
- [ ] WebView opens at correct size, layout matches mockup, all 10 controls render
- [ ] Harness still builds and passes after WebView editor introduction

---

### Phase 3.2: Parameter Binding and Interaction

**Goal:** Two-way UI ↔ DSP for all 10 parameters (UI-01) + sync/free conditional display (UI-02).

**Components:**
- Knob readouts via `SliderState.getScaledValue()` (never a JS min/max map — 6 of 10 params are skewed)
- `syncMode` toggle shows noteDivision (Sync) vs delayTime (Free) control
- Host automation → UI updates; dblclick-reset via getParameterDefaults native fn if readouts need defaults
- Verify UI in a browser against the ~20-line JUCE-bridge stub before DAW testing

**Test Criteria:**
- [ ] All 10 controls drive DSP; host automation updates UI; preset changes update all controls (UI-01)
- [ ] Sync/Free switch swaps visible time control without dead controls (UI-02)
- [ ] Readouts match C++ NormalisableRange incl. skew (spot-check delayTime midpoint ≈ 315 ms)
- [ ] grep-diff getNativeFunction vs withNativeFunction (no silent bridge gaps)

---

## Implementation Flow

- Stage 1: Foundation — CMake target, APVTS (10 params, log-skew via setSkewForCentre), pluginval gate (COMPAT-01)
- Stage 2: DSP — Phase 2.1 → 2.2 → 2.3 (git commit per phase; harness green before advancing)
- Stage 3: GUI — Phase 3.1 → 3.2 (mockup must be finalized first)
- Stage 4: Polish/Validation — factory presets (engineering units + convertTo0to1; staircase-check any threshold-like values), OuariconPresetManager integration, pluginval strictness 10 ×3 (VST3 + AU), auval, CHANGELOG

---

## Implementation Notes

### Thread Safety
- All parameter reads once per block via `getRawParameterValue()->load()`
- All allocation in `prepareToPlay`; grain pool/scheduler/buffers preallocated; spawn cap prevents vector growth
- No locks, no logging, no file I/O anywhere near the audio thread

### Performance
- Estimated < 5% single core @ 48 kHz (≤ 8 grains × 2 ch table-lerp reads + 4 biquads); no oversampling, no FFT
- Hot path: grain render loop — precompute per-grain constants at spawn, branch-free inner loop

### Latency
- Zero reported latency (delay is the effect; dry is unbuffered)

### Denormal Protection
- `juce::ScopedNoDenormals` in processBlock; non-finite guard at feedback write point

### Known Challenges
- Reverse read off-by-one → forward echoes or pitch artifacts: caught by the single-grain reversed-ramp probe (build harness first)
- Overlap compensation must sit BEFORE the feedback tap or density multiplies loop gain (breaks DSP-03)
- Cached-cutoff guard: gate only coefficient recompute, never any enable flag (O-MultiBandCompressor v1.6.0 lesson)
- Factory presets: 6 skewed params — author in engineering units + convertTo0to1 (O-SpectralShaper CR-02 lesson)
- pluginval-10 locally ×3 before any release (latent feedback-NaN class)

---

## References

- Creative brief: `plugins/O-ReverseDelay/.planning/BRIEF.md`
- Requirements: `plugins/O-ReverseDelay/.planning/REQUIREMENTS.md`
- Parameter spec: `plugins/O-ReverseDelay/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-ReverseDelay/.planning/research/ARCHITECTURE.md`
- UI mockup: not yet created — required before Stage 3

Reference plugins:
- **O-GrainScatter** — DelayBuffer (capture ring), GrainScheduler (spawn cap, BPM hardening), GrainPool
- **O-simpleGrain** — WindowLuts (Hann LUT), grain render loop, offline render harness structure
- **O-Freeze** — capture-buffer effect processor shape (stereo effect, no MIDI)
