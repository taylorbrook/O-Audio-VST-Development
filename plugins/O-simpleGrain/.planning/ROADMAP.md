# O-simpleGrain - Implementation Plan

**Date:** 2026-06-24
**Complexity Score:** 5.0 (Complex — capped; raw 13.0)
**complexity_score: 5.0**
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 18 core APVTS parameters (18/5 = 3.6, capped at 2.0) = **2.0**
  - `sourceSample`, `grainSize`, `density`, `position`, `scan`, `freeze`, `windowShape`, `pitchSpray`, `positionSpray`, `scatter`, `grainPitch`, `panSpray`, `velToDensity`, amp ADSR (4), `outputLevel`. (`Load…` is an action/native-function + custom state, NOT an APVTS param.)
- **Algorithms:** 8 DSP components = **8**
  - Grain scheduler + overlap-add engine (preallocated bounded grain pool, steal-oldest)
  - Five precomputed window LUTs (rect / tri / Welch / Gauss / Hann)
  - Global read head — position / scan-time-stretch / freeze (pinned playhead)
  - Per-grain spray & scatter RNG (position / pitch / period / pan)
  - Anti-aliased fractional transposition (4-pt Lagrange read + rate-tracking one-pole)
  - Per-voice amp ADSR + key-tracked MIDI resample
  - Source sample loading (embedded-`.wav` decode + resample; load-your-own drag-drop/picker; atomic hot-swap)
  - Real-time visualization (output FFT/scope + grain-cloud event handoff + grain-count/CPU meter)
- **Features:** 3 points = **3**
  - Modulation / randomization systems (+1) — per-grain spray + period scatter RNG
  - FFT / frequency domain (+1) — output spectrum view (sync↔async)
  - External MIDI control (+1) — polyphonic MIDI instrument (key-tracked)
  - (No feedback loops, no multiband — granular overlap-add is feed-forward.)
- **Total (raw):** 2.0 + 8 + 3 = **13.0** → **capped at 5.0**

> Raw 13.0 reflects genuine implementation weight (a polyphonic granular engine + a global read head with freeze/stretch + sample loading incl. macOS drag-drop + four live visualizations). But the build is **lower core-DSP risk than it looks**: granular overlap-add is **feed-forward** (no feedback-loop stability problem — unlike O-simpleFM's DX7 feedback), the grain engine (`GrainPool` / `GrainScheduler` / `LagrangeInterpolation` / `TripleBuffer` / `FreezeManager`) is mined **near-verbatim from the shipped O-GrainScatter**, the voice/ADSR/viz/WebView/CMake infrastructure is **inherited from O-simpleFM/O-simpleAdditive**, and the highest-effort item (macOS WebView load-your-own drag-drop) is fully de-risked by the **documented, shipped O-MicrotonalSampler v1.0.4 content-streaming pattern** + a guaranteed file-picker fallback. The genuinely new integration work is marrying the grain pool to a polyphonic `Synthesiser` voice, the global read head, MIDI-key resample, and band-limited transposition.

---

## ⚠️ Pre-Stage-1 Gate: parameter-spec.md required

**Only `parameter-spec-draft.md` currently exists.** This is the **plan-before-mockup** path. Before Stage 1 (Foundation) begins, a **full `parameter-spec.md` MUST be produced at mockup finalization**, at which point the **mockup becomes the source of truth** for the final parameter set. The 18-parameter set in ARCHITECTURE.md (17 draft params + the research-adopted `panSpray` and `velToDensity`) is the planning contract; the finalized spec should reconcile any naming/range deltas surfaced by the mockup. Do NOT start Stage 1 against the draft alone.

---

## Stages

- Stage 0: Research ✓
- Stage 0: Planning ✓
- (Gate) Mockup finalization → full `parameter-spec.md` ← required before Stage 1
- Stage 1: Foundation ← Next (after gate)
- Stage 2: DSP (phased — 3 phases)
- Stage 3: GUI (phased — 3 phases)
- Stage 4: Validation / Polish

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: Core Grain Engine + Overlap-Add + Amp ADSR (FUNC-01/02, DSP-01/02/03/07, PERF-01/02)

**Goal:** A polyphonic, MIDI-playable granular voice — a preallocated bounded grain pool scheduled at the density-derived period, each windowed grain read from the (embedded-default) source and overlap-added, shaped by the amp envelope, transposed by MIDI key. The audible granular core.

**Components:**
- `GrainSound`, `GrainSynthesiser` (8-voice cap), `GrainVoice : juce::SynthesiserVoice` (copy O-simpleFM `FMVoice` skeleton; JUCE 8 custom `prepareToPlay`, no virtual).
- Preallocated grain pool `std::array<Grain, MaxGrainsPerVoice=24>` per voice + round-robin/steal-oldest `spawnGrain` (O-GrainScatter `GrainPool` verbatim); global cap 192.
- Per-sample scheduler countdown: period `= fs/density`; `spawnGrain` on fire (O-GrainScatter `GrainScheduler::processBlockFree`).
- Five precomputed 2048-pt window LUTs (rect/tri/Welch/Gauss/Hann); linear-interp read by grain phase (`computeEnvelope`-style).
- Overlap-add summation; 4-pt `lagrangeInterpolate` fractional source read; equal-power per-grain pan.
- Key-tracked resample: `voiceRate = 2^((note−60)/12)`; combined with `grainPitch`.
- Amp `juce::ADSR` (voice lifetime = `ampEnv.isActive()`); velocity→amplitude; `outputLevel` (`SmoothedValue`, dB→lin) + overlap-aware headroom normalization.
- A single embedded default source (e.g. `fire.wav`) decoded at construction (full loading system lands in Phase 2.3); `ScopedNoDenormals`; block-level `isfinite` scrub.
- Processor reads APVTS once/block → `voice->setParams(...)`; advances the global read head (position-only for now — scan/freeze in Phase 2.2).

**Test Criteria:**
- [ ] Plugin loads in DAW as an **instrument**, MIDI routes, plays polyphonically (no crash).
- [ ] Low density (period > grain size) = audibly separated grains; raising density to overlap fuses them into a continuous cloud (FUNC-01/DSP-02 acceptance).
- [ ] Grain size at a few ms = pitched buzz; tens of ms = recognizable source fragments (DSP-01 acceptance).
- [ ] All five window shapes selectable; **rectangular audibly clicks**, Hann/Gaussian do not (DSP-03 acceptance).
- [ ] Held MIDI notes transpose the cloud to pitch; chords work (FUNC-02).
- [ ] `processBlock` allocation-free under profiler; high density × size × 8 voices does NOT xrun (grains thin via steal-oldest) (PERF-01/02).
- [ ] Amp ADSR shapes notes; no stuck/silent voices; no clicks except the intentional rectangular artifact.

#### Phase 2.2: Read Head (Scan / Time-Stretch / Freeze) + Spray & Scatter + Anti-Aliasing (FUNC-03, DSP-04/05/06/08, QUAL-01)

**Goal:** The granular "moves" — move/freeze/stretch the read head, scatter the cloud (sync↔async), and keep up-transposed grains clean.

**Components:**
- Global read head owned by the processor: `playheadPos` advanced by `scan%·realtime` per sample, wrapped to `[0,sourceLen)`; `position` sets the resting point.
- `freeze` (bool, `getToggleState`): pin velocity to 0, smoothed crossfade on engage/disengage (zipper-free, QUAL-01); held note sustains the frozen instant as a pad (FUNC-03).
- Per-voice `juce::Random` spray/scatter (no alloc, no lock): position spray (read start), pitch spray (rate, ± st), scatter (scheduler period jitter → sync↔async), pan spray (equal-power).
- Anti-aliasing: per-grain rate-tracking one-pole low-pass `fc≈0.5fs/rate` applied when `rate>1` (band-limit up-transposition); bypassed at `rate≤1` (DSP-08).
- `velToDensity` (opt-in): velocity scales effective density.
- `SmoothedValue` on `scan`, `position`, `outputLevel`.

**Test Criteria:**
- [ ] Scan moves the read head through the source (forward/slowed/reverse/double); position sets where (DSP-06).
- [ ] Freeze pins the playhead — texture does not drift; freeze→unfreeze is click-free (FUNC-03 acceptance, QUAL-01).
- [ ] Pitch spray and position spray apply independent per-grain randomization (frozen texture shimmers, no two grains identical) (DSP-04 acceptance).
- [ ] Scatter at 0% = synchronous (discrete sidebands/pitched on the spectrum); high scatter = asynchronous (smeared/noisy) (DSP-05 acceptance).
- [ ] High pitch-spray grains stay clean — no unintended buzz/aliasing (DSP-08 acceptance).
- [ ] No zipper on scan/position automation; no allocation/locks introduced by the per-grain RNG (QUAL-01, PERF-01).

#### Phase 2.3: Sample Loading + Visualization Taps (FUNC-04/05, UI-05 tap, PERF-01)

**Goal:** Source selection/loading + the audio-thread taps that feed all four visualizations and the CPU readout.

**Components:**
- Built-in embedding: `juce_add_binary_data` for `fire.wav` / `voice.wav` / `water.wav` / `piano.wav`; decode each via `AudioFormatManager::createReaderFor(MemoryInputStream)` + resample to engine rate; `sourceSample` (`AudioParameterChoice`) selects.
- Source hot-swap via **atomic pointer swap** (double-buffer; build off-thread, publish atomically, reap old) — audio thread never touches a half-loaded buffer.
- Load-your-own (FUNC-05): macOS WebView **content-streaming drag-drop** (`webkitGetAsEntry`→FileReader→base64→`NativeFunction`→temp; **decode with `juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`**), + `FileChooser` picker fallback; 10 s source-length cap.
- Visualization **taps** (audio thread, lock-free, no alloc, no FFT): mono-sum → `VizRing` (output scope/spectrum, lifted from O-simpleFM `FmVizAnalyzer.h`); per-spawn grain event → `TripleBuffer<GrainCloudFrame>` (cloud + playheads, from O-GrainScatter); `std::atomic<int> activeGrainCount` (CPU/overlap readout).

**Test Criteria:**
- [ ] All built-in sources selectable and granulated; "fire" worked example reproducible (FUNC-04).
- [ ] Drag-drop a user `.wav` on macOS loads and granulates (content-streaming, correct base64 decode); picker fallback works; oversized files truncated to 10 s with notice (FUNC-05).
- [ ] Source swap is glitch-free (atomic publish; no audio-thread access to half-loaded buffer).
- [ ] `processBlock` remains allocation-free; viz ring + grain-event triple-buffer + atomic count fill without locks; FFT not on the audio thread (PERF-01).
- [ ] `activeGrainCount` and derived overlap update live (drives UI-05 in Stage 3).

### Stage 3: GUI Phases

#### Phase 3.1: Layout + Controls + Cross-Platform Wiring (UI-06, COMPAT-02)

**Goal:** Single-page projector-readable WebView with all 18 params bound, cross-platform wiring correct.

**Components:**
- `ui/public/{index.html, css/, js/app.js, js/juce/...}`; copy `index.js` + `check_native_interop.js` (inherit O-simpleFM/Additive structure).
- PluginEditor member order: **relays → WebView → attachments**; `WebSliderRelay` (grain/spray/scatter/ADSR/output knobs), `WebComboBoxRelay` (`sourceSample`, `windowShape`), `WebToggleButtonRelay` (`freeze`); critical-patterns #11/#12 (3-arg attach, `nullptr` undoManager).
- Resource provider **explicit bare-path** mapping; `type="module"` scripts; `import * as Juce` (critical-pattern #21); pass the **`Juce` namespace** (not `window.__JUCE__`) to any shared panel; `getSliderState`/`getComboBoxState`/`getToggleState` per type (#19).
- `NativeFunction` registry for drag-drop streaming (`dropSampleStart`/`Chunk`/`Commit`) + the picker trigger.
- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; Windows `withUserDataFolder(tempDir)`.
- Groups: **Source** (selector + Load…) | **Grain** (size, density+overlap readout, position, scan, freeze) | **Window Shape** | **Spray & Scatter** (pitch/position/pan spray, scatter) | **Amp Env** | **Output**.

**Test Criteria:**
- [ ] WebView opens, single-page layout renders, classroom/projector-readable (UI-06).
- [ ] All knobs/menus/toggle two-way bound (drag → DSP; host automation → UI); relative-drag knobs (critical-pattern #16); `freeze` via `getToggleState` (#19).
- [ ] Drag-drop + picker load a source from the UI (FUNC-05 wiring).
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI) (COMPAT-02).

#### Phase 3.2: The Four Live Visualizations + Overlap/CPU Readout (UI-01/02/03/04/05, QUAL-01)

**Goal:** The headline teaching visuals — grain-cloud scatter, source waveform + playheads, window-envelope inset, output scope/spectrum, and the grain-count/CPU readout.

**Components:**
- Editor `Timer` (30 Hz): `TripleBuffer.read()` grain events → **grain-cloud scatter** (read-position × time dots, UI-01) + **source waveform with live playheads, freeze point, shaded position-spray range** (UI-02); copy scope window from `VizRing` **before** FFT → `FmVizAnalyzer` (4096/Blackman-Harris) → **output scope/spectrum** (sync→sidebands, async→noise, UI-04).
- **Window-envelope inset** (UI-03): draw the selected window LUT (one grain), redrawn on `windowShape` change.
- **Grain-count / CPU readout** (UI-05): `activeGrainCount` + derived overlap (`grainSizeSec×density`) → `Grains: N/192`, `Overlap: X.X×`, coarse CPU bar.
- Canvas DPR-aware backing store (project memory: `<canvas>` replaced-element sizing — use `width: calc(...)`, `canvas.width = clientWidth·dpr`).
- JS: `window.__JUCE__.backend.addEventListener` for `grainCloudUpdate` / `scopeUpdate` / `spectrumUpdate` / `windowInsetUpdate` / `grainMeterUpdate`.

**Test Criteria:**
- [ ] Grain cloud accumulates dots (read-position × time); raising density thickens it, spray widens it (UI-01).
- [ ] Source waveform shows live playheads + freeze point + shaded spray range; position/scan/freeze visibly tracked (UI-02).
- [ ] Window-envelope inset matches the selected shape and redraws on change (UI-03).
- [ ] Output scope/spectrum shows discrete sidebands at scatter 0 and smears toward noise at high scatter (UI-04, DSP-05 visual).
- [ ] Grain-count/overlap/CPU readout updates live and ties density×size×poly to cost (UI-05).
- [ ] No audio-thread FFT/alloc; UI smooth at 30 Hz; scope not corrupted by in-place FFT (PERF-01).

#### Phase 3.3: Pedagogical Layer (FUNC-06/07, UI-06)

**Goal:** The teaching scaffolding — "oh, THAT's how granular works" in 5 minutes.

**Components:**
- On-hover plain-language tooltips on **every** control (what overlap-add is, why rectangular clicks, what freeze/scan do, sync vs async, why density×size×poly is the CPU cost) — JS const map (FUNC-07).
- Concept-isolating preset tour (Single Grain, Pitched Buzz, Fragments, Smooth Cloud, Frozen Pad, Asynchronous Cloud, Granular Fire, Rect Click) via the Ouaricon `preset-manager` module / APVTS snapshots, selectable from the UI (FUNC-06).
- Optional cloud/waveform annotations (label the playhead, the freeze pin, the spray range).

**Test Criteria:**
- [ ] Every parameter has a working hover tooltip with a concrete, class-grounded example (FUNC-07).
- [ ] Each preset loads and audibly/visually isolates its concept (single grain, buzz, fragments, cloud, frozen pad, async, fire, rect-click) (FUNC-06).
- [ ] Layout stays single-page and projector-readable (UI-06).

---

### Implementation Flow

- (Gate) Mockup finalization → full `parameter-spec.md` (required before Stage 1).
- Stage 1: Foundation — project structure, CMake (synth + WebView2 flags, inherit O-simpleFM), APVTS skeleton (18 params), state persistence (incl. loaded-source identity), embedded-sample binary-data target; silent shell. (COMPAT-01/02.)
- Stage 2: DSP — 3 phases
  - Phase 2.1 Core grain engine + overlap-add + window LUTs + amp ADSR + key resample
  - Phase 2.2 Read head (scan/stretch/freeze) + spray/scatter + anti-aliasing
  - Phase 2.3 Sample loading (embed + drag-drop + hot-swap) + viz taps
- Stage 3: GUI — 3 phases
  - Phase 3.1 Layout + controls + cross-platform wiring + load-your-own UI
  - Phase 3.2 Four visualizations + overlap/CPU readout
  - Phase 3.3 Tooltips + preset tour
- Stage 4: Validation — pluginval (VST3+AU), preset sweep, artifact/aliasing/freeze audit, drag-drop smoke test (macOS+Windows), changelog.

Each phase = one git commit with its test criteria met.

---

## Implementation Notes

### Thread Safety
- Parameters read via cached `apvts.getRawParameterValue()` atomics, once per block in the processor; pushed to voices (`setParams`). Voices never touch APVTS. The **global read head** is advanced in the processor (shared by all voices). Per-voice `juce::Random` for spray/scatter (no shared RNG).
- Source-buffer hot-swap via **atomic pointer swap** (double-buffer; build off-thread; reap old after a safe interval). Audio thread reads only the published buffer.
- Audio → UI strictly via lock-free `VizRing` (samples), `TripleBuffer<GrainCloudFrame>` (grain events), and `std::atomic<int>` (grain count). FFT/cloud/meter build on the message-thread Timer. UI never calls processing code.

### Performance
- 8 voices × up to 24 grains × (1 Lagrange read + 1 LUT lookup + optional one-pole) per sample. Worst case 192 grains — comfortably under one core at 48 kHz. The bounded pool + steal-oldest caps it; the CPU readout surfaces the cost pedagogically.
- 4096 FFT @ 30 Hz on the message thread — cheap.

### Latency
- **Zero added latency** — `setLatencySamples(0)` in `prepareToPlay`. (`getLatencySamples()` is non-virtual in JUCE 8 — do NOT override.) If the AA fallback ever needs whole-engine 2× oversampling, report its latency then.

### Denormal Protection
- `juce::ScopedNoDenormals` at top of `processBlock`; block-level `std::isfinite` scrub after summing voices. (No feedback loop → low denormal exposure, but the AA one-pole state + ADSR tails warrant the guard.)

### Known Challenges
- **Grain budget / no-xrun (PERF-01/02):** preallocated `std::array<Grain,24>`/voice + steal-oldest; never allocate in `processBlock`.
- **Freeze/unfreeze + scan smoothing (QUAL-01):** smooth the playhead, crossfade on freeze toggle; never hard-jump.
- **Upward-transposition AA (DSP-08):** 4-pt Lagrange + per-grain rate-tracking one-pole; 2× OS only as a documented fallback.
- **macOS load-your-own drag-drop (FUNC-05):** content-streaming (O-MicrotonalSampler v1.0.4); **`juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`** (project memory); picker fallback.
- **Source hot-swap:** atomic pointer swap; never touch a half-loaded buffer on the audio thread.
- **Built-in embedding:** `juce_add_binary_data` accepts `.wav`; decode via `MemoryInputStream` + `AudioFormatManager`; resample off-thread.
- **Headroom:** overlap-aware normalization so dense clouds don't clip.

---

## References

- Creative brief: `plugins/O-simpleGrain/.planning/BRIEF.md`
- Requirements: `plugins/O-simpleGrain/.planning/REQUIREMENTS.md`
- Parameter spec (DRAFT — full spec required at mockup finalization before Stage 1): `plugins/O-simpleGrain/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-simpleGrain/.planning/research/ARCHITECTURE.md`
- Stage 0 context / decisions: `plugins/O-simpleGrain/.planning/stages/0-ideation/CONTEXT.md`
- UI mockup: none yet (plan-before-mockup path; mockup phase pending)

**Similar plugins to reference during implementation:**
- **O-GrainScatter** (shipped) — PRIMARY grain-engine template: `GrainPool.h` (preallocated voice array + round-robin/steal-oldest + `computeEnvelope` + equal-power pan + overlap-add), `GrainScheduler.h` (per-sample countdown), `LagrangeInterpolation.h` (4-pt random-access read), `TripleBuffer.h` (lock-free grain-event handoff), `FreezeManager.h` (playhead pinning). Reuse near-verbatim.
- **O-simpleFM / O-simpleAdditive** (shipped) — PRIMARY infrastructure template: `juce::Synthesiser`/`SynthesiserVoice` skeleton, amp `juce::ADSR`, `setParams` block push, `VizRing`+`FmVizAnalyzer` lock-free viz, WebView CMake + cross-platform flags, preset-manager module, pedagogical UI (tooltips, concept presets).
- **O-MicrotonalSampler** (v1.0.4) — macOS WebView content-streaming drag-drop (`juce::Base64::convertFromBase64`), session-temp reaping, atomic source hot-swap.
- **O-TextureForge / O-Freeze** — overlap-add summation, freeze-buffer pinning, grain voice structs, sample loading.
</content>
</invoke>
