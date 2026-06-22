# O-simpleAdditive - Implementation Plan

**Date:** 2026-06-22
**Complexity Score:** 5.0 (Complex — capped; raw 12.0)
**complexity_score: 5.0**
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 33 core parameters (33/5 = 6.6, capped at 2.0) = **2.0**
  - 16 Frame-A drawbars + `frameBSource` + scan (position/LFO rate/LFO depth/env amount) + `spectralDecay` + `bitDepth` + `velToDecay` + amp ADSR (4) + mod ADSR (4) + `outputLevel`.
- **Algorithms:** 7 DSP components = **7**
  - Additive single-cycle table-fill engine (16 sines → 2048-pt band-limited table)
  - Frame A → Frame B linear *spectral* morph
  - Spectral-decay macro (per-partial exponential tilt over the note)
  - Bit-depth quantizer (discrete, off sentinel)
  - Per-note Nyquist band-limiting (Kmax drop + boundary taper)
  - Dual ADSR (amp lifetime + mod-env→scan) + scan LFO
  - Real-time FFT spectrum + oscilloscope + drawbar-spectrum visualization
- **Features:** 3 points = **3**
  - Modulation systems (+1) — scan LFO + mod-envelope → scan routing
  - FFT / frequency domain (+1) — live spectrum overlay / scope
  - External MIDI control (+1) — polyphonic MIDI instrument
  - (No feedback loops, no multiband — additive is feed-forward.)
- **Total (raw):** 2.0 + 7 + 3 = **12.0** → **capped at 5.0**

> Raw 12.0 reflects genuine implementation weight (16-partial additive voice + morph + spectral-decay + dual-domain real-time viz). But the build is meaningfully **lower-risk than its sibling O-simpleFM**: additive is feed-forward (no DX7 feedback stability problem), band-limiting is exact (no oversampling, zero latency), and the entire voice/viz/WebView/ADSR/CMake infrastructure is **inherited from the shipped O-simpleFM**. The genuinely new work is the additive render + morph + spectral-decay; everything else is a port.

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

#### Phase 2.1: Core Additive Voice (FUNC-01/05, DSP-01/02)

**Goal:** A polyphonic, MIDI-playable additive voice — 16 drawbars summed into a per-note band-limited single-cycle table, read by phase, shaped by the amp envelope. The audible additive core.

**Components:**
- `AdditiveSound`, `AdditiveSynthesiser` (16-voice cap), `AdditiveVoice : juce::SynthesiserVoice` (copy O-simpleFM `FMVoice`/O-Bassoon skeleton; JUCE 8 custom `prepareToPlay`, no virtual).
- `fastSine` reused from O-simpleFM `Operator.h` (1024-pt `juce::dsp::LookupTableTransform`, floor-modulo wrap).
- `fillSingleCycleTable(table[2048], activeSpectrum, Kmax)` — additive sum of partials k ≤ Kmax; `readTableLinear(table, phase)` per sample.
- Per-note band-limiting: `Kmax = floor(0.5·fs/f0)` at note-on + raised-cosine taper on top 2 harmonics.
- `partial1..16` (Frame A) read once/block → active spectrum (no morph/decay yet); table refill at control rate.
- Amp `juce::ADSR` (voice lifetime = `ampEnv.isActive()`); velocity→amplitude; `outputLevel` (`SmoothedValue`, dB→lin) + headroom normalization of the table sum.
- Processor reads APVTS once/block → `voice->setParams(...)`; `ScopedNoDenormals`; block-level `isfinite` scrub.

**Test Criteria:**
- [ ] Plugin loads in DAW as an **instrument**, MIDI routes, plays polyphonically (no crash).
- [ ] Fundamental-only (H1=100, rest 0) = pure sine; drawbars at 1/k approximate a sawtooth (FUNC-01 acceptance).
- [ ] Raising/lowering each drawbar audibly changes the corresponding harmonic.
- [ ] Tone correctly pitched to MIDI note number; high notes (C7+) with all drawbars up produce **no aliasing/buzz** (DSP-02 acceptance).
- [ ] 16 maxed drawbars do not clip (headroom normalization works).
- [ ] Amp ADSR shapes notes; no stuck/silent voices; no clicks on note-on/off.

#### Phase 2.2: Scan/Morph + Mod-Env + LFO (FUNC-02/03, DSP-03/06)

**Goal:** The wavetable dimension — Frame A → Frame B spectral morph driven by manual knob, LFO, and mod-envelope; zipper-free.

**Components:**
- Frame B preset vectors (sine / saw=1/k / square=odd 1/k / odd hollow) selected by `frameBSource` (`AudioParameterChoice`).
- Linear *spectral* morph: `active_k = lerp(A_k, B_k, scan)`.
- Scan pointer = `clamp(scanPosition + lfo·scanLfoDepth + modEnv·scanEnvAmount, 0, 1)` → `SmoothedValue` (~20 ms).
- Global scan LFO (sine, `scanLfoRate` 0.01–20 Hz log skew, advanced once/block).
- Independent mod `juce::ADSR`; `scanEnvAmount` bipolar (−100..+100%).
- Table refills at control rate when scan changes.

**Test Criteria:**
- [ ] scan=0 → output = Frame A; scan=100% → output = Frame B; intermediate = smooth spectral morph (FUNC-02 acceptance).
- [ ] Manual scan, LFO sweep, and mod-env each move the morph pointer; all three sum (FUNC-03 acceptance).
- [ ] Held note morphs over time under LFO and/or mod-env; scope waveform visibly morphs.
- [ ] No zipper noise on scan automation or fast LFO (DSP-03 acceptance).

#### Phase 2.3: Spectral-Decay + Bit-Depth + Viz Tap (DSP-04/05, PERF-01 tap, QUAL-01)

**Goal:** The two remaining spectral lessons + the audio-thread visualization tap.

**Components:**
- Spectral-decay macro: `D_k = exp(−rate·k·tau)`, `rate = spectralDecay·RATE_MAX (+ vel·velToDecay)`, internal per-voice `tau` ramp 0→1 over the note. Applied to the morphed spectrum before table fill. `velToDecay` opt-in (default 0).
- Bit-depth quantizer: `AudioParameterChoice {off,12,10,8,6,4,2}`; `off`=passthrough; else `round(x·L)/L`, `L=2^(bits−1)` at read time.
- Visualization **tap**: mono-sum post-gain → pre-allocated `VizRing` (lifted from O-simpleFM `FmVizAnalyzer.h`); plus a lock-free **active-spectrum snapshot** (16 amplitudes post morph+decay) for the exact drawbar display.

**Test Criteria:**
- [ ] `spectralDecay=0` → balance steady over the note; raising it makes higher partials decay faster, tone darkens, spectrum visibly tilts (DSP-04 acceptance).
- [ ] Lowering bit depth adds audible quantization grit; `off` = clean (DSP-05 acceptance).
- [ ] No objectionable aliasing anywhere across the keyboard (QUAL-01).
- [ ] `processBlock` allocation-free under profiler; `setLatencySamples(0)` (no oversampling); viz ring + snapshot fill without locks.

### Stage 3: GUI Phases

#### Phase 3.1: Layout + Drawbars + Controls (UI-04/05, COMPAT-02)

**Goal:** Single-page WebView with the 16-drawbar surface dominant, all controls bound, cross-platform wiring correct.

**Components:**
- `ui/public/{index.html, css/, js/app.js, js/juce/...}`; copy `index.js` + `check_native_interop.js` (inherit O-simpleFM structure).
- PluginEditor member order: **relays → WebView → attachments**; `WebSliderRelay` (drawbars, scan, decay, ADSR, output), `WebComboBoxRelay` (`frameBSource`, `bitDepth`), per critical-patterns #11/#12 (3-arg attach, `nullptr` undoManager).
- Resource provider **explicit bare-path** mapping; `type="module"` scripts; `import * as Juce` (critical-pattern #21); `getSliderState`/`getComboBoxState` per type (#19).
- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; Windows `withUserDataFolder(tempDir)`.
- Groups: **16 drawbars (Frame A — dominant)** | Frame B / Scan + LFO + Env | Spectral Decay + Bit Depth | Amp Env | Mod Env | Output.

**Test Criteria:**
- [ ] WebView opens, single-page layout renders, classroom/projector-readable.
- [ ] All 16 drawbars + all knobs/menus two-way bound (drag → DSP; host automation → UI); relative-drag knobs (critical-pattern #16).
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI) (COMPAT-02).

#### Phase 3.2: Live Drawbar Spectrum + Oscilloscope (UI-01/02, QUAL-02)

**Goal:** The headline teaching visuals — the drawbars-as-spectrum and the live summed-waveform scope.

**Components:**
- Editor `Timer` (30 Hz): copy scope window from `VizRing` **before** FFT; run `FmVizAnalyzer` (4096 / Blackman-Harris) for the optional FFT overlay.
- **Drawbar spectrum:** read the lock-free active-spectrum snapshot → `"drawbarSpectrumUpdate"`; the 16 bars render the *morphed + decayed* levels (exact, not FFT-derived).
- Scope: downsample window (1024→128, max-abs keep sign) → `"scopeUpdate"`; canvas DPR-aware backing store (project memory: canvas replaced-element sizing).
- JS: `window.__JUCE__.backend.addEventListener` for scope/spectrum/drawbar updates.

**Test Criteria:**
- [ ] The 16 bars are the primary control surface AND read as the live spectrum; they reflect morph + spectral-decay tilt in real time (UI-01, QUAL-02).
- [ ] Scope redraws as drawbars/scan move; waveform morphs visibly with scan (UI-02).
- [ ] Spectrum/scope accurately match the audio (QUAL-02).
- [ ] No audio-thread FFT/alloc; UI smooth at 30 Hz; scope not corrupted by in-place FFT.

#### Phase 3.3: Pedagogical Layer (UI-03, FUNC-04)

**Goal:** The teaching scaffolding — "oh, THAT's how additive works" in 5 minutes.

**Components:**
- On-hover plain-language tooltips on **every** control (overtone series, why odd-only is hollow, what scan/morph does, what bit depth does) — JS const map.
- Educational preset tour (Pure Sine, Sawtooth, Square/hollow, Organ, Morph Pad, Lo-Fi Bells) via the Ouaricon `preset-manager` module / APVTS snapshots, selectable from the UI (UI-05/FUNC-04).
- Optional annotations on the spectrum (label H1, mark odd vs even partials).

**Test Criteria:**
- [ ] Every parameter has a working hover tooltip with a concrete example.
- [ ] Each preset loads and audibly/visually isolates its concept (sine→pure, saw→ramp, square→hollow, morph→evolving, lo-fi→grit).
- [ ] Layout stays single-page and projector-readable (UI-04).

---

### Implementation Flow

- Stage 1: Foundation — project structure, CMake (synth + WebView2 flags, inherit O-simpleFM), APVTS skeleton (33 params), state persistence; silent shell.
- Stage 2: DSP — 3 phases
  - Phase 2.1 Core additive voice (table fill + band-limit + amp ADSR)
  - Phase 2.2 Scan/morph + mod-env + LFO
  - Phase 2.3 Spectral-decay + bit-depth + viz tap
- Stage 3: GUI — 3 phases
  - Phase 3.1 Layout + drawbars + controls + cross-platform wiring
  - Phase 3.2 Drawbar spectrum + oscilloscope
  - Phase 3.3 Tooltips + preset tour
- Stage 4: Validation — pluginval (VST3+AU), preset sweep, artifact/aliasing audit across keyboard, changelog.

Each phase = one git commit with its test criteria met.

---

## Implementation Notes

### Thread Safety
- Parameters read via cached `apvts.getRawParameterValue()` atomics, once per block in the processor; pushed to voices (`setParams`). Voices never touch APVTS. Global scan LFO advanced once/block.
- Audio → UI strictly via lock-free `VizRing` ring (pre-allocated, from O-simpleFM) + a lock-free active-spectrum snapshot. FFT/scope build on the message-thread Timer.
- UI never calls processing code.

### Performance
- 16 voices × (1 table read + amp env) — trivial. Table refills are bounded 16-sine writes at control rate.
- **No oversampling** (band-limiting is exact) → lower CPU and **zero latency** vs O-simpleFM.
- 4096 FFT @ 30 Hz on the message thread — cheap. Estimated well under one core at 48 kHz.

### Latency
- **Zero added latency** — `setLatencySamples(0)` in `prepareToPlay`. (`getLatencySamples()` is non-virtual in JUCE 8 — do NOT override.)

### Denormal Protection
- `juce::ScopedNoDenormals` at top of `processBlock`; block-level `std::isfinite` scrub after summing voices. (Lower denormal exposure than O-simpleFM — no feedback loop — but ADSR tails still warrant the guard.)

### Known Challenges
- **Table-refill cadence:** refill at control rate (per block / on change), scan smoothed 20 ms; escalate to sub-block refill only if fast-LFO zipper appears.
- **Headroom:** normalize the additive sum (`÷ max(1, ΣA_k)` or fixed factor) so 16 maxed drawbars don't clip.
- **Display fidelity (QUAL-02):** the drawbar spectrum must read the *active* (morphed + decayed) amplitudes, not the raw drawbar params — publish the active-spectrum snapshot to the UI.
- **Band-limit boundary:** drop k > Kmax + raised-cosine taper on the top 2 harmonics to avoid clicks at the top of the keyboard.

---

## References

- Creative brief: `plugins/O-simpleAdditive/.planning/BRIEF.md`
- Requirements: `plugins/O-simpleAdditive/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-simpleAdditive/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-simpleAdditive/.planning/research/ARCHITECTURE.md`
- Stage 0 context / decisions: `plugins/O-simpleAdditive/.planning/stages/0-ideation/CONTEXT.md`
- UI mockup: none yet (mockup phase pending)

**Similar plugins to reference during implementation:**
- **O-simpleFM** (shipped) — PRIMARY template: `juce::Synthesiser`/`SynthesiserVoice` skeleton, dual ADSR, `setParams` block push, `fastSine` (`Operator.h`), `VizRing`+`FmVizAnalyzer` lock-free viz, WebView CMake + cross-platform flags, preset-manager module. Reuse near-verbatim.
- **O-Bassoon / O-Lyrica / O-Prism** — `SynthesiserVoice` skeleton, JUCE 8 custom `prepareToPlay`, APVTS `NormalisableRange`/skews, WebView relay/attachment member order.
- **O-Marimba** — oscilloscope FIFO + 30 Hz Timer + `emitEventIfBrowserIsVisible`.
