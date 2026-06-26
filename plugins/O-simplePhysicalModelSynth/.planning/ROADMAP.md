---
plugin: O-simplePhysicalModelSynth
complexity_score: 5.0
complexity_class: complex
strategy: staged
param_count: 17
algorithm_count: 8
feature_count: 3
created: 2026-06-26
---

# O-simplePhysicalModelSynth - Implementation Plan

**Date:** 2026-06-26
**Complexity Score:** 5.0 (Complex — capped; raw 13.0)
**Strategy:** Phase-based (staged) implementation

---

## Complexity Factors

- **Parameters:** 17 parameters → 17/5 = 3.4, capped at **2.0**
- **Algorithms:** **8** core DSP components counted
  1. Karplus-Strong string loop (delay + loop LPF + feedback)
  2. Fractional-delay tuning (Thiran all-pass + group-delay compensation)
  3. Modal resonant biquad bank (8 modes)
  4. Inharmonicity stretch model (`f_k = f0·k·√(1+B·k²)`)
  5. Pluck exciter (filtered noise burst + position comb)
  6. Strike exciter (band-limited impulse / mallet)
  7. Bow exciter (memoryless STK friction drive)
  8. Material macro + velocity dynamics mapping
  *(Waveguide string is a `nice`-tier add-on, not counted toward the must-set.)*
- **Features:** **3** points
  - Feedback loops (+1) — the KS string loop
  - FFT / frequency-domain (+1) — spectrum/spectrogram visualization
  - Modulation/envelope systems (+1) — amp ADSR + exciter envelopes + velocity→brightness routing
- **Total:** 2.0 + 8 + 3 = 13.0 → **capped at 5.0**

Matches the pedagogical siblings (O-simpleFM, O-simpleAdditive, O-simpleSubtractive all hit the 5.0 cap). Two resonator engines + three exciters + one HIGH-risk friction component → **staged build**.

---

## Stages

- Stage 0: Research ✓ (ARCHITECTURE.md)
- Stage 0: Planning ✓ (this file)
- Stage 1: Foundation ← Next
- Stage 2: DSP (3 must-phases + 1 nice-phase)
- Stage 3: GUI (3 phases)
- Stage 4: Polish (presets, pluginval, changelog, install)

---

## Complex Implementation (Score = 5.0)

### Stage 1: Foundation

**Goal:** Silent synth shell that loads and passes pluginval, with the full 17-param APVTS wired and state persistence.

**Components:**
- `CMakeLists.txt`: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; compile defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; `juce_generate_juce_header` after `target_link_libraries`; link `juce_dsp`. (Single `juce_add_binary_data` target — added in Stage 3.)
- `PluginProcessor.h/.cpp`: APVTS with all 17 params (ParamIDs namespace, O-simpleFM pattern); output-only stereo bus; `getStateInformation`/`setStateInformation`; 16-voice `juce::Synthesiser` (silent `PhysicalModelVoice` stub); `GenericAudioProcessorEditor` placeholder.
- Voice classes named `StringVoice`/`ModalVoice` or one `PhysicalModelVoice` (NOT `SamplerVoice`).
- `tests/render-harness/` scaffold (off by default, `-DOUARICON_BUILD_TESTS=ON`) — guarded under `JUCE_WEB_BROWSER=0`, `PluginEditor.cpp` dropped from harness sources.

**Verifies:** COMPAT-01 (pluginval VST3+AU), COMPAT-02 (WebView2 flags).

**Test Criteria:**
- [ ] VST3 + AU build; plugin appears in DAW instrument list (IS_SYNTH)
- [ ] pluginval passes (strictness 5+)
- [ ] All 17 params visible in generic editor; state save/restore round-trips
- [ ] Silent (no audio yet); no crashes on note input

---

### Stage 2: DSP Phases

#### Phase 2.1: Core KS String + Tuning + Pluck

**Goal:** First audio — a tunable, decaying plucked string.

**Components:**
- `StringResonator`: `juce::dsp::DelayLine<float, Thiran>` + `OnePoleLPF` loop filter + feedback; group-delay-compensated delay length (O-Lyrica reuse).
- `PluckExciter`: filtered noise burst + brightness LPF + position comb (O-Lyrica reuse).
- Loop Damping (cutoff) + Decay (feedback `g<1` clamped) mapping.
- Amp ADSR + 16-voice poly + output gain.
- Coarse/Fine tune.
- Render-harness: **autocorrelation pitch probe** (NOT spectral) + makes-sound + decay + feedback-stable checks.

**Verifies:** FUNC-01, DSP-01, DSP-02, DSP-03 (partial), partial FUNC-02 (Pluck).

**Test Criteria:**
- [ ] Note-on produces a plucked tone that rings and decays
- [ ] Measured f0 within a few cents at C1/C3/C5/C7 (autocorrelation harness — DSP-02)
- [ ] Lowering Damping + Decay darkens/shortens the tail (steel→nylon direction)
- [ ] Feedback clamped <1 — bounded/finite at max Decay (QUAL-01)
- [ ] No note-on click; no DC

#### Phase 2.2: Strike + Bow exciters, Material macro, Velocity, Position

**Goal:** All three exciters on the String; material/dynamics complete.

**Components:**
- `StrikeExciter`: band-limited raised-cosine impulse + hardness LPF (O-Bells reuse).
- `BowExciter`: memoryless STK bow-table friction into the string junction (sustains). **Memoryless-first; validate basic sustained tone before refining.**
- Material macro → co-moves Damping + Decay (log-cutoff + linear-feedback curve).
- Velocity → exciter amp + brightness (`velToBrightness`).
- Excitation Position via exciter comb on KS.

**Verifies:** FUNC-02 (all exciters), FUNC-06, DSP-07, DSP-08, QUAL-01.

**Test Criteria:**
- [ ] Pluck=plucked attack, Strike=mallet attack, Bow=sustained (no decay while held) — FUNC-02 acceptance
- [ ] Swapping exciter (resonator fixed) changes only attack/drive
- [ ] Material knob sweeps steel→nylon in one gesture; Damping+Decay visibly co-move
- [ ] Harder velocity = brighter/stronger
- [ ] No buzz/click/alias; Bow stable at max Bow Force + max Decay (finite/bounded)

#### Phase 2.3: Modal resonator + cross-driving

**Goal:** The second resonator engine; same exciters drive it.

**Components:**
- `ModalResonator`: 8 parallel resonant bandpass biquads (custom direct-form, O-Bassoon precedent), driven by `e[n]`.
- Inharmonicity stretch `f_k = f0·k·√(1+B·k²)`; per-mode T60 + `DECAY_MULTIPLIERS` (O-Bells reuse).
- Mode Brightness upper-mode tilt.
- `resonatorType` switch (String↔Modal); each exciter drives Modal.
- Modal-stem viz snapshot (mode freqs + amplitudes, lead voice).

**Verifies:** FUNC-03, FUNC-04, DSP-04, DSP-05.

**Test Criteria:**
- [ ] Modal mode = inharmonic struck-bar/bell; String mode = harmonic string
- [ ] Each exciter drives Modal (same Strike rings a struck string and a struck bell)
- [ ] Inharmonicity 0%≈bar (near-harmonic), high%≈bell (inharmonic)
- [ ] Struck modal body starts bright, settles onto lowest mode
- [ ] Stem snapshot matches the modes actually sounding

#### Phase 2.4 (`nice`): Waveguide string option

**Goal:** Dual-rail traveling-wave string + true Excitation Position. **Deferrable — does NOT block v1.0.**

**Components:**
- `WaveguideString`: dual `DelayLine<float,Thiran>` rails + bridge LPF + nut sign-inversion (O-Lyrica/O-Bowed reuse); `stringModel` switch; rail-split injection at Position.

**Verifies:** DSP-06 (`nice`).

**Test Criteria:**
- [ ] Waveguide produces an in-tune string equivalent to KS
- [ ] Excitation Position audibly changes timbre (true pickup)
- [ ] If skipped: `stringModel` exposes KS only; no contract break

**Viz wiring (through Stage 2):** `VizRing` lock-free ring + atomic loop-energy scalar + modal-stem array, lead-voice tap (O-simpleFM `FmVizAnalyzer` reuse). Audio-thread copy-only (PERF-01). Confirmed running by end of Stage 2; consumed in Stage 3.

---

### Stage 3: GUI Phases (WebView)

#### Phase 3.1: Layout + basic controls + binding

**Goal:** Single-page WebView, signal-flow left→right, all controls bound.

**Components:**
- Mockup HTML → `Source/ui/public/index.html`; CSS; `js/juce/index.js` + `check_native_interop.js`; `juce_add_binary_data` (UI resources).
- WebView editor (unique_ptr relays→WebView→attachments order; 3-arg `WebSliderParameterAttachment`).
- Bind all 17 params; resonator-aware control visibility (String controls vs Modal controls greyed by `resonatorType`).
- Material macro UI → writes Damping + Decay.

**Verifies:** UI-01.

**Test Criteria:**
- [ ] WebView opens at correct size; layout matches mockup
- [ ] All knobs/switches drag and move DSP params; host automation updates UI
- [ ] Modal-only controls grey out in String mode and vice versa

#### Phase 3.2: Scope + spectrum

**Goal:** Live waveform/decay scope + spectrum/spectrogram.

**Components:**
- Editor 30 Hz Timer reads `VizRing` → scope downsample + FFT spectrum (O-simpleFM reuse); emit to WebView.

**Verifies:** UI-03, UI-04.

**Test Criteria:**
- [ ] Scope shows pluck attack + decay envelope
- [ ] Spectrum shows upper harmonics dying before lower ones during decay
- [ ] Modal mode shows inharmonic spacing

#### Phase 3.3: Loop/flow diagram + modal stems + tooltips

**Goal:** The headline pedagogical visual.

**Components:**
- Animated loop/flow diagram driven by the atomic loop-energy scalar (real circulating energy, visibly dampening each pass); re-skins KS loop / Waveguide rails / modal stems per `resonatorType`.
- Modal stem display from the stem snapshot.
- On-hover tooltips per control.

**Verifies:** UI-02, UI-05, UI-06.

**Test Criteria:**
- [ ] Diagram animates from real loop state and visibly loses energy each pass
- [ ] Diagram re-skins between KS loop / waveguide / modal stems with the switch
- [ ] Stems match sounding modes; tooltips present on every control

---

### Stage 4: Polish

**Goal:** Presets, validation, ship.

**Components:**
- Concept-isolating factory presets (FactoryPresets.cpp): bright steel, muted nylon, koto/harp, struck bar, bell, bowed string (FUNC-07).
- Re-run render-harness at START (guarded against the now-WebView editor).
- pluginval VST3+AU; CHANGELOG; build-and-install.

**Verifies:** FUNC-07, COMPAT-*, all remaining.

---

### Implementation Flow

- Stage 1: Foundation — CMake + 17-param APVTS + silent 16-voice shell + harness scaffold
- Stage 2: DSP — 3 must-phases (2.1 KS+Pluck, 2.2 Strike+Bow+Material, 2.3 Modal) + 2.4 Waveguide (`nice`)
- Stage 3: GUI — 3 phases (3.1 layout/binding, 3.2 scope/spectrum, 3.3 diagram/stems/tooltips)
- Stage 4: Polish — presets, harness re-run, pluginval, changelog, install

---

## Implementation Notes

### Thread Safety
- All param reads `getRawParameterValue()->load()`; coefficient updates on audio thread, crossfaded (64 samples) to avoid zipper clicks.
- Viz: `VizRing` lock-free ring + relaxed atomics (loop-energy scalar, fixed-size modal-stem array, lead-voice pitch). No locks on audio thread (PERF-01).
- Material macro writes `damping`/`decay` via APVTS (message thread, atomic).

### Performance
- KS voice <1% CPU; Modal voice = 8 biquads (128 total @ 16 voices, O-Bassoon-affordable); Bow = table lookup. Total well under 20% single core @ 48 kHz. No oversampling in v1.0 (harness audits Strike top-octave aliasing; add only if needed).

### Latency
- Zero. `setLatencySamples(0)` (getter non-virtual in JUCE 8). `getTailLengthSeconds() ≈ 5`.

### Denormal Protection
- `juce::ScopedNoDenormals` in `processBlock`; feedback `g<1` + DC-blocker; epsilon-guarded modal biquads + Bow friction divisions.

### Known Challenges
- **Bow friction (HIGH risk):** memoryless-first; validate basic sustained tone before refining; hard-clamp; fallback = sustained filtered-noise drive into the loop (O-Bowed lesson).
- **KS tuning (MEDIUM):** Thiran + group-delay compensation; gate via autocorrelation pitch probe, NOT spectral (the loop comb fools spectral probes — O-simpleGrain lesson).
- **Render-harness vs WebView editor:** drop `PluginEditor.cpp` from harness sources + `#if JUCE_WEB_BROWSER` guard on `createEditor`; re-run harness at start of Stage 4 (O-simpleBeatmaker lesson).
- **Naming:** `StringVoice`/`ModalVoice`, not `SamplerVoice`; no bare `end`/`begin` param-ID symbols.

---

## References

- Creative brief: `plugins/O-simplePhysicalModelSynth/.planning/BRIEF.md`
- Parameter spec (draft): `plugins/O-simplePhysicalModelSynth/.planning/parameter-spec-draft.md`
- Requirements: `plugins/O-simplePhysicalModelSynth/.planning/REQUIREMENTS.md`
- DSP architecture: `plugins/O-simplePhysicalModelSynth/.planning/research/ARCHITECTURE.md`
- Stage-0 context: `plugins/O-simplePhysicalModelSynth/.planning/stages/0-ideation/CONTEXT.md`

**In-house reference plugins:**
- O-Lyrica — KS/waveguide loop, `PluckExciter`, fractional-delay tuning, `OnePoleLPF`, group-delay comp.
- O-Bells — modal bank, ratio/decay tables, `StrikeExciter`.
- O-Bowed — dual Thiran rails, memoryless friction junction.
- O-simpleFM — `VizRing`/`FmVizAnalyzer`, 16-voice synth structure, CMake/WebView2 flags, render-harness.
- O-simpleAdditive / O-simpleSubtractive — pedagogical sibling structure, presets, tooltips, lead-voice display tap.
