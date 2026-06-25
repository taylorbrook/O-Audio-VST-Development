# O-simpleBeatmaker - Implementation Plan

**Date:** 2026-06-25
**Complexity Score:** 5.0 (Complex — capped; raw 10.0)
**complexity_score: 5.0**
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** ~42 APVTS parameters (42/5 = 8.4, capped at 2.0) = **2.0**
  - (The 6×32 step/velocity grid is **custom ValueTree state**, NOT counted as APVTS params — see ARCHITECTURE.md "State Persistence".)
- **Algorithms:** 6 DSP components = **6**
  - SequencerClock (host transport → sample-accurate step grid)
  - TimingFeelEngine (swing + humanize + quantize-strength → per-hit Δt)
  - DrumVoiceEngine (5 synthesized 808/909 voices: kick/snare/clap/hat/tom)
  - UnifiedTriggerRouter (GM map + merge host/sequencer MIDI + sub-slice render)
  - Mixer / Master
  - VizAnalyzer (lock-free AbstractFifo + playhead atomic)
- **Features:** 2 points = **2**
  - Modulation systems (+1) — per-voice pitch/amp envelopes + velocity→timbre
  - External MIDI control (+1) — GM-mapped MIDI-playable voices
- **Total (raw):** 2.0 + 6 + 2 = **10.0** → **capped at 5.0**

> Score caps at 5.0 like every "simple"-family sibling, but the **kind** of difficulty is different: O-simpleFM/Subtractive/Additive are hard in DSP math; O-simpleBeatmaker is hard in **timing infrastructure**. The dominant risk — host-transport sync + sample-accurate sub-step Δt scheduling — is brand-new (no sibling queries the playhead). Phase the build to de-risk *that* first; the voices and state are conventional.

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
| Stage 1 Foundation | COMPAT-01 (loads/pluginval shell); ~42-param APVTS + custom PATTERN ValueTree state + persistence |
| Stage 2 DSP | FUNC-01/02/03/04/06/07, DSP-01/02/03/04/05, DSP-06, PERF-01/02, QUAL-01 |
| Stage 3 GUI | UI-01/02/03/04/05/06, COMPAT-02, QUAL-02 |
| Stage 4 Polish | FUNC-05 (concept presets), FUNC-08 (playability), validation sweep |

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: DrumVoiceEngine — synthesized voices, MIDI-playable (no sequencer yet)

**Goal:** A playable 6-voice synthesized kit triggered by **incoming MIDI only** (GM map). Validate every voice in isolation before any sequencer/timing complexity exists. This is the conventional, low-risk half of the DSP.

**Components:**
- Per-voice structs: `KickVoice`, `SnareVoice`, `ClapVoice`, `HatVoice` (closed+open, shared source + choke), `TomVoice`. Exponential pitch/amp envelopes; `dsp::LookupTableTransform` sine (reuse O-simpleFM `fastSine`); xorshift/`juce::Random` noise; `dsp::StateVariableTPTFilter`/`dsp::IIR` shaping. 808/909 flavor per ARCHITECTURE table.
- `UnifiedTriggerRouter` (MIDI half only): GM note# → voice; velocity → loudness + timbre; mute/solo at mixer.
- `Mixer`/master with `SmoothedValue` output gain.
- Processor reads APVTS once/block → per-voice `setParams`; `ScopedNoDenormals`; `setLatencySamples(0)`.

**Test Criteria:**
- [ ] Loads in DAW as an **instrument**; MIDI routes; GM notes 36/38/39/42/46/45 each fire the correct voice (no crash).
- [ ] Each voice makes a distinct, recognizable sound; per-voice tune/decay/tone/level audibly reshape it.
- [ ] Velocity scales loudness + a little timbre (harder = brighter/snappier).
- [ ] Closed hat (42) chokes open hat (46).
- [ ] Mute silences a voice; solo silences the rest.
- [ ] No clicks/denormal stalls on tails; no aliasing buzz on repeated high-rate hits (render-harness).

#### Phase 2.2: SequencerClock + UnifiedTriggerRouter — host-synced grid, **swing/humanize OFF**

**Goal:** Prove the **sample-accurate** grid: the internal sequencer emits GM note-ons into the shared MidiBuffer at exact sample offsets, host-synced, with the feel engine bypassed (straight time). This is where the hardest correctness item is gated — BEFORE adding the feel math.

**Components:**
- `SequencerClock`: `getPlayHead()->getPosition()` → bpm/ppq/isPlaying (treat all `Optional`); enumerate firing steps in `[ppqStart, ppqStart+block)` (+1-block lookahead bookkeeping); commit each ON step once (discontinuity-safe).
- PATTERN matrix: flat `std::atomic<uint8_t>[6×32]` (0=off, 1–127=on@velocity); audio reads lock-free.
- Standalone/stopped free-run: integrate `phaseInSteps` at `tempo` BPM when host not playing.
- `patternLength` (8/16/32) wrapping; bar alignment via `getPpqPositionOfLastBarStart()`.
- Sequencer emits `MidiMessage::noteOn` at `nominalOffsetInBlock` into the sequencer buffer; **merge** with host buffer (sorted); voices sub-slice render on event offsets.
- `playheadStepPhase` atomic updated each block.

**Test Criteria:**
- [ ] With straight time, each ON step fires at exactly `stepIndex * samplesPer16th` within the bar (±0 samples) — **render-harness Probe 1**.
- [ ] Grid on/off respected; per-step base velocity applied.
- [ ] Host-synced: grid aligns to DAW bar; playhead atomic tracks ppq. Transport stop → free-run at `tempo`.
- [ ] 8/16/32 pattern lengths wrap correctly.
- [ ] A step whose nominal onset sits near a block boundary still fires at the correct absolute sample (no block snapping) — **render-harness Probe 4**.
- [ ] Sequencer-emitted note 36 and host-played note 36 are indistinguishable to the voice (one MIDI stream).

#### Phase 2.3: TimingFeelEngine + VizAnalyzer — swing/humanize/quantize Δt + truthful viz tap (THE lesson)

**Goal:** The pedagogy. Per-hit sub-step Δt and velocity from swing + humanize + quantize-strength, and the lock-free viz tap that carries the **applied** Δt.

**Components:**
- `TimingFeelEngine`: swing `s = 0.5 + (swing/75)/3` on off-beat 16ths (deterministic, NOT quantized); humanize timing ±30 ms + velocity ±24 from **pre-seeded** `juce::Random` (sampled once per hit); composition `Δt = Δswing + Δhuman·(1−q)`, `vel = clamp(stepVel + Δhuman_v·(1−q), 1, 127)`. Bake into the emitted note-on `samplePosition`.
- `VizAnalyzer`: `AbstractFifo` of `VizEvent{voice, step, nominalSampleInBar, appliedSampleInBar, velocity, source}`; UI Δt = applied − nominal (**QUAL-02 by construction**).
- Lookahead handles symmetric (±) humanize; **Fallback A** = late-only humanize if lookahead proves fiddly.

**Test Criteria:**
- [ ] swing=75%: off-beat 16ths delayed by exactly `(swing01/3)*T8` samples; on-beat 16ths unmoved — **render-harness Probe 2**.
- [ ] swing≈50% display → off-beat lands on the 8th-note triplet (triplet swing) within tolerance.
- [ ] humanize=100, quantize=0: measured timing spread > 0 and bounded by ±30 ms; velocity spread bounded by ±24 — **render-harness Probe 3**.
- [ ] **quantize=100: humanize spread collapses to ~0 BUT swing offsets remain** (the core DSP-04 invariant).
- [ ] Viz event `appliedSampleInBar − nominalSampleInBar` equals the offset actually applied to audio (compare to the emitted MidiMessage) — **QUAL-02 gate**.
- [ ] `processBlock` allocation-free and lock-free under profiler (PERF-01); no dropouts (PERF-02).

### Stage 3: GUI Phases

#### Phase 3.1: Step grid + playhead + basic controls + cross-platform wiring (UI-01/02, COMPAT-02)

**Goal:** The headline grid: 6×16 clickable cells, click-again-to-accent velocity quick-states, a live playhead sweep, and all knobs bound — cross-platform correct.

**Components:**
- `ui/public/{index.html, css/, js/app.js, js/juce/...}`; copy `index.js` + `check_native_interop.js`.
- Grid: 6 rows × 16 cols; click toggles cell (native fn `toggleStep`); click-again cycles ghost/normal/accent (native fn `setStepVelocity`); per-cell velocity shown (height/brightness). Playhead column highlighted from the `playhead` event/atomic.
- PluginEditor member order: **relays → WebView → attachments**; `WebSliderRelay`/`WebComboBoxRelay`/`WebToggleButtonRelay` + 3-arg attachments (`nullptr` undoManager); explicit bare-path resource provider; `type="module"`; `import * as Juce`; relative-drag knobs; `getToggleState` for booleans.
- Knob groups: TIMING (swing/humanize/quantize/length/tempo) | per-voice strips (tune/decay/tone/level/mute/solo ×6) | MASTER (output).
- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; Windows `withUserDataFolder(tempDir)`. If a 2nd `juce_add_binary_data` target is ever added, give it a **distinct `NAMESPACE`** (O-simpleGrain lesson).

**Test Criteria:**
- [ ] Grid renders; clicking a cell toggles a hit; click-again cycles ghost/normal/accent; per-step velocity visible.
- [ ] Playhead sweeps the grid in sync with transport (and free-run in standalone).
- [ ] All knobs/selectors two-way bound (drag → DSP; host automation → UI).
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI).
- [ ] Grid state round-trips (save/reload restores the pattern).

#### Phase 3.2: Timing/groove lane + live MIDI readout (UI-03/04, QUAL-02)

**Goal:** The teaching visuals that separate a stiff pattern from a living one — driven by the FIFO drain so they mirror audio exactly.

**Components:**
- Editor `Timer` (30–60 Hz) drains `VizAnalyzer` → emits `trigger`, `timingOffset`, `midi` events; reads `playhead` atomic.
- **Timing/groove lane:** for each hit, draw its **applied Δt** (`appliedSampleInBar − nominalSampleInBar`) relative to its grid line — leftward = early, rightward = late, with swing vs humanize visually distinguishable. Moving swing/humanize/quantize visibly reshapes the lane in real time.
- **Live MIDI readout:** scrolling list/lane of note-on (note#, velocity) from **both** the internal sequencer and incoming MIDI (source flag) — "step grid and piano roll are two views of one MIDI stream."

**Test Criteria:**
- [ ] Raising swing pushes off-beat hits visibly later in the lane; humanize scatters them; raising quantize pulls the scatter back to the grid while leaving swing.
- [ ] The lane offset **matches what is heard** (QUAL-02) — it is the applied Δt, not a recomputation.
- [ ] MIDI readout shows note-ons from sequencer playback AND from played MIDI, with velocity.
- [ ] No audio-thread allocation/FFT; UI smooth; no event loss under fast patterns.

#### Phase 3.3: Tooltips + single-page pedagogical scaffolding + preset hook (UI-05/06, FUNC-05 hook)

**Goal:** The "oh, THAT'S what swing/quantize/velocity do" scaffolding — plain-language tooltips on every control and a projector-readable single page.

**Components:**
- On-hover plain-language tooltips on every parameter + the grid + the timing lane (JS const map).
- Single-page layout tuned for projector legibility (large grid, readable labels, the timing lane as a first-class panel).
- Preset selector UI hook (named concept presets selectable; content lands in Stage 4).

**Test Criteria:**
- [ ] Every control + the grid + the timing lane has a working hover tooltip in plain language.
- [ ] Single page is readable on a projector (no scrolling to see the grid + lane + key knobs).
- [ ] Preset selector loads a pattern + params and updates grid, knobs, and visuals.

---

### Implementation Flow

- Stage 1: Foundation — CMake (synth + MIDI + WebView2 flags), ~42-param APVTS + custom PATTERN ValueTree state + persistence (silent shell).
- Stage 2: DSP — 3 phases
  - Phase 2.1 DrumVoiceEngine (MIDI-playable, no sequencer)
  - Phase 2.2 SequencerClock + unified trigger path (swing/humanize OFF — sample-accurate grid gate)
  - Phase 2.3 TimingFeelEngine + viz tap (swing/humanize/quantize Δt — the lesson)
- Stage 3: GUI — 3 phases
  - Phase 3.1 Step grid + playhead + controls + cross-platform wiring
  - Phase 3.2 Timing/groove lane + live MIDI readout
  - Phase 3.3 Tooltips + single-page scaffolding + preset hook
- Stage 4: Validation/Polish — concept-isolating factory presets (Straight, Triplet Swing, Humanized, Quantize-Strength demo, Ghost-Note Groove), playability tuning, pluginval (VST3+AU), timing-accuracy/QUAL-02 audit, CHANGELOG v1.0.0.

Each phase = one git commit with its test criteria met.

---

## Stage-2 Correctness Gate — Offline DSP Render-Harness

Port O-simpleFM `tests/render-harness/` (console app drives the processor directly, **no DAW**; enable with `-DOUARICON_BUILD_TESTS=ON`). **This harness is the gate for the single hardest item — sample-accurate sub-step Δt.** It must inject a synthetic transport (a small `juce::AudioPlayHead` subclass returning a known bpm/ppq) and assert exact sample offsets, AND expose the sequencer's emitted MidiBuffer as a test hook so offsets are checked directly (not only via audio transients).

Concrete probes:
- **Probe 1 — grid accuracy:** straight time (swing=humanize=0, quantize=100). Each ON step fires at exactly `stepIndex * samplesPer16th` within the bar (±0 samples).
- **Probe 2 — swing:** swing=75%. Off-beat 16ths delayed by exactly `(swing01/3) * T8` samples; on-beat 16ths unmoved. swing≈50% display → off-beat on the 8th-triplet.
- **Probe 3 — humanize + quantize:** humanize=100/quantize=0 → timing spread > 0, bounded by ±30 ms; velocity spread bounded by ±24. quantize=100 → spread → ~0 **but swing offsets remain** (DSP-04 invariant).
- **Probe 4 — block-boundary independence:** a step + Δt straddling a block edge still fires at the correct absolute sample (no snapping). THE hardest case.
- **Probe 5 — MIDI-playable + voices:** inject host note-on per GM note → correct voice fires, makes sound; closed-hat chokes open-hat; velocity scales output. Aliasing budget on repeated high-rate hits (DSP-06/QUAL-01).
- **Probe 6 — viz truth:** emitted `appliedSampleInBar − nominalSampleInBar` equals the offset applied to audio (QUAL-02).

---

## Implementation Notes

### Thread Safety
- Knob params read via cached `getRawParameterValue()` atomics once/block → per-voice `setParams` + feel engine. PATTERN grid via `std::atomic<uint8_t>[6×32]` (UI write / audio read). Audio→UI strictly via lock-free `AbstractFifo` + `std::atomic<float> playheadStepPhase`. Humanize RNG pre-seeded in `prepareToPlay`, never reseeded on the audio thread. UI never calls processing code (critical pattern #5).

### Performance
- ~6 short percussion voices + sequencer bookkeeping — very light, well under one core at 48 kHz. No FFT, no oversampling. Hot path = per-sample envelope multiplies + a couple of filters per active voice + sub-slice boundaries.

### Latency
- **Zero** added latency — the scheduling "lookahead" is bookkeeping, not an output delay line. `setLatencySamples(0)` in `prepareToPlay`. `getLatencySamples()` is **non-virtual** in JUCE 8 — do NOT override.

### Denormal Protection
- `juce::ScopedNoDenormals` at top of `processBlock`; flush long decaying tails; resonant hat filters need it most.

### Known Challenges
- **Sample-accurate sub-step Δt (highest risk):** emit-as-MIDI + sub-slice render; prove in the harness BEFORE the UI. Build straight-time grid first (2.2), add feel math second (2.3). Fallback A = late-only humanize (drops the lookahead requirement); Fallback B = per-sample trigger array.
- **Host-transport robustness:** every `PositionInfo` field is `Optional`; resync on ppq discontinuity; free-run when not playing. Test Logic/Ableton/Reaper + Standalone.
- **quantize must not touch swing:** compute `Δswing` and `Δhuman` as separate terms; quantize scales only `Δhuman`. Single most important invariant.
- **Grid state:** custom `ValueTree "PATTERN"` child + atomics; do NOT make 384 APVTS params.
- **Viz truth (QUAL-02):** carry the applied sample offset in the viz event; never recompute swing/humanize UI-side.

---

## References

- Creative brief: `plugins/O-simpleBeatmaker/.planning/BRIEF.md`
- Requirements: `plugins/O-simpleBeatmaker/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-simpleBeatmaker/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-simpleBeatmaker/.planning/research/ARCHITECTURE.md`
- Stage-0 context: `plugins/O-simpleBeatmaker/.planning/stages/0-ideation/CONTEXT.md`
- UI mockup: none yet (mockup phase pending)

**Similar plugins to reference during implementation:**
- **O-simpleFM** — `Synthesiser`-style voice skeleton, dual-`ADSR`/envelope patterns, `VizRing`/`FmVizAnalyzer` lock-free viz, WebView editor, CMake, **render-harness**. Primary template for voices + viz + harness.
- **O-simpleSubtractive / O-simpleAdditive** — WebView pedagogical template; QUAL-02 truth-by-construction discipline; single-page projector layout.
- **O-simpleGrain** — BinaryData `NAMESPACE` collision lesson (Stage 3.1); render-harness gate precedent.
- **No sibling queries the host playhead** — `SequencerClock` is genuinely new; verify `AudioPlayHead::PositionInfo` usage against local JUCE source.
