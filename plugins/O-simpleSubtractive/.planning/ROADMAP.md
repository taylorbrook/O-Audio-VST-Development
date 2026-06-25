# O-simpleSubtractive - Implementation Plan

**Date:** 2026-06-25
**Complexity Score:** 5.0 (Complex — capped; raw 11.0)
**complexity_score: 5.0**
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 20 core parameters (20/5 = 4.0, capped at 2.0) = **2.0**
- **Algorithms:** 5 DSP components = **5**
  - Oscillator bank (PolyBLEP main + square sub + white noise + mixer)
  - Multimode ZDF state-variable filter (LP/HP/BP/Notch, 6/12/24 dB, self-osc)
  - Dual ADSR system (filter env → cutoff bipolar/octaves, amp env → VCA + lifetime)
  - Voice & glide manager (Poly / Mono / Legato + portamento)
  - Visualization (closed-form filter curve + FFT spectrum + oscilloscope + dual-ADSR)
- **Features:** 4 points = **4**
  - Modulation systems (+1) — filter envelope → cutoff routing
  - Feedback loops (+1) — ZDF resonance loop / nonlinear self-oscillation
  - FFT / frequency domain (+1) — live spectrum analyzer (headline visual)
  - External MIDI control (+1) — polyphonic MIDI instrument
- **Total (raw):** 2.0 + 5 + 4 = **11.0** → **capped at 5.0**

> Raw 11.0 matches O-simpleFM. The osc/ADSR/viz layers are near-verbatim sibling-ports; the genuine implementation weight is the **self-oscillating multimode filter + its exact magnitude curve + the voice modes** — phase the build to de-risk those first.

---

## Stages

- Stage 0: Research ✓
- Stage 0: Planning ✓
- Stage 1: Foundation ← Next
- Stage 2: DSP (phased — 3 phases)
- Stage 3: GUI (phased — 3 phases)
- Stage 4: Validation / Polish

---

## Requirements → Stage Traceability

| Stage | Requirements covered |
|-------|----------------------|
| Stage 1 Foundation | COMPAT-01 (loads/pluginval shell), 20-param APVTS + state persistence |
| Stage 2 DSP | FUNC-01/02/03/04/05, DSP-01/02/03/04/05/06, PERF-01/02 (tap + 16-voice), QUAL-01 |
| Stage 3 GUI | UI-01/02/03/04/05/06/07, COMPAT-02, QUAL-02 |
| Stage 4 Polish | FUNC-06 (preset tour), FUNC-07 (playability), validation sweep |

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: Source + Linear Filter + Dual ADSR + VCA (FUNC-01/02/03, DSP-01/02/04/06, QUAL-01)

**Goal:** A polyphonic, MIDI-playable subtractive voice — band-limited source through a LINEAR multimode SVF (all 4 modes, all 3 slopes) shaped by two independent ADSRs and a VCA. The audible subtractive core, self-oscillation deferred to 2.2.

**Components:**
- `SubSound`, `SubSynthesiser` (16-voice), `SubVoice : juce::SynthesiserVoice` (copy O-simpleFM/O-Bassoon skeleton).
- `OscillatorBank`: PolyBLEP saw/square, polyBLAMP triangle, LUT sine (`dsp::LookupTableTransform`); PolyBLEP square sub (−1 oct); xorshift white noise; mixer (`oscWave`, `subLevel`, `noiseLevel`).
- `SvfZDF` (Cytomic core, LINEAR first): simultaneous LP/BP/HP; Notch = src − k·BP; 1-pole (`FirstOrderTPTFilter`) for 6 dB; cascade ×2 (resonance on stage 1) for 24 dB. `filterType`, `filterSlope`, `cutoff`, `resonance`.
- Filter `juce::ADSR` → `fcEff` via bipolar `filterEnvAmount` (octaves) + `keyTrack`; amp `juce::ADSR` → VCA + voice lifetime (`ampEnv.isActive()`); velocity→amp always-on.
- Processor reads APVTS once/block → `voice->setParams(...)`; `ScopedNoDenormals`; `SmoothedValue` on cutoff/res/output; per-sample (or ≤16-sample) `g` recompute.

**Test Criteria:**
- [ ] Loads in DAW as an **instrument**, MIDI routes, plays 16-voice polyphonically (no crash).
- [ ] Each `oscWave` audibly differs; saw is bright, sine passes the filter almost unchanged.
- [ ] All four `filterType` modes work; all three `filterSlope` settings audibly change steepness; LP default sweeps cleanly.
- [ ] Filter ADSR sweeps cutoff independently of amp ADSR; bipolar `filterEnvAmount` opens (+) and closes (−).
- [ ] `keyTrack` raises cutoff with pitch when > 0.
- [ ] **No aliasing/buzz at high notes** (saw/square) — render-harness aliasing probe (QUAL-01, DSP-06).
- [ ] No clicks on note-on/off; no denormal CPU stalls on long releases.

#### Phase 2.2: Self-Oscillation + Gain Compensation + Magnitude-Curve Validation (DSP-03/05, QUAL-01)

**Goal:** The "class whistle" — clean bounded self-oscillation at max resonance, in tune with key-track, plus the closed-form magnitude curve validated against the running filter.

**Components:**
- Nonlinear SVF: `tanh` on the resonant integrator path → bounded clean sine at cutoff as `k→0`; resonance taper so the top of the knob reaches self-osc.
- Self-osc gain compensation: tanh bounding + gentle resonance-dependent make-up trim; `isfinite` scrub.
- Key-track validated for self-osc-in-tune (`keyTrack=100%` → chromatic whistle).
- `computeMagnitudeDb(f, g, k, type, slope)` closed-form helper (`Ω = tan(π·f/fs)/g`) — the SAME g/k math as audio; render-harness compares closed-form vs measured swept-sine response (QUAL-02 foundation).
- `displayCutoffHz` / `displayK` atomics updated from the lead voice.

**Test Criteria:**
- [ ] Max resonance + no input → **clean sustained sine at cutoff** (LP/BP/HP), no blow-up, no DC.
- [ ] Resonance sweeps do not jump in level (gain compensation works).
- [ ] `keyTrack=100%` + self-osc → plays chromatically in tune across the keyboard.
- [ ] Closed-form magnitude curve matches measured filter sweep within tolerance for all modes/slopes (render-harness).
- [ ] No zipper/instability sweeping resonance through the self-osc threshold.

#### Phase 2.3: Voice Modes + Glide + Visualization Tap (FUNC-05, PERF-01/02)

**Goal:** Poly/Mono/Legato + portamento, and the audio-thread visualization tap (FFT/scope/curve build deferred to Stage 3).

**Components:**
- `MonoController` (held-note stack, last-note priority): Mono retriggers envelopes; Legato suppresses retrigger while held; both glide pitch.
- Glide: per-voice `SmoothedValue<float, Multiplicative>` over `glide` seconds; `glide=0` instant.
- `voiceMode` switching (Poly → Synthesiser allocation; Mono/Legato → single voice via controller).
- `VizRing` (copy O-simpleFM): mono-sum post-gain → pre-allocated lock-free ring (no alloc/FFT/locks). Env-value atomics for the dual-ADSR display.

**Test Criteria:**
- [ ] Poly = 16 independent voices; Mono = one voice, last-note priority; Legato = slur without envelope retrigger.
- [ ] Glide ramps pitch in Mono/Legato over `glide`; `glide=0` instant; no stuck notes on fast playing.
- [ ] `processBlock` allocation-free under profiler (PERF-01); 16 voices no dropouts (PERF-02).
- [ ] Latency reported as 0 (no oversampling) via `setLatencySamples(0)`.

### Stage 3: GUI Phases

#### Phase 3.1: Layout + Basic Controls + Cross-Platform Wiring (UI-05/06, COMPAT-02)

**Goal:** Single-page projector-readable left-to-right signal-path WebView with all 20 controls bound, cross-platform correct.

**Components:**
- `ui/public/{index.html, css/, js/app.js, js/juce/...}`; copy `index.js` + `check_native_interop.js`.
- PluginEditor member order: **relays → WebView → attachments**; `WebSliderRelay`/`WebComboBoxRelay`/`WebToggleButtonRelay` + matching 3-arg attachments (`nullptr` undoManager).
- Resource provider with **explicit bare-path** mapping; `type="module"` scripts; `import * as Juce`; relative-drag knobs.
- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; Windows `withUserDataFolder(tempDir)`.
- Groups left→right: **OSC** (wave/sub/noise) | **FILTER** (type/slope/cutoff/res/envAmt/keyTrack) | **FILTER ADSR** | **AMP ADSR** | **VOICE/OUT** (mode/glide/output).

**Test Criteria:**
- [ ] WebView opens; single-page left-to-right signal-path layout renders, classroom/projector-readable (UI-06).
- [ ] All 20 knobs/selectors two-way bound (drag → DSP; host automation → UI).
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI).

#### Phase 3.2: Headline Filter-Curve-Over-Spectrum + Oscilloscope + Dual-ADSR (UI-01/02/04, PERF-01, QUAL-02)

**Goal:** The headline teaching visuals, driven by the message-thread Timer.

**Components:**
- Editor `Timer` (30 Hz): copy scope window from `VizRing` **before** FFT.
- Spectrum: Blackman-Harris → `dsp::FFT` order 12 (4096) → `performFrequencyOnlyForwardTransform` → log-freq dB, rise-fast/fall-slow → `"spectrumUpdate"`.
- **Filter curve:** `computeMagnitudeDb` at the same display bins from `displayCutoffHz`/`displayK`/type/slope atomics → `"filterCurveUpdate"`; JS draws the curve OVER the spectrum (the live "before/after filter" figure).
- Scope: downsample window (max-abs keep sign) → `"scopeUpdate"`.
- Dual-ADSR: push filter-env + amp-env values → `"envUpdate"`; JS animates both ADSR shapes on independent scales with playheads.

**Test Criteria:**
- [ ] Sweeping cutoff/resonance moves the filter curve over the spectrum live; harmonics above cutoff visibly attenuated.
- [ ] Curve **matches what is heard** (QUAL-02) — moves with the filter envelope (lead voice).
- [ ] Self-oscillation shows a peak/spike at cutoff; scope shows the filtered waveform morphing in real time.
- [ ] Dual-ADSR display shows the two envelopes moving independently.
- [ ] No audio-thread FFT/alloc; UI smooth at 30 Hz; scope not corrupted by in-place FFT.

#### Phase 3.3: Signal-Path Diagram + Tooltips + Preset Tour Hook (UI-03/07, FUNC-06)

**Goal:** The pedagogical scaffolding for the "oh, that's how subtractive works" moment.

**Components:**
- Live signal-path diagram (osc→filter→VCA with the two envelopes routing) — JS/SVG reflecting `oscWave`/`filterType`/envelope state.
- On-hover plain-language tooltips on **every** parameter (JS const map) (UI-05).
- Preset tour UI hook: named concept presets selectable from the UI (UI-07) — full preset content lands in Stage 4.

**Test Criteria:**
- [ ] Every parameter has a working hover tooltip.
- [ ] Signal-path diagram reflects current routing/waveform/filter state.
- [ ] Preset selector loads presets and updates all controls + visuals.

---

### Implementation Flow

- Stage 1: Foundation — project structure, CMake (synth + WebView2 flags), 20-param APVTS + state persistence (silent shell).
- Stage 2: DSP — 3 phases
  - Phase 2.1 Source + linear multimode filter + dual ADSR + VCA
  - Phase 2.2 Self-oscillation + gain compensation + magnitude-curve validation
  - Phase 2.3 Voice modes + glide + viz tap
- Stage 3: GUI — 3 phases
  - Phase 3.1 Layout + controls + cross-platform wiring
  - Phase 3.2 Filter-curve-over-spectrum + scope + dual-ADSR
  - Phase 3.3 Signal-path diagram + tooltips + preset tour hook
- Stage 4: Validation/Polish — concept-preset tour (8 presets), pluginval (VST3+AU), aliasing/self-osc/QUAL-02 audit, CHANGELOG v1.0.0.

Each phase = one git commit with its test criteria met.

---

## Stage-2 Correctness Gate — Offline DSP Render-Harness

Port O-simpleFM `tests/render-harness/` (console app drives MIDI + asserts acceptance criteria, **no DAW**; enable with `-DOUARICON_BUILD_TESTS=ON`). Assertions for this plugin:
- Note-on → audio; note-off → release tail → silence; voice lifetime gated on amp env.
- **Per-mode magnitude:** swept-sine measured response vs the closed-form `computeMagnitudeDb` for LP/HP/BP/Notch at 6/12/24 dB (QUAL-02 foundation).
- **Aliasing budget:** saw/square at high keys — no spurious partials above tolerance (DSP-06/QUAL-01).
- **Self-osc-in-tune:** resonance=100%, keyTrack=100% — measured pitch tracks the played note (DSP-03/05).
- **Dual-ADSR independence:** filter env sweeps cutoff with amp env held flat and vice-versa (FUNC-04).

---

## Implementation Notes

### Thread Safety
- Parameters read via cached `apvts.getRawParameterValue()` atomics once per block; pushed to voices (`setParams`) + `MonoController`. Voices never touch APVTS.
- Audio → UI strictly via lock-free `VizRing` (pre-allocated) + `std::atomic<float>` (displayCutoff/K, env values). FFT/scope/curve build on the message-thread Timer.
- UI never calls processing code (critical-pattern #5).

### Performance
- 16 voices × (PolyBLEP osc + 1–2 SVF stages + 2 ADSR) — light. Per-sample `tan` for cutoff is the main cost; sub-block (≤16-sample) `g` recompute is the documented fallback. No oversampling.
- 4096 FFT @ 30 Hz on message thread — cheap. Estimated well under one core at 48 kHz.

### Latency
- **Zero** added latency (no oversampling) — call `setLatencySamples(0)` in `prepareToPlay`. `getLatencySamples()` is **non-virtual** in JUCE 8 — do NOT override.

### Denormal Protection
- `juce::ScopedNoDenormals` at top of `processBlock` (resonant filters + decaying envelopes stall hard on denormals).
- SVF self-osc bounded by `tanh`; per-voice + block-level `std::isfinite` scrub.

### Known Challenges
- **Self-oscillating multimode SVF (highest risk):** build the LINEAR filter + all modes/slopes + curve match FIRST (Phase 2.1), add `tanh` self-osc SECOND (Phase 2.2). Fallback A = linear `juce::dsp::StateVariableTPTFilter` + faked self-osc sine.
- **Magnitude-curve correctness (QUAL-02):** use the prewarped `Ω = tan(π·f/fs)/g` with the same g/k as audio; validate in the render-harness.
- **PolyBLEP aliasing:** composes here (steady phase increment); polyBLAMP for triangle; high-key probe in harness.
- **Voice modes (new vs O-simpleFM):** held-note stack + last-note priority; legato suppresses retrigger; multiplicative glide; fallback = Poly+Mono in v1.0, refine Legato in polish.
- **In-place FFT:** `performFrequencyOnlyForwardTransform` overwrites its buffer — copy the scope window first.

---

## References

- Creative brief: `plugins/O-simpleSubtractive/.planning/BRIEF.md`
- Requirements: `plugins/O-simpleSubtractive/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-simpleSubtractive/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-simpleSubtractive/.planning/research/ARCHITECTURE.md`
- Stage-0 context: `plugins/O-simpleSubtractive/.planning/stages/0-ideation/CONTEXT.md`
- UI mockup: none yet (mockup phase pending)

**Similar plugins to reference during implementation:**
- **O-simpleFM** — `Synthesiser`/`SynthesiserVoice` skeleton, dual-`ADSR`, `VizRing`/`FmVizAnalyzer`, WebView editor, CMake, render-harness. **Primary template.**
- **O-simpleAdditive** — same WebView pedagogical template; QUAL-02 spectral-truth discipline.
- **O-Prism** — JUCE 8 SVF usage (StateVariableTPTFilter LP/BP/HP, notch=LP+HP, cascade ×2 for 24 dB, resonance on stage 1); APVTS skews; WebView member order.
- **O-Bassoon** — JUCE 8 custom `prepareToPlay`; block-param push; custom per-voice filter structs.
- **O-simpleGrain** — BinaryData NAMESPACE collision lesson; render-harness gate.
