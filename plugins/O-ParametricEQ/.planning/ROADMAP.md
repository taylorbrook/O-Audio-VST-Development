---
plugin: O-ParametricEQ
complexity_score: 5.0
strategy: phased
staged_implementation: true
created: 2026-11-12
---

# O-ParametricEQ - Implementation Plan

**Date:** 2026-11-12
**Complexity Score:** 5.0 (Complex; raw 11.0, capped)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 196 host parameters (192 band + 4 global) → 196 / 5 = 39.2, capped at **2.0**
- **Algorithms:** 8 DSP components = **8**
  - Analog prototype builder (8 types, 8 slopes)
  - Matched section designer (matched pairs, MLR, matched cuts, Vicanek first-order shelf)
  - Section engine (trapezoidal SVF + TPT one-pole, double state, settled and ramping paths)
  - Band runtime (control grid, parameter ramps, two-slot structural crossfade, neutral sleep)
  - Placement router (Stereo / L / R / Mid / Side, difference injection)
  - Output stage (auto gain, output gain, soft bypass, meter)
  - Auto gain estimator
  - Spectrum tap and analyzer
- **Features:** **1** point
  - FFT / frequency domain (+1): analyzer, display only
  - No feedback loop, no multiband split, no LFO / envelope, no MIDI
- **Total:** 2.0 + 8 + 1 = 11.0 → **5.0** (capped)

---

## Stages

- Stage 0: Research and planning ✓ (this document, `research/ARCHITECTURE.md`)
- Stage 1: Foundation ← next, after the mockup is finalized and `parameter-spec.md` exists
- Stage 2: DSP, 7 phases (2.1b added 2026-11-13, Stage 2 CONTEXT D7)
- Stage 3: GUI, 3 phases
- Stage 4: Polish and validation

**Precondition for Stage 1:** `parameter-spec.md` from mockup finalization (`/start O-ParametricEQ` → option 3). It must adopt the layout in ARCHITECTURE.md "Parameter Mapping" (196 parameters, IDs `bNN_`, global block first, `analyzer` as UI state).

### Requirement coverage by stage

| Stage / phase | Requirements verified |
|---------------|-----------------------|
| 1 Foundation | COMPAT-01, DSP-07 |
| 2.1 Designer | DSP-01 (closed form, Stage 0 tolerance), DSP-03 (static), DSP-06 (design side), FUNC-02 (design targets) |
| 2.1b Designer polish | DSP-01 (final tolerance), DSP-06 (design side, re-run with the polish) |
| 2.2 Static engine | FUNC-01, FUNC-02, FUNC-03, DSP-02, DSP-06, QUAL-01, PERF-01 |
| 2.3 Continuous modulation | DSP-04 (sweeps, stress), DSP-05 |
| 2.4 Structural crossfades | DSP-04 (stepped), DSP-03 (live), FUNC-04, FUNC-01 (enable / remove click) |
| 2.5 Output stage | FUNC-05, FUNC-06 |
| 2.6 Analyzer, display model, listen | UI-02 (C++ half), FUNC-08 (engine half), UI-03 (taps) |
| 3.1 - 3.3 GUI | UI-01, UI-02, UI-03, UI-04, UI-06, UI-07, FUNC-08 |
| 4 Polish | FUNC-07, UI-05, PERF-02, COMPAT-02, and a full re-run of every gate |

---

## Complex Implementation

### Stage 1: Foundation

**Goal:** buildable plugin shell with the frozen parameter layout and state format.

**Components:**
- `plugins/O-ParametricEQ/CMakeLists.txt` on the O-AnalogEQ pattern (VST3, AU, Standalone; `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`; modules `preset-manager` 1.1.1, `eb-garamond` 1.0.0, `vu-meter` 1.0.0)
- APVTS with 196 parameters in parameter groups, `ParameterID { id, 1 }`, ranges and skews per ARCHITECTURE.md
- `getBypassParameter()`, `isBusesLayoutSupported` (mono→mono, stereo→stereo only), `setLatencySamples (0)`
- State: `stateVersion="1"`, `UI` child, tolerant restore
- `param-dump` target and `tests/golden/params.tsv`

**Test criteria:**
- [ ] Builds VST3 + AU; `auval` passes; pluginval strictness 10 passes (COMPAT-01)
- [ ] Gate G11: golden parameter dump committed and diff-clean; state round trip exact; a state with an unknown parameter, a missing parameter and `stateVersion="2"` loads without error (DSP-07)
- [ ] No parameter is written from another parameter's listener
- [ ] `(2,1)` and `(1,2)` layouts refused
- [ ] Reserved IDs `bNN_dyn_range`, `bNN_dyn_threshold`, `bNN_dyn_auto` are not registered and are listed in a source comment next to the layout

Suite traps to check here: the first-order `IIR::Filter` default constructor is not used anywhere; `resolve-target.sh` matches `juce_add_plugin(` inside a comment, so the CMake header comment must not contain it; the Stage 1 placeholder editor must carry the resource map so repo `check-i18n` rule 8 passes.

---

### Stage 2: DSP Phases

#### Phase 2.1: Section designer and offline gate

**Goal:** the design maths alone, proved against the analog prototypes before any processor code.

**Components:**
- `eq::Prototype` and `eq::Designer` as pure headers (ARCHITECTURE.md "Algorithm Details")
- `ResponseEval.h`: closed-form complex response of an SVF section and a one-pole, shared by the gates, the auto gain and the display model
- Offline harness `tests/designer-harness` that reproduces the Stage 0 model tables

**Test criteria:**
- [ ] Gate G1 passes at 44.1, 48, 88.2, 96, 192 kHz; the RBJ negative control fails it (DSP-01)
- [ ] "Model results" table in ARCHITECTURE.md reproduced within 0.05 dB; INFO rows reported
- [ ] Boost / cut reciprocity within 1e-9 dB
- [ ] 10 Hz, Q 40 bell at 192 kHz within 1e-6 dB of the prototype around the peak
- [ ] Every design output finite with `g, k, G > 0` over a 1e6-point random sweep of the full parameter ranges at 22.05 - 192 kHz (DSP-06)
- [ ] Gate G2 static part: corner gains and stop-band slopes from the closed form (DSP-03)
- [ ] Notch and Band Pass: below-clamp regression ceilings hold; above-clamp figures reported (Stage 2 research Q2)
- [ ] Designer output agrees with the Python reference model on a committed golden

**Decision point at the end of 2.1:** if G1 fails for MLR, switch to the documented fallback (Vicanek 2016 section 4.4 bell, Vicanek 2024 Butterworth shelves, shelf Q inert) and record it in the stage CONTEXT. Nothing downstream changes. Amended 2026-11-13: the fallback has no general-Q shelf, so D7b's shelf coverage would need a new user call; a G1 failure therefore stops the phase and escalates before any fallback is written.

---

#### Phase 2.1b: Designer polish (added 2026-11-13; Stage 2 CONTEXT D7)

**Goal:** the accuracy polish behind the same designer interface, proved offline; G1 tightened. Still no processor code.

**Components:**
- `eq::Polish` (ARCHITECTURE.md "Accuracy polish (v1, Phase 2.1b)"): per-rate grid constants, anchored Gauss-Newton refine of each second-order gain section, blend weights, exact inverse for cuts
- Rework (2026-11-13, `stages/2-dsp/RESEARCH-2.1b-continuity.md`): shelf re-centre on the seed side (`Prototype.h`, model, designs golden regenerated), step limit 0.35, no polish below 44.1 kHz
- One constants header shared by both threads
- Designer harness rows for the tightened gate; Python reference of the polish

**Test criteria:**
- [ ] Gate G1 at the Phase 2.1b thresholds at 44.1, 48, 88.2, 96, 192 kHz; the RBJ build and the closed-form seed both fail the 44.1 kHz bell row (DSP-01)
- [ ] Per section, polished error ≤ seed error + 1e-3 dB over the G1 grid at five rates (D7c)
- [ ] Boost / cut reciprocity within 1e-9 dB with the polish on
- [ ] Continuity (amended 2026-11-13, `stages/2-dsp/PLAN-2.1b-rework.md`): on fine sweeps along random lines in parameter space at 44.1 and 48 kHz (bells; shelves at 12, 18, 24, 48, 72, 96 dB/oct; tilt at 12 and 48 dB/oct) and at 88.2, 96, 192 kHz, the polished design's neighbour-step response change is within 4x of the seed's and its 32 000-step figure is at most 0.5x its 8000-step figure
- [ ] Random full-range never-worse at 44.1 and 48 kHz, per section: polished error ≤ seed error + 1.0 dB; 0 sections accepted with g or k more than 10x from the seed; the first Phase 2.1b build fails all three of these rows (added 2026-11-13)
- [ ] Below 44.1 kHz the design is the closed form bit for bit (no polish; added 2026-11-13)
- [ ] Shelf re-centre (a Phase 2.1 seed change): continuous at its boundaries; designs outside its region bit-unchanged from the verified Phase 2.1 golden (added 2026-11-13)
- [ ] Every output finite with `g, k, G > 0` over a 1e6-point random sweep, gain to ±60 dB, 22.05 - 192 kHz (DSP-06)
- [ ] Share of polished / blended / seed sections reported per zone; INFO rows for odd-order shelves, resonant shelves, Tilt, 36 / 72 dB/oct, 96 dB/oct with Q above 4
- [ ] Allocation-free and bit-identical between two threads; design time per section reported (model: seed about 35 ns, polished about 0.8 µs)
- [ ] Every Phase 2.1 row still passes with the seed designer

---

#### Phase 2.2: Static engine, placement, transparency

**Goal:** audio passes through 24 bands with fixed parameters.

**Components:**
- `eq::SvfSection`, `eq::OnePole` (double state, settled path)
- `eq::Band` with one slot; active-band list
- `eq::Router` with difference injection; float ↔ double working buffers; fast path
- Render harness `tests/render-harness` (suite pattern, stub WebView backend)

**Test criteria:**
- [ ] 24 bands enabled together, each changes the output independently (FUNC-01)
- [ ] Rendered impulse response of each of the 8 types matches the designer's closed form within 0.01 dB where the closed form is above -80 dB, on a render whose tail is at least 140 dB below its peak (asserted); 1e-6 dB in the double engine (FUNC-02; wording amended 2026-11-13)
- [ ] Gate G4 (a), (b), (c), (g) as amended 2026-11-13 (QUAL-01, FUNC-03); mono layout: every placement processes the channel
- [ ] Gate G8 as amended 2026-11-13 (DSP-02)
- [ ] `Band::dynOffsetDb` is added to the effective gain and is 0 in v1 (DSP-07, engine half; added 2026-11-13)
- [ ] Gate G6 with static parameters (PERF-01)
- [ ] Frequencies above Nyquist at 44.1 kHz render finite and follow the frequency policy (DSP-06)

---

#### Phase 2.3: Continuous modulation

**Goal:** frequency, gain, Q and gain scale move without artifacts.

**Components:**
- Global control grid; `ControlRamp` (log frequency, dB gain, log Q); target sanitising
- Ramping path of the section engine (per-sample parameter interpolation)
- `dynOffsetDb` hook (held at 0)

**Test criteria:**
- [ ] Gate G3, sweep rows, with the hard control reading ≥ 10x (DSP-04)
- [ ] Gate G5 (DSP-04, DSP-06)
- [ ] Gate G7 (DSP-05)
- [ ] Gate G6 under stress (PERF-01)
- [ ] Parameter events placed on multiples of 2048 samples render bit-identically at block sizes 32, 64, 512, 2048

---

#### Phase 2.4: Structural crossfades and bypass

**Goal:** type, slope, placement, enable, band bypass and global bypass are click-free.

**Components:**
- Second slot per band; 20 ms raised-cosine crossfade; latched pending change
- Neutral sleep and wake
- Global soft bypass; `processBlockBypassed` override; state clear when fully bypassed

**Test criteria:**
- [ ] Gate G3, step rows (DSP-04, FUNC-01, FUNC-04)
- [ ] Gate G2 live part: slope changes on a running band (DSP-03)
- [ ] Gate G4 (d), (e), (f) (QUAL-01, FUNC-04, FUNC-05)
- [ ] A parameter changed while bypassed is in effect after un-bypass
- [ ] Gate G5 re-run with structural parameters randomised

---

#### Phase 2.5: Output stage and auto gain

**Goal:** output gain, gain scale, auto gain, meter.

**Components:**
- `eq::OutputStage` (seeded ramps), `eq::AutoGain`, peak atomics for `vu-meter`

**Test criteria:**
- [ ] Gate G12 (FUNC-05, FUNC-06)
- [ ] Auto gain toggled and auto gain following a gain sweep pass Gate G3
- [ ] Offline render and real-time-sized blocks give bit-identical auto gain
- [ ] Gate G4 (a) still passes with `auto_gain` off

---

#### Phase 2.6: Analyzer taps, display model, listen

**Goal:** everything the editor needs from the processor, testable headless.

**Components:**
- `eq::SpectrumTap` rings; `SpectrumAnalyzer` (message-thread class, unit-run in the harness)
- `eq::DisplayModel` producing section packets
- `eq::Listen`

**Test criteria:**
- [ ] Gate G9 (a): closed-form response vs measured response at all five rates, LL / RR / MM / SS (UI-02)
- [ ] Analyzer: a -12 dBFS 1 kHz sine reads -12 ± 0.5 dB at 1 kHz at 44.1, 96 and 192 kHz; ring overflow drops data without blocking
- [ ] Gate G6 with the analyzer on and the rings saturated (PERF-01)
- [ ] Listen on / off and listen band changes pass Gate G3; with listen off the output is bit-identical to the output without the listen code path (FUNC-08 engine half)

---

### Stage 3: GUI Phases

#### Phase 3.1: Layout, bindings, scale check

**Goal:** mockup integrated, parameters bound.

**Components:**
- Finalized mockup → `Source/ui/public/index.html`; EB Garamond; i18n scaffold
- Relays for 196 parameters; `UI` state native functions (`getUiState`, `setUiState`)

**Test criteria:**
- [ ] Editor opens at the mockup size in VST3 and AU
- [ ] Editor open time and idle message rate measured with all 196 relays; if open time exceeds 500 ms, switch to the documented fallback (relays for the selected band only, native functions for the rest)
- [ ] Host automation moves the controls; preset load refreshes every control
- [ ] Controls for inert parameters are disabled per type / slope

---

#### Phase 3.2: Interactive response display

**Goal:** the curve is the instrument.

**Components:**
- `eqSections` event and init pull; JS response evaluator (two closed forms) and 2x2 composition
- Nodes: click to create (first free band), drag frequency / gain with begin / end gestures, scroll Q, double-click reset, remove
- Selected-band strip with typed entry

**Test criteria:**
- [ ] Gate G9 (b): JS evaluator vs C++ within 0.01 dB (UI-02)
- [ ] Curve updates while the transport is stopped
- [ ] Creating a band on empty space sets its frequency and gain from the click (UI-01); typed entry reaches every range end (UI-04, DSP-06)
- [ ] Drag gestures are recorded as automation in a VST3 host and in Logic

---

#### Phase 3.3: Analyzer, meter, fills, listen

**Components:**
- Analyzer canvas (pre / post), mode control bound to `UI` state
- `vu-meter` output meter; per-band fills; listen while dragging

**Test criteria:**
- [ ] Analyzer Off / Pre / Post / Pre+Post (UI-03); Off stops the audio-thread writes
- [ ] Meter tracks a -6 dBFS sine within 0.5 dB (UI-06)
- [ ] Fills coloured per band (UI-07); listen engages on the dragged band and releases cleanly (FUNC-08)
- [ ] 60 fps redraw while dragging with the analyzer on does not raise audio-thread time

---

### Stage 4: Polish and validation

- Factory presets through `preset-manager` (reset-to-defaults before apply; presets never set `output_gain`) (FUNC-07)
- French and zh-Hans (glossary and lint first) (UI-05)
- Gate G10 CPU (PERF-02); lane-pair SIMD only if the scalar budget is missed. INFO row: 24 shelves x 96 dB/oct all sweeping (polish on every tick; fallback is the correction hold in ARCHITECTURE.md "Accuracy polish")
- Gate G13 and renders at 44.1, 48, 88.2, 96, 192 kHz; Logic mono and stereo; one VST3 host; Windows VST3 CI with pluginval (COMPAT-02)
- Full gate battery re-run, listening pass across the factory presets (QUAL-01)

---

### Implementation Flow

- Mockup finalization → `parameter-spec.md`
- Stage 1: Foundation
- Stage 2: DSP — 2.1, 2.1b, 2.2, 2.3, 2.4, 2.5, 2.6 (one commit per phase, path-scoped)
- Stage 3: GUI — 3.1, 3.2, 3.3
- Stage 4: Polish

---

## Risks

| Risk | Level | Phase that retires it | Fallback |
|------|-------|-----------------------|----------|
| MLR gain-section design is a Stage 0 derivation proved only in a float64 model | MEDIUM-HIGH | 2.1 | Vicanek 2016 bell + Vicanek 2024 Butterworth shelves |
| Resonant and steep shelves near Nyquist at 44.1 / 48 kHz are inaccurate in the closed form | LOW-MEDIUM | 2.1 reports; 2.1b polishes (steep shelves become binding; one resonant region stays poor, INFO) | Per section the closed form is kept wherever the polish is not better (D7c); the display shows the truth |
| Polish cost with many steep shelves sweeping at once (model: 46 % of a core for 24 x 96 dB/oct) | MEDIUM | 2.2 smoke reading; 4 (G10 INFO row) | Correction hold: polish every fourth tick |
| Time-varying stability is argued from a paper whose proof was not read | MEDIUM | 2.3 (G3, G5) | Longer ramps, 40 ms crossfades |
| Click metric confounded by level changes of slow filters | MEDIUM | 2.3 / 2.4 | Local κ and a firing hard control are already specified |
| CPU model is not a measurement | MEDIUM | 4 (early smoke measurement in 2.2) | 2-wide SIMD over Stereo lanes |
| 196 WebView relays | MEDIUM | 3.1 | Relays for the selected band only |
| `vu-meter` 1.0.0 has no consumer in the registry | LOW | 3.3 | O-AnalogEQ's `outputLevel` event |
| Notch and band pass above their frequency clamp park a feature inside the audio band (characterised in Stage 2 research Q2; D10 keeps the clamp) | LOW | 2.1 INFO rows | Clamp as specified |

---

## Implementation Notes

### Thread Safety
- Audio thread reads APVTS atomics once per block; structural values are compared with the previous block.
- Designer is stateless; the audio thread and the message thread each own an instance.
- Audio → UI: two `AbstractFifo` rings and peak atomics. UI → audio: three atomics. The curve needs no channel.
- No locks, no allocation, no `AsyncUpdater` in `processBlock`.

### Performance
- PERF-02 budget: 24 cuts x 96 dB/oct, stereo, 48 kHz, settled ≤ 10 % of one core (model 4.6 %); all sweeping ≤ 35 % (model about 22 %); nothing enabled ≤ 0.3 %.
- Hot loop: per active band, per lane, per section over the block. Settled bands use constant parameters.
- Take a smoke CPU reading at the end of 2.2 so a miss is known early. It includes 24 shelves x 96 dB/oct sweeping (polish on every tick).

### Latency
- 0 samples in every configuration. `setLatencySamples (0)` in `prepareToPlay`.

### Denormal Protection
- `juce::ScopedNoDenormals` in `processBlock`. No DC offset injection.

### Known Challenges
- Cancellation-free forms in `MatchedPair` are mandatory; the naive forms fail at low frequency and high Q.
- A ramp that never lands exactly on its target breaks every `memcmp` gate: ramps snap and report settled.
- Hard controls must fire before a click row counts; toggle from deep settings, not around neutral.
- Stimulus level must exceed any threshold in the gate (suite lesson on vacuous gates).
- `File::replaceWithText` defaults to CRLF; harness dumps that are byte-compared need `"\n"`.
- Harness must re-exec with `CFFIXED_USER_HOME` so the processor constructor's factory-preset stamp does not touch the real home.

---

## References

- Creative brief: `plugins/O-ParametricEQ/.planning/BRIEF.md`
- Requirements: `plugins/O-ParametricEQ/.planning/REQUIREMENTS.md`
- Draft parameter spec: `plugins/O-ParametricEQ/.planning/parameter-spec-draft.md` (superseded by ARCHITECTURE.md "Parameter Mapping" until `parameter-spec.md` exists)
- DSP architecture: `plugins/O-ParametricEQ/.planning/research/ARCHITECTURE.md`
- Stage 0 context: `plugins/O-ParametricEQ/.planning/stages/0-ideation/CONTEXT.md`
- Repo research: `research/parametric-eq-filter-design.md`

Similar plugins:
- O-AnalogEQ 1.5.7 — CMake layout, preset-manager wiring, render harness, 30 Hz meter timer, asymmetric-layout refusal
- O-simpleFM — lock-free ring + message-thread FFT analyzer pattern
- O-SimpleReverb `tests/render-check` — max |d2| toggled-vs-held click gate with a negative control
- O-GranulatorInstrument — local κ click rows, `malloc_logger` allocation gate, parameter golden
