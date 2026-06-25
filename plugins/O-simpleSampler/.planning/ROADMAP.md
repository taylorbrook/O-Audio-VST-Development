# O-simpleSampler - Implementation Plan

**Date:** 2026-06-25
**Complexity Score:** 5.0 (Complex — capped; raw 12.0)
**complexity_score: 5.0**
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 21 core parameters (21/5 = 4.2, capped at 2.0) = **2.0**
- **Algorithms:** 7 DSP components = **7**
  - Sample playback engine (Repitch fractional-read varispeed + region/read head)
  - Granular Stretch engine (synchronous-granular / SOLA pitch-shift, overlap-add, pitch/time independent)
  - Region / Loop / Reverse engine (equal-power loop crossfade, ping-pong, zero-crossing snap)
  - Anti-aliasing read (4-pt Lagrange + rate-tracking one-pole on upward transposition)
  - Vintage macro (sample-rate decimation S&H + bit-depth reduction; clean at zero)
  - Resonant low-pass filter (`StateVariableTPTFilter` LP + closed-form magnitude curve)
  - Amp ADSR + VCA + 16-voice polyphony/voice-stealing
- **Features:** 3 points = **3**
  - Modulation systems (+1) — amp envelope → VCA
  - FFT / frequency domain (+1) — optional output spectrum (sibling-consistent secondary viz)
  - External MIDI control (+1) — polyphonic MIDI instrument
- **Total (raw):** 2.0 + 7 + 3 = **12.0** → **capped at 5.0**

> Raw 12.0 matches O-simpleFM / O-simpleAdditive. Tier escalates to **5–6** by file I/O + load-your-own sample streaming + the interactive waveform editor → research depth **DEEP**. Most infrastructure (voice model, amp ADSR, lock-free viz, WebView/CMake, dual binary-data NAMESPACE) is inherited from O-simpleGrain/O-simpleSubtractive; most sample-read + grain + loading machinery is mined near-verbatim from O-simpleGrain/O-GrainScatter/O-MicrotonalSampler. The genuine implementation weight is the **two-mode pitch engine (Repitch↔Stretch)**, the **loop crossfade engine**, **Vintage**, and the **interactive waveform editor** — phase the build to land the audible playable sampler first, then the headline lesson.

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
| Stage 1 Foundation | COMPAT-01 (loads/pluginval shell), COMPAT-02 (WebView2 flags + dual binary-data NAMESPACE), 21-param APVTS + state persistence |
| Stage 2 DSP | FUNC-01, FUNC-02, FUNC-04, FUNC-05, FUNC-06, FUNC-08, DSP-01…DSP-07, PERF-01, QUAL-01 |
| Stage 3 GUI | FUNC-03 (load-your-own), FUNC-07 (preset tour), UI-01…UI-05, QUAL-02 |
| Stage 4 Polish | COMPAT-* re-verify, preset tour content, validation sweep, all remaining |

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: Core Playable Sampler — Repitch + Region + Amp ADSR + Built-in Decode (FUNC-01/02/04, DSP-02/06, partial QUAL-01)

**Goal:** A polyphonic, MIDI-playable sampler — a buffered built-in source read through the **Repitch** (fractional-read varispeed) engine, isolated by start/end, shaped by a per-voice amp ADSR + VCA, tuned to the keyboard relative to Root Key. The audible playable instrument; Stretch/loop/Vintage/filter deferred to 2.2.

**Components:**
- `SamplerSound`, `SamplerSynthesiser` (16-voice), `SamplerVoice : juce::SynthesiserVoice` (copy O-simpleFM/O-simpleGrain skeleton; custom — NOT `juce::SamplerVoice`).
- **Source buffer + region read head:** shared `juce::AudioBuffer<float>`; `[startSamp,endSamp)` region; `readPos += keyRatio` (Repitch); `keyRatio = 2^((note − rootKey + tune + fine/100)/12)`, root C3. `start`, `end`, `rootKey`, `tune`, `fine`.
- **Anti-aliasing read:** 4-pt Lagrange (`Source/dsp/LagrangeInterpolation.h` from O-simpleGrain) + rate-tracking one-pole (`fc=0.5fs/rate`) for `rate>1`. DSP-02.
- **Built-in embedding + decode:** second `juce_add_binary_data` target (`NAMESPACE BinaryData`); `createReaderFor(MemoryInputStream)` → resample to engine rate off-thread → atomic publish. Per-sample default root seeds `rootKey`. FUNC-02.
- Amp `juce::ADSR` → VCA + voice lifetime; `velToAmp` velocity scaling. Processor reads APVTS once/block → `voice->setParams(...)`; `ScopedNoDenormals`; `SmoothedValue` on output.

**Test Criteria:**
- [ ] Loads in DAW as an **instrument**, MIDI routes, plays 16-voice polyphonically (no crash).
- [ ] Playing the Root Key sounds the sample at original pitch; notes above/below transpose by varispeed (Repitch).
- [ ] Moving Start/End changes the played region; playback begins at Start, ends at End.
- [ ] `tune`/`fine` transpose independent of the keyboard.
- [ ] Built-in samples select, decode, and play; selecting one seeds its default root.
- [ ] **No aliasing/buzz at high notes** — render-harness aliasing probe (DSP-02, QUAL-01).
- [ ] No clicks on note-on/off; no denormal CPU stalls on long releases.

#### Phase 2.2: Region Completion + Stretch + Vintage + Filter (FUNC-05/06/08, DSP-01/03/04/05)

**Goal:** The full per-voice tone chain and the headline lesson — loop (fwd/ping-pong + equal-power crossfade), reverse, the **Stretch** synchronous-granular engine (the Repitch↔Stretch A/B), the Vintage macro, and the resonant LP filter.

**Components:**
- **Loop engine:** `loopMode` (off/forward/ping-pong); `loopStart`/`loopEnd` (% of region); equal-power (sin/cos) crossfade over `loopCrossfade` ms via a second read head; zero-crossing marker snap (off-thread). `reverse` negates read direction. DSP-03, FUNC-05/06.
- **Stretch engine (HEADLINE):** synchronous-granular SOLA — time-axis read head at 1× realtime + per-grain resample by `keyRatio`, Hann overlap-add (fixed ~60 ms grain, 2× overlap, `MaxGrainsPerVoice=4`); mined from O-simpleGrain `GrainVoice`/`GrainScheduler`. `pitchMode` toggles Repitch↔Stretch. DSP-01.
- **Vintage:** per-voice S&H decimation (`fsEff=lerp(fs,3000,vintage)`) + bit-crush (`bits=lerp(clean,8,vintage)`); **full bypass at 0**. Placed before the filter. DSP-04.
- **Filter:** per-voice `juce::dsp::StateVariableTPTFilter<float>` LP; `filterCutoff` (log) + `filterResonance`→Q; lead-voice `displayCutoffHz`/`displayK` atomics for the curve. DSP-05.

**Test Criteria:**
- [ ] Loop forward sustains a sub-second sound held as a note without dropout; **no audible click at the seam** with crossfade (DSP-03).
- [ ] Ping-pong loops smoothly; reverse plays the region backwards.
- [ ] **Repitch vs Stretch:** in Repitch a low note slows/lengthens and a high note speeds/shortens; in Stretch a held note keeps duration while pitch tracks the key; the difference is obvious toggling on a sustained sample (DSP-01).
- [ ] Vintage at 0% is bit-for-bit clean; increasing adds quantization/aliasing grit; no NaNs/runaway across range (DSP-04).
- [ ] Filter cutoff/resonance audibly shape the tone; LP open at default.
- [ ] No zipper on parameter moves; no denormal stalls.

#### Phase 2.3: Anti-alias Hardening + Viz Tap + Voice-Stealing + RT-safety + Render-Harness (DSP-07, PERF-01, QUAL-01)

**Goal:** RT-safety, graceful polyphony, the audio-thread visualization tap (waveform playhead + filter curve + optional scope), and the offline DSP render-harness correctness gate.

**Components:**
- Voice-stealing (`juce::Synthesiser` default — steal quietest/oldest); confirm 16-voice budget. DSP-07.
- Anti-alias hardening pass (verify high-key + extreme-stretch budgets); denormal/`isfinite` audit; `setLatencySamples(0)`.
- **Viz tap (lock-free):** `displayPlayhead` (lead-voice readPos), `displayCutoffHz`/`displayK` (lead voice), optional `VizRing` mono-sum output scope/spectrum (reuse O-simpleGrain `VizAnalyzer.h`). Static waveform peaks computed off-thread on load. No alloc/FFT/locks on the audio thread (FFT on the message Timer). PERF-01.
- **Offline DSP render-harness** (port O-simpleFM/O-simpleGrain `tests/render-harness/`): the Stage-2 correctness gate.

**Test Criteria:**
- [ ] `processBlock` allocation-free under profiler/sanitizer; loading a new sample does not glitch/block the audio thread (PERF-01).
- [ ] 16 voices, graceful voice-stealing, no stuck notes on fast playing (DSP-07).
- [ ] Latency reported as 0 (no oversampling).
- [ ] Render-harness asserts: Repitch tuning accuracy; **Stretch pitch/time independence (single-grain autocorrelation probe)**; loop-seam click absence; Vintage clean-at-zero; anti-alias budget on high notes (QUAL-01).

### Stage 3: GUI Phases

#### Phase 3.1: Layout + Controls + Cross-Platform Wiring + Sample Loading (FUNC-03, UI-05, COMPAT-02)

**Goal:** Single-page projector-readable signal-path WebView with all 21 controls bound, cross-platform correct, plus load-your-own (drag-drop + file picker).

**Components:**
- `ui/public/{index.html, css/, js/app.js, js/juce/...}`; copy `index.js` + `check_native_interop.js`; reuse `modules/webview-drop-streaming.js` from O-simpleGrain.
- PluginEditor member order: **relays → WebView → attachments**; `WebSliderRelay`/`WebComboBoxRelay`/`WebToggleButtonRelay` + matching 3-arg attachments (`nullptr` undoManager). `getComboBoxState` for `sourceSample`/`loopMode`/`pitchMode`; `getToggleState` for `reverse`.
- Resource provider with **explicit bare-path** mapping; `type="module"` scripts; `import * as Juce`; pass the `Juce` namespace to the drop module; relative-drag knobs.
- **Sample loading (FUNC-03):** `dropSampleStart`/`dropSampleChunk`/`dropSampleCommit` native fns + `juce::Base64::convertFromBase64`; `FileChooser` fallback; truncate >30 s + notice.
- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; Windows `withUserDataFolder(tempDir)`; **dual binary-data NAMESPACE** (`UIBinaryData` + `BinaryData`).
- Groups left→right: **SOURCE** (sample/Load…) | **REGION** (start/end/loop/reverse/xfade) | **PITCH** (root/mode/tune/fine) | **VINTAGE** | **FILTER** (cutoff/res) | **AMP** (ADSR/velToAmp) | **OUTPUT**.

**Test Criteria:**
- [ ] WebView opens; single-page signal-path layout renders, classroom/projector-readable (UI-05).
- [ ] All 21 controls two-way bound (drag → DSP; host automation → UI).
- [ ] Drag a `.wav` onto the editor loads + plays it; file-picker fallback works; oversized file truncates with a notice (FUNC-03).
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI).

#### Phase 3.2: Interactive Waveform Editor + Filter Curve + Amp-ADSR (UI-01/02/03, QUAL-02)

**Goal:** The headline teaching surface — the interactive waveform editor with live playhead, plus the filter curve and amp-ADSR, driven by the message-thread Timer.

**Components:**
- **Waveform editor canvas:** static source waveform (peak bins pushed once on load); draggable **start/end** handles + shaded **loop region** with handles; **root-key indication**; **live playhead** from `displayPlayhead` atomic at 30 Hz. DPR-aware canvas (`width=clientWidth·dpr`, project memory canvas gotcha). Handles drive params via relays (two-way). UI-01.
- **Repitch-vs-Stretch visible (UI-02):** the playhead moves at the pitch-coupled rate in Repitch and at 1× in Stretch — a live indicator/label makes the difference explicit.
- **Filter curve:** closed-form `|H_LP|` from `displayCutoffHz`/`displayK` atoms (same g/k as audio) → `"filterCurveUpdate"`. **Amp-ADSR:** push amp-env value → `"envUpdate"`; JS animates the ADSR shape with a playhead. UI-03, QUAL-02.
- Optional output scope/spectrum from `VizRing` (copy scope window BEFORE FFT).

**Test Criteria:**
- [ ] Dragging start/end/loop handles updates the params and vice-versa; loop region shaded inside the region.
- [ ] Playhead tracks the live read position during playback (UI-01).
- [ ] Toggling Repitch↔Stretch visibly changes the playhead behaviour (UI-02).
- [ ] Filter curve **matches what is heard** (QUAL-02); amp-ADSR animates with the note.
- [ ] No audio-thread FFT/alloc; UI smooth at 30 Hz; canvas crisp on Retina.

#### Phase 3.3: Pedagogical Layer — Tooltips + Preset Tour Hook + Signal-Path Readability (UI-04, FUNC-07)

**Goal:** The scaffolding for the "oh, that's how a sampler works" moment.

**Components:**
- On-hover plain-language tooltips on **every** control (JS const map) (UI-04).
- Preset tour UI hook: named concept presets selectable from the UI (Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell, Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped) — full content lands in Stage 4. FUNC-07.
- Signal-path readability pass (Source → Region → Pitch → Vintage → Filter → Amp → Output left-to-right).

**Test Criteria:**
- [ ] Every control has a working hover tooltip.
- [ ] Preset selector loads presets and updates all controls + the waveform/curve visuals.
- [ ] Layout reads as the signal path; projector-legible.

---

### Implementation Flow

- Stage 1: Foundation — project structure, CMake (synth + WebView2 flags + dual binary-data NAMESPACE), 21-param APVTS + state persistence (silent shell).
- Stage 2: DSP — 3 phases
  - Phase 2.1 Core playable sampler (Repitch) + region + amp ADSR + built-in decode
  - Phase 2.2 Region completion + Stretch + Vintage + filter
  - Phase 2.3 AA hardening + viz tap + voice-stealing + RT-safety + render-harness
- Stage 3: GUI — 3 phases
  - Phase 3.1 Layout + controls + cross-platform wiring + sample loading
  - Phase 3.2 Interactive waveform editor + filter curve + amp-ADSR
  - Phase 3.3 Tooltips + preset tour hook + signal-path readability
- Stage 4: Validation/Polish — concept-preset tour (7 presets), pluginval (VST3+AU), aliasing/loop-click/Vintage-clean/RT/QUAL-02 audit, CHANGELOG v1.0.0.

Each phase = one git commit with its test criteria met.

---

## Stage-2 Correctness Gate — Offline DSP Render-Harness

Port O-simpleFM/O-simpleGrain `tests/render-harness/` (console app drives MIDI + asserts acceptance criteria, **no DAW**; enable with `-DOUARICON_BUILD_TESTS=ON`). Assertions for this plugin:
- Note-on → audio; note-off → release tail → silence; voice lifetime gated on amp env.
- **Repitch tuning:** measured pitch at the Root Key = source pitch; an octave up = 2× (FUNC-01/DSP-01).
- **Stretch pitch/time independence:** held note keeps duration while pitch tracks the key — validated by a **single-grain autocorrelation probe** (the grain-rate comb confounds spectral probes; project memory) (DSP-01).
- **Loop-seam click absence:** forward/ping-pong loop with crossfade — no transient at the seam (DSP-03/QUAL-01).
- **Vintage clean-at-zero:** `vintage=0` output is bit-for-bit identical to the clean read; increasing adds measurable quantization (DSP-04).
- **Anti-alias budget:** high keys (and extreme Stretch) — no spurious partials above tolerance (DSP-02/QUAL-01).

---

## Implementation Notes

### Thread Safety
- Parameters read via cached `apvts.getRawParameterValue()` atomics once per block; pushed to voices (`setParams`). Voices never touch APVTS.
- Source buffer hot-swap via **atomic pointer swap** (double-buffer; build off-thread; reap old after a safe interval). Audio thread reads only the published buffer.
- Audio → UI strictly via `displayPlayhead`/`displayCutoffHz`/`displayK`/env atomics + lock-free `VizRing`. FFT/scope/curve build on the message-thread Timer.
- UI never calls processing code (critical-pattern #5).

### Performance
- 16 voices × (1 Lagrange read [Repitch] or ≤4 grain reads [Stretch] + optional one-pole + S&H/quantize + 1 SVF LP + 1 ADSR) — light; most voices run the cheap Repitch path. No oversampling. Optional 4096 FFT @ 30 Hz on the message thread — cheap.

### Latency
- **Zero** added latency (no oversampling, no lookahead) — call `setLatencySamples(0)` in `prepareToPlay`. `getLatencySamples()` is **non-virtual** in JUCE 8 — do NOT override.

### Denormal Protection
- `juce::ScopedNoDenormals` at top of `processBlock`; per-voice + block-level `std::isfinite` scrub. No feedback loop, so denormal exposure is otherwise low.

### Known Challenges
- **Stretch (highest risk):** synchronous-granular SOLA reusing O-simpleGrain `GrainVoice`/`GrainScheduler`; tune the fixed grain (60 ms / 2× Hann / pool 4) for the cleanest "same length, different pitch"; phase-vocoder HQ mode deferred to v1.1. Validate with the harness autocorrelation probe.
- **Upward-transposition AA:** Lagrange + rate-tracking one-pole (O-simpleGrain decision #3); fallback 2× OS only if the harness budget fails.
- **Loop crossfade:** equal-power second-read-head crossfade + zero-cross snap; never hard-jump the read head.
- **Vintage clean-at-zero:** full bypass at `vintage==0`.
- **Dual binary-data NAMESPACE:** samples target `NAMESPACE BinaryData` + UI target `NAMESPACE UIBinaryData` (O-simpleGrain Stage-3.1 collision lesson) — distinct NAMESPACE *and* HEADER_NAME.
- **macOS drag-drop:** reuse `webview-drop-streaming.js` + `juce::Base64::convertFromBase64` (NOT `MemoryBlock::fromBase64Encoding`); picker fallback.

---

## References

- Creative brief: `plugins/O-simpleSampler/.planning/BRIEF.md`
- Requirements: `plugins/O-simpleSampler/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-simpleSampler/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-simpleSampler/.planning/research/ARCHITECTURE.md`
- Stage-0 context: `plugins/O-simpleSampler/.planning/stages/0-ideation/CONTEXT.md`
- UI mockup: none yet (mockup phase pending)

**Similar plugins to reference during implementation:**
- **O-simpleGrain** — PRIMARY reuse: `LagrangeInterpolation.h`, `GrainVoice`/`GrainScheduler` (Stretch engine), rate-tracking one-pole AA, embedded `.wav` + decode/resample/hot-swap, dual binary-data NAMESPACE, `webview-drop-streaming.js`, `VizAnalyzer.h`, render-harness.
- **O-simpleSubtractive** — resonant TPT LP + closed-form curve, per-voice ADSR, 16-voice Synthesiser, lead-voice display-atomic, doc format.
- **O-simpleFM / O-simpleAdditive** — voice skeleton, `setParams` block push, WebView CMake/flags, preset-manager; Additive's bit-depth lesson = Vintage bit-crush model.
- **O-MicrotonalSampler** — macOS WebView content-streaming drag-drop, `juce::Base64::convertFromBase64`, atomic source hot-swap.
- **O-GrainScatter / O-Freeze** — overlap-add, loop/region read + equal-power crossfade.
