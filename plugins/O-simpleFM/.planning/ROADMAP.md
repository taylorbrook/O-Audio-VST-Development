# O-simpleFM - Implementation Plan

**Date:** 2026-06-20
**Complexity Score:** 5.0 (Complex — capped; raw 11.0)
**complexity_score: 5.0**
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 17 core parameters (17/5 = 3.4, capped at 2.0) = **2.0**
- **Algorithms:** 5 DSP components = **5**
  - PM voice core (carrier + modulator phase accumulators)
  - Modulator self-feedback (DX7 two-sample average)
  - Dual ADSR system (independent amp + mod envelopes, mod→index routing)
  - Anti-aliasing (key-tracked index ceiling + polyphase-IIR oversampling)
  - Real-time FFT spectrum + oscilloscope visualization
- **Features:** 4 points = **4**
  - Feedback loops (+1) — modulator self-feedback
  - FFT / frequency domain (+1) — live spectrum analyzer
  - Modulation systems (+1) — mod-envelope → index routing
  - External MIDI control (+1) — polyphonic MIDI instrument
- **Total (raw):** 2.0 + 5 + 4 = **11.0** → **capped at 5.0**

> Raw 11.0 reflects genuine implementation weight (synth voice + feedback + dual-domain real-time viz). The DSP itself is conceptually simple (2 operators), but the **stable feedback + anti-aliasing + lock-free dual-visualization** stack is the real work — phase the build accordingly.

---

## Stages

- Stage 0: Research ✓
- Stage 0: Planning ✓
- Stage 1: Foundation ← Next
- Stage 2: DSP (phased — 3 phases)
- Stage 3: GUI (phased — 3 phases)
- Stage 4: Validation / Polish

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: Core PM Voice (FUNC-01/02/05, DSP-01/02/03)

**Goal:** A polyphonic, MIDI-playable 2-operator PM voice with the amp envelope and clean sine operators — the audible FM core.

**Components:**
- `FMSound`, `FMSynthesiser` (16-voice cap), `FMVoice : juce::SynthesiserVoice` (copy O-Bassoon skeleton).
- `Operator.h` phase-accumulator helper + `fastSine` via `juce::dsp::LookupTableTransform` (1024 pts, linear, **floor-modulo wrap**).
- PM core in **radians** convention: `carOut = fastSine(carPhase + I·modOut)`; `modPhase += 2π·f_m/fs`.
- Modulator frequency mode: `f_m = modFixedMode ? modFixedHz : (fc·ratio)` — Ratio (key-tracks) vs Fixed-Hz (constant, inharmonic teaching).
- `ratio` (with `ratioSnap` at read site), `modFixedMode`/`modFixedHz`, `modIndex` (`I = 20·norm^1.7`), `outputLevel`.
- Amp `juce::ADSR` (voice lifetime = `ampEnv.isActive()`); velocity→amplitude.
- Processor reads APVTS once/block → `voice->setParams(...)`; `ScopedNoDenormals`; `SmoothedValue` on index/output, Multiplicative (or snap+ramp) on ratio.

**Test Criteria:**
- [ ] Plugin loads in DAW as an **instrument**, MIDI routes, plays polyphonically (no crash).
- [ ] Index 0 = pure sine; raising index audibly adds sidebands (no zipper).
- [ ] Integer ratio = harmonic; non-integer (e.g. 1:1.414) = inharmonic/bell.
- [ ] Fixed-mode modulator holds constant Hz while carrier tracks pitch (inharmonic shifts with note); Ratio mode key-tracks.
- [ ] Amp ADSR shapes notes; long sustains hold, release tails cleanly; no stuck/silent voices.
- [ ] No clicks on note-on/off; no denormal CPU stalls on long releases.

#### Phase 2.2: Mod Envelope → Index + Feedback (FUNC-03/04, DSP-05/06)

**Goal:** The two headline expressive features — mod-envelope-scaled index and stable self-feedback.

**Components:**
- Mod `juce::ADSR` independent of amp ADSR.
- **Multiplicative** mod→index: `I_inst = baseIndex·((1−depth)+depth·modEnv)`, `modEnvToIndex` default 1.0.
- `velToIndex` (default 0).
- DX7 self-feedback: `fbOut = fastSine(modPhase + coeff·½(prev1+prev2))`, clamp **history**, `isfinite` scrub, reset on note-on; `feedback` 0–1 → coeff max ≈ π with `x^1.5` taper.
- `SmoothedValue` on feedback.

**Test Criteria:**
- [ ] Mod env sweeps timbre over a held note independent of amplitude (brass swell, e-piano ping).
- [ ] `modEnvToIndex` at depth 1.0 + sustain 0 → pure-sine tail; carrier null reachable near I≈2.405.
- [ ] Feedback enriches modulator toward sawtooth → noise smoothly; **no screech/limit-cycle**, no NaN.
- [ ] Velocity→index responds only when `velToIndex` > 0.
- [ ] No zipper on feedback/index automation.

#### Phase 2.3: Anti-Aliasing + Visualization Tap (QUAL-01, PERF-01 tap)

**Goal:** Hit QUAL-01 across the full range (sine-only operators) and add the audio-thread visualization tap.

**Components:**
- Key-tracked index ceiling: `effIndex = min(I_inst, (0.9·Nyq − fc)/fm − 1)` (uses current `f_m`, incl. fixed mode), smoothed/crossfaded.
- `juce::dsp::Oversampling<float>` (`filterHalfBandPolyphaseIIR`) **2× always-on** around the voice sum; `setLatencySamples()`. **This is the complete v1.0 AA chain (sine-only).**
- **(v1.1 — DEFERRED)** non-sine operators (`carWave`/`modWave`) + band-limited additive wavetables + 4× OS. Not in v1.0 (decision: sine-only).
- Visualization **tap**: mono-sum post-gain → pre-allocated `juce::AbstractFifo` ring (no alloc, no FFT, no locks). Block-level `isfinite` scrub.

**Test Criteria:**
- [ ] High index + high feedback + high pitch: no objectionable aliasing (QUAL-01).
- [ ] Fixed-mode modulator at high `modFixedHz` + high index stays within aliasing budget (ceiling uses `f_m`).
- [ ] Index ceiling dulls extreme highs smoothly (no zipper at the crossfade).
- [ ] `processBlock` is allocation-free under profiler; latency reported correctly to host.

### Stage 3: GUI Phases

#### Phase 3.1: Layout + Basic Controls (UI-05, COMPAT-02)

**Goal:** Single-page WebView with all knobs/toggles bound and cross-platform wiring correct.

**Components:**
- `ui/public/{index.html, css/, js/app.js, js/juce/...}`; copy `index.js` + `check_native_interop.js`.
- PluginEditor member order: **relays → WebView → attachments**; `WebSliderRelay`/`WebComboBoxRelay`/`WebToggleButtonRelay` + matching attachments (3-arg, `nullptr` undoManager).
- Resource provider with **explicit bare-path** mapping; `type="module"` scripts; `import * as Juce`.
- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`; Windows `withUserDataFolder(tempDir)`.
- Groups: Operators (Ratio/Index/Feedback + Modulator Ratio/Fixed mode) | Mod Env | Amp Env | Output.

**Test Criteria:**
- [ ] WebView opens, single-page layout renders, classroom-readable.
- [ ] All knobs/toggles two-way bound (drag → DSP; host automation → UI); relative-drag knobs.
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI).

#### Phase 3.2: Live Spectrum + Oscilloscope (UI-01/02, PERF-01)

**Goal:** The headline teaching visuals, driven by the message-thread Timer.

**Components:**
- Editor `Timer` (30 Hz): copy scope window from `AbstractFifo` **before** FFT.
- Spectrum: Blackman-Harris window → `juce::dsp::FFT` order 12 (4096) → `performFrequencyOnlyForwardTransform` → log-freq dB bins, rise-fast/fall-slow smoothing → `emitEventIfBrowserIsVisible("spectrumUpdate", ...)`.
- Scope: downsample window (512→128, max-abs keep sign) → `"scopeUpdate"`.
- JS: `window.__JUCE__.backend.addEventListener` for spectrum/scope; canvas DPR-aware backing store.

**Test Criteria:**
- [ ] Raising index visibly multiplies discrete sidebands in the spectrum (clearly separated).
- [ ] Changing ratio snaps spectrum harmonic↔inharmonic; scope waveform morphs in real time.
- [ ] Feedback smears spectrum toward sawtooth/noise visibly.
- [ ] No audio-thread FFT/alloc; UI smooth at 30 Hz; scope not corrupted by in-place FFT.

#### Phase 3.3: Pedagogical Layer (UI-03/04, FUNC-06)

**Goal:** The teaching scaffolding that makes it "oh, THAT's how FM works" in 5 minutes.

**Components:**
- Live operator routing diagram (CAR/MOD + feedback reflecting signal flow) — JS/SVG.
- On-hover plain-language tooltips on **every** parameter (JS const map).
- Educational preset tour: named concept-isolating presets (E-Piano, Tubular Bell, Brass, Clarinet, Clang Bell) via suite preset manager / APVTS snapshots.
- Optional annotation marker at carrier null (I ≈ 2.405).

**Test Criteria:**
- [ ] Every parameter has a working hover tooltip.
- [ ] Routing diagram reflects feedback/ratio state.
- [ ] Each preset loads and audibly/visually isolates its concept.

---

### Implementation Flow

- Stage 1: Foundation — project structure, CMake (synth + WebView2 flags), APVTS skeleton
- Stage 2: DSP — 3 phases
  - Phase 2.1 Core PM voice
  - Phase 2.2 Mod-env→index + feedback
  - Phase 2.3 Anti-aliasing + viz tap
- Stage 3: GUI — 3 phases
  - Phase 3.1 Layout + controls + cross-platform wiring
  - Phase 3.2 Spectrum + oscilloscope
  - Phase 3.3 Routing diagram + tooltips + preset tour
- Stage 4: Validation — pluginval (VST3+AU), preset sweep, artifact/aliasing audit, changelog

Each phase = one git commit with its test criteria met.

---

## Implementation Notes

### Thread Safety
- Parameters read via cached `apvts.getRawParameterValue()` atomics, once per block in the processor; pushed to voices (`setParams`). Voices never touch APVTS.
- Audio → UI strictly via lock-free `juce::AbstractFifo` ring (pre-allocated). FFT/scope build on the message-thread Timer.
- UI keyboard MIDI (optional) → native fn → `MidiBuffer` merge in `processBlock`. UI never calls processing code.

### Performance
- 16 voices × (2 sine LUT lookups + feedback) — trivial.
- 2× polyphase-IIR oversampling around the voice sum is the dominant cost (modest). v1.0 is sine-only so 2× is fixed; the 4× non-sine path is deferred to v1.1.
- 4096 FFT @ 30 Hz on message thread — cheap. Estimated well under one core at 48 kHz.

### Latency
- 2× polyphase-IIR oversampling adds small fixed latency — report via `setLatencySamples()` in `prepareToPlay`.
- `getLatencySamples()` is **non-virtual** in JUCE 8 — do NOT override.

### Denormal Protection
- `juce::ScopedNoDenormals` at top of `processBlock` (decaying feedback loops stall hard on denormals).
- Per-voice NaN guard + history clamp; block-level `std::isfinite` scrub after summing voices.

### Known Challenges
- **Phase convention discipline:** commit to **radians** internally; never mix with normalized-turns (wrong index/feedback scaling otherwise). Centralize in `Operator.h`.
- **Feedback stability:** two-sample average is mandatory; clamp the **history**, not just output; reset on note-on (DX7/Tomisawa).
- **In-place FFT:** `performFrequencyOnlyForwardTransform` overwrites its buffer — copy the scope window first.
- **Aliasing budget:** combinatorial in index×ratio×pitch×feedback; index ceiling is the primary guard; tiered OS fallback documented in ARCHITECTURE.md risk table.

---

## References

- Creative brief: `plugins/O-simpleFM/.planning/BRIEF.md`
- Requirements: `plugins/O-simpleFM/.planning/REQUIREMENTS.md`
- Parameter spec: `plugins/O-simpleFM/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-simpleFM/.planning/research/ARCHITECTURE.md`
- Upstream research: `research/fm-phase-modulation-synthesis-o-simplefm.md`
- UI mockup: none yet (mockup phase pending)

**Similar plugins to reference during implementation:**
- **O-Bassoon** — `SynthesiserVoice`/`Synthesiser`/`Sound` skeleton; JUCE 8 custom `prepareToPlay`; block-param push.
- **O-Marimba** — `WaveformFifo` + native-fn oscilloscope; 30 Hz Timer + `emitEventIfBrowserIsVisible`.
- **O-Prism** — APVTS `NormalisableRange`/skews; WebView relay/attachment member order; `dsp::FFT`.
- **O-AnalogEQ** — canonical cross-platform WebView2 CMake config.
- **O-MultiBandCompressor** — `dsp::FFT` usage (we move FFT to the message thread).
