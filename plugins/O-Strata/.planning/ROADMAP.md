---
plugin: O-Strata
version: 2
date: 2026-09-08
complexity_score: 5.0
complexity_raw: 25.0
strategy: phased
supersedes: superseded-baked-v1/ROADMAP.md
---

# O-Strata - Implementation Plan (v2 — live wave-terrain oscillator)

**Date:** 2026-09-08 (re-plan; supersedes the 2026-09-07 baked-geometry plan in `superseded-baked-v1/ROADMAP.md`)
**Complexity Score:** 5.0 (Very High, capped — raw 25.0)
**Strategy:** Phase-based implementation (staged DSP and GUI), on top of the **existing, verified fork** (`Source/`, Stage 1 first pass 2026-09-07)

**Contracts:** `BRIEF.md` (v2), `REQUIREMENTS.md` (v2.0.0, 28 IDs), `parameter-spec-draft.md` (v2: 205 params, 46 mod destinations — locked as `parameter-spec.md` v2 after mockup v2), `research/ARCHITECTURE.md` (v2, this session). Every phase below names the requirement IDs it verifies; the traceability table at the end maps all 28.

**Precondition for Stage 1 (second pass):** UI mockup v2 (`design UI for O-Strata`, ui-mockup skill) and the locked `parameter-spec.md` v2. The Stage 1 pass re-parameterises from the locked spec, not from the draft.

---

## Complexity Factors

- **Parameters:** 205 parameters (205/5 = 41, capped at 2.0) = **2.0**
  - 171 inherited from O-Prism v1.24.0 + 34 new (17 × Osc A/B); `osc?Pos` default and `osc?Unison` range changed in place; `modSlot?Dst` 26 → 46
- **Algorithms:** 17 DSP components = **17**
  - New (8): TerrainOscillator (interface-parity core, per-partial oversampling loop, DC blocker), OrbitLibrary (11 curves), TerrainLibrary (6 analytic terrains), TrajectoryFeedback, HalfbandDecimator (2× / 4× polyphase IIR, JUCE-designed coefficients), ChebyshevTerrain (projector + Clenshaw + per-pitch truncation), ChebyshevScheduler (timer / key / ThreadPool / publish / reaper), TerrainImage (PNG decode / blur / edge / projection / publish)
  - Inherited unchanged (9): SubOscillator, NoiseGenerator, SVFFilter ×2, ADSREnvelope ×2, UnisonEngine (laws; count cap 4), EffectsChain (5 FX), LFO ×4 + ModulationMatrix (destination list grows), GlideProcessor, TuningEngine (scala-tuning-engine v3.0.1)
- **Features:** 6 points
  - Feedback loop (trajectory feedback, per partial) (+1)
  - Frequency-domain processing (Chebyshev projection = DCT-type transform; per-pitch spectral truncation) (+1)
  - Modulation system (4 LFOs, 16-slot matrix, 46 destinations, per-sample smoothing — inherited + 20 new destinations) (+1)
  - External MIDI control (synth voice, note expression, mod wheel / aftertouch — inherited) (+1)
  - File I/O (PNG import via chooser + drag-and-drop, base64 state embedding with a 2 MB cap, Scala / KBM — new + inherited) (+1)
  - Real-time visualization (3D terrain view, 30 Hz playhead, feedback trail, ≋ cycle view in WebView) (+1)
- **Raw Total:** 2.0 + 17 + 6 = **25.0**
- **Final Score:** min(25.0, 5.0) = **5.0** (capped)

**Assessment:** Raw 25.0 is the highest in the catalogue (O-Prism 14.0, the superseded baked plan 23.0). Unlike the baked plan, the risk now sits **on the audio thread**: a new per-sample oscillator with three evaluation paths, per-partial oversampling and feedback, and three harness-gated quality requirements (aliasing, CPU, symmetry). The phasing therefore builds the oscillator one evaluation path at a time with the render harness beside it from Phase 2.1, so every later phase (oversampling, Bandlimited, images) drops into a loop that already passes θ parity and the symmetry gate.

---

## Stages

- Stage 0: Research and Planning — **COMPLETE (2026-09-08, v2)**
- **Pre-Stage-1:** UI mockup v2 → `parameter-spec.md` v2 locked — Next
- Stage 1: Foundation, **second pass** (re-parameterise the verified fork) — 1 phase
- Stage 2: DSP — 5 phases
- Stage 3: GUI — 3 phases
- Stage 4: Polish / Validation — 2 phases

**Workflow ≠ research numbering:** research §7.4 "Stage 2 — live terrain oscillator" = workflow Stage 2; §7.4 "Stage 3 — 3D view" = workflow Stage 3; §7.4 "Stage 1 — baked generators" shipped as the O-Prism v1.25.0 Geometry bank and is v1.1 for O-Strata at the earliest.

**Entry mechanics:** the workflow has no "re-enter a verified stage" command. To start the second pass, set `stage: 1`, `phase: discuss` in `STATUS.md` and run `/plugin-discuss O-Strata 1-foundation` (replan-proposal step 5).

---

## Pre-Stage-1: UI mockup v2 and parameter-spec v2

**Goal:** a v2 mockup (`mockups/v2-ui.yaml/.html`) of the Synth-tab oscillator panels and the Terrain tab from BRIEF §UI Concept, and the locked `parameter-spec.md` v2 (205 params, 46 destinations, exact suffix strings).

**Tasks:**
- `design UI for O-Strata` (ui-mockup skill): O-Prism shell unchanged; Synth tab: Terrain▾ / Orbit▾ dropdowns, 160 × 90 canvas with ⬡/≋ toggle, knob order Size · Level · Pan · Coarse · Fine · Phase · Unison · Detune · Width · Warp · Warp Amt, "Drop PNG" overlay; Terrain tab: top row (Osc A/B · Terrain▾ · Orbit▾ · Quality segmented · Import… · readout), 700 × 540 view + HUD, right column Terrain group (Freq, Mod X, Mod Y, Pitch Track, Saturation; Blur / Edge greyed unless Imported) and Orbit group (Size, Aspect, Rotation, Centre X/Y, Orbit Mod, Feedback, Feedback Damp), amber "Source missing" notice, nautilus at 0.3 opacity.
- Finalise the mockup → `parameter-spec.md` v2; `STATUS.md` `mockup_finalized: true`, `ready_for_implementation: true`.

**Test Criteria:**
- [ ] Mockup parameter list = ARCHITECTURE "Parameter Mapping" (34 new IDs, `osc?Pos` relabelled Orbit Size, no baked IDs)
- [ ] Every new label has a `data-i18n` key placeholder (en / fr / zh-Hans filled in Stage 3)
- [ ] Readout text slots exist for: quality, partials at C4, fit %, exact / approximate, "terrain mod 20 Hz" (Bandlimited)

**Requirements verified:** none (contract lock)

---

## Stage 1: Foundation — second pass (re-parameterise the verified fork)

**Goal:** the existing O-Strata binary with the 48 baked-geometry parameters replaced by the 34 live-oscillator parameters, 46 mod destinations, and COMPAT-01 re-verified. The oscillators still play the sine wavetable placeholder (deleted in Phase 2.1) so the plugin sounds during validation.

**Tasks:**
- `createParameterLayout()` (`PluginProcessor.cpp:95-135` region): remove the 24 baked suffixes per oscillator (`GeoSource … TerSweepRange`, keeping the `osc?Terrain`, `osc?TerBlur`, `osc?TerEdge` IDs with their new meaning / choice lists); add the 17 per-oscillator parameters with the locked spec's IDs, ranges, defaults, skews (Terrain Freq log); `osc?Pos` default 0.5, `osc?Unison` range 1–4; every Choice ≥ 2 entries; `Imported…` last.
- `StrataParamIds::oscIds()` suffix list (`StrataParamIds.h:47-60`) updated; `allSliderIds()` comment counts recomputed (≈ 174 sliders); no suffix shadows a `juce::` free function (grep the list against `juce_core` free-function names).
- `ModDest` enum + `getModDestNames()` (`dsp/ModulationMatrix.h:52-101`): relabel indices 1/2 to "OscA Size" / "OscB Size"; append the 20 entries in ARCHITECTURE Core 9 order; `NumDests = 46`. The voice's mod section reads the 20 new offsets and (for now) discards them — wiring is Phase 2.1.
- State child `geometryImports` → `terrainImports` (`PluginProcessor.cpp:1081, 1129`), still empty.
- UI: O-Prism `index.html` with the v2 mockup's oscillator panels and Terrain tab rendered as plain knobs / dropdowns / segmented control bound by relay (full styling, 3D view and import are Stage 3); `data-i18n` keys added with en / fr / zh-Hans strings (glossary + lint gate); `getModDestNames` native function unchanged (list grows automatically).
- Regenerate `.planning/params.tsv` (param-dump target) → 205 rows; smoke harness (`stages/1-foundation/smoke/`) updated to assert 205 / `terrainImports` and re-run.
- `FactoryPresets.cpp`: the existing preset set re-pointed at the new IDs (values at defaults; real presets in 4.1) so the factory-preset version bump regenerates cleanly.
- CHANGELOG `## v1.0.0 (unreleased)` — Stage 1 second-pass entry; `STATUS.md`; PLUGINS.md row → 🚧 Stage 1.

**Test Criteria:**
- [ ] `ninja O-Strata_VST3 O-Strata_AU` clean; `./scripts/build-and-install.sh O-Strata` installs with the dual-variant sweep
- [ ] `params.tsv` = 205 rows; diff against O-Prism v1.24.0's `params.tsv` = exactly 2 removed (`osc?Table`) + 34 added + 2 changed in place (`osc?Pos` default, `osc?Unison` max) (FUNC-09 ID parity)
- [ ] `modSlot0Dst` exposes 46 choices; indices 0–25 match O-Prism's strings except 1/2 relabelled (FUNC-05 list acceptance)
- [ ] pluginval strictness 10 VST3 + AU pass; `auval` pass (COMPAT-01)
- [ ] Smoke harness: notes sound on A and B (placeholder), Scala load, 205-param state round-trip into a fresh processor, `<terrainImports/>` present
- [ ] UI gates: check-ui-labels, tips, boot-all-uis, i18n lint (en / fr / zh-Hans) green; every new knob moves its parameter
- [ ] No preset-migration gate needed (no O-Strata preset has shipped) — recorded in CHANGELOG as the last free range change

**Requirements verified:** COMPAT-01, FUNC-09 (IDs), FUNC-05 (list)
**Commit:** `feat(O-Strata): Stage 1 second pass — 205 live-oscillator parameters, 46 mod destinations, COMPAT-01 re-verified`

---

## Stage 2: DSP Phases

Each phase lands with its harness gates in `plugins/O-Strata/tests/render-harness/` (ARCHITECTURE "Harness design", gates H1–H11). The harness is a `juce_add_console_app` after `plugins/O-Bowed/tests/render-harness/`, built with `OUARICON_BUILD_TESTS=ON`, compiling `PluginProcessor.cpp` with `JUCE_WEB_BROWSER=0` and no editor TU (the Stage 1 smoke pattern).

### Phase 2.1: TerrainOscillator core at 1× — libraries, interface parity, mod wiring, wavetable deletion, harness skeleton

**Goal:** both oscillators are live terrain oscillators at 1× (no oversampling, no feedback yet) with all 11 orbits and 6 analytic terrains, θ parity with the wavetable oscillator's warps, the 20 new mod destinations wired per sample, and the harness running H1 / H2 / H5 / H8 / H9.

**Tasks:**
- `dsp/TerrainOscillator.h/.cpp` (ARCHITECTURE Core 1; the Sync / Window / Bend / FM code copied from `WavetableOscillator.cpp:183-268`), `dsp/Orbits.h` (Core 2, Superellipse LUT in `prepare`), `dsp/Terrains.h` (Core 3, πF convention, Mitsuhashi normalised 1.747), Saturation (tanh, identity at 0), DC blocker, unison cap 4, `CycleCapture` ring (Core 10, data only).
- `StrataVoice`: `TerrainOscillator oscA, oscB`; 22 `SmoothedValue` base ramps + ModWheel / Aftertouch ramps; offset → parameter mappings (Core 9); `updateBlockRate` (frequency, enums; tracking / damp arrive in 2.2); remove `setWavetableA/B`.
- **Delete** `WavetableOscillator`, `WavetableData`, `WavetableGenerator`, `oscTablePtr[]`, `placeholderTable`, `lastAssignedTable`, `getActiveOscTable`, `getActiveOscInfo` / `getActiveOscFrame` native functions (Decision 6); `retireTable` → type-erased `retire(std::unique_ptr<Retirable>)`; `updateWavetableAssignments` → `updateOscillatorAssignments` (pointers arrive in 2.4 / 2.5, so it is a stub here).
- Harness: console target, CLI (note / velocity / seconds / terrain / orbit / quality / param overrides / preset), exact-cycle spectral analysis (from `terrain-bench/alias.cpp`), gates H1 (Identity-X harness terrain), H2 (symmetry incl. the negative control), H5 (zipper incl. the 0 s-ramp negative control and the 40 Hz-LFO sideband proof), H8 (allocation counter), H9 (block-size invariance).

**Test Criteria:**
- [ ] H1: Identity-X + centred unit circle = cos θ within −80 dB for every warp type, unison 4, Phase 0.25; C4 pitch ± 1 cent (FUNC-01)
- [ ] Every terrain's Mod X / Mod Y and every orbit's Orbit Mod audibly change the spectrum (harness: spectral-centroid delta ≥ 5 % across the 0 → 1 sweep, Ellipse exempt and documented) (FUNC-02, FUNC-03)
- [ ] Orbit Size 0.05 → 1.0 over Sine Product: spectral centroid monotonically non-decreasing (FUNC-03)
- [ ] H2: every terrain × orbit at defaults passes the symmetry gate; the (0,0) / Aspect 1 control fails (DSP-06)
- [ ] H5: 22 destinations stepped 10 % / 100 ms → max sample step ≤ 0.1; 0 s ramp fails; 40 Hz LFO on Centre X shows ±40 Hz sidebands (FUNC-05, QUAL-02)
- [ ] H8: zero allocations across terrain / orbit changes mid-note (DSP-05, PERF-01)
- [ ] H9: 64 / 256 / 1024 block sizes render identically to −100 dB
- [ ] `grep -rn Wavetable Source/` returns nothing; `retire()` type-erased; pluginval strictness 10 still passes
- [ ] No `pow`, `std::function` or `new` in `TerrainOscillator.cpp` / `Orbits.h` / `Terrains.h` outside `prepare` (grep gate)

**Requirements verified:** FUNC-01, FUNC-02, FUNC-03, FUNC-05, DSP-05 (path rules), DSP-06, DSP-07, QUAL-02, PERF-01 (allocation), FUNC-09 / FUNC-10 (regression: tuning tab loads a Scala file and 31-EDO; rendered fundamental = TuningEngine within 0.5 cent on ≥ 5 keys)
**Commit:** `feat(O-Strata): Phase 2.1 — live terrain oscillator at 1x, orbit/terrain libraries, mod wiring, wavetable path removed, harness`

---

### Phase 2.2: Pitch tracking, trajectory feedback, saturation gates

**Goal:** DSP-01 and FUNC-04 complete and gated.

**Tasks:**
- Pitch tracking law (ARCHITECTURE Algorithm "Pitch tracking": F_eff = F_mod · min(1, f_ref/f_note)^track, f_ref = C4, block-rate `pow`), Terrain Freq log-domain mod.
- TrajectoryFeedback (Core 4): per-partial `d, y1, y2`, two-sample average, damp law a = 1 − 2^(−1 − 9·Damp) with rate / OS correction, clamps, NaN scrub, note-on reset, û = rotation vector; harness switch `feedbackPathEnabled`.
- `CycleCapture` writes the displaced point.
- Harness gates H3, H4.

**Test Criteria:**
- [ ] H4: Track 1 → partials above −40 dB at C2 / C4 / C6 differ ≤ 2 at F = 1 and F = 4; Track 0 at F = 8, C6 raises non-harmonic energy ≥ 20 dB (DSP-01)
- [ ] H3: full Feedback × Damp × terrain × orbit × pitch grid finite, |y| ≤ 1, |DC| < 1e-3; Feedback 0 bit-identical to the path removed (FUNC-04)
- [ ] Saturation 0 bit-identical to bypass; Saturation 1 raises partial count (DSP-07)
- [ ] Damp 0 with Feedback 1 on Ridged Cosines + Epitrochoid 5: no Nyquist-bin dominance (the anti-hunting average works; negative control: harness flag `singleSampleFeedback` shows a Nyquist peak ≥ 20 dB higher)
- [ ] H2 and H5 still green

**Requirements verified:** DSP-01, FUNC-04, DSP-04 (clamp half), DSP-07
**Commit:** `feat(O-Strata): Phase 2.2 — pitch-tracked terrain frequency, trajectory feedback, gates`

---

### Phase 2.3: Per-oscillator oversampling (2× / 4×), CPU and aliasing gates

**Goal:** DSP-03 / FUNC-06 for the 2× and 4× paths; QUAL-01 at 2×; PERF-02.

**Tasks:**
- `dsp/HalfbandDecimator.h` (Core 5): per-sample polyphase-IIR allpass structure, coefficients from `FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod` in `prepare` (2× stage tw 0.06 / −70 dB; 4×→2× stage tw 0.15 / −60 dB), both stages pre-allocated per partial; latency computed from the coefficients and asserted ≤ 2.
- Oversampling loop in `getNextSampleStereo` (parameters held over sub-samples; feedback per sub-sample with a_eff); Quality switch mid-note = enum flip + state reset + 64-sample crossfade.
- Latency: constant +1 sample added to the distortion latency in `prepareToPlay` and `timerCallback`.
- The two planned savings (Decision 11): SmoothedValue ramps shared per oscillator in the processor (read by all voices), feedback update skipped when fb = 0 for the whole block.
- Harness gates H6 (2× / 4× rows), H7 (`terrainKernelBypass` baseline + delta), H8 across Quality changes; `tests/exports/` WAV grid for the listening pass (gitignored; goldens as `.sha256`).

**Test Criteria:**
- [ ] H6: every terrain × orbit at defaults, C2 / C4 / C6, 2×, Track 1 → non-harmonic ≤ −60 dB; 4× ≤ −90 dB on the same grid reported (QUAL-01)
- [ ] H7: oscillator delta ≤ 12 % at the default patch (16 v × 2 osc, unison 1, 2×, 48 kHz, 10 s), total and baseline printed; unison 4 and 4× reported (PERF-02)
- [ ] H8: zero allocations while Quality toggles Bandlimited→2×→4× every 50 ms under 16 held notes (DSP-05, FUNC-06)
- [ ] Quality switch mid-note: no sample step > 0.1 beyond the 64-sample crossfade (FUNC-06)
- [ ] `getLatencySamples()` = distortion latency + 1 in every Quality combination; reported from `prepareToPlay` (FUNC-06)
- [ ] Harness at 44.1 / 48 / 96 kHz: H6 numbers within 3 dB of each other (decimator design is rate-normalised)
- [ ] If H6 fails on a high-K orbit: apply the fallbacks in ARCHITECTURE "Implementation Risks" in order and re-run; if H7 fails after the savings: float terrain phase path, re-run; record the outcome in `stages/2-dsp/SUMMARY.md`

**Requirements verified:** DSP-03, FUNC-06 (2× / 4× half), QUAL-01 (2× half), PERF-02, DSP-05 (Quality no-alloc)
**Commit:** `feat(O-Strata): Phase 2.3 — per-oscillator 2x/4x halfband oversampling, aliasing + CPU gates`

---

### Phase 2.4: Bandlimited (Chebyshev) mode and the ChebyshevScheduler

**Goal:** DSP-02 complete: projector, truncation, scheduler, atomic swap, crossfade, readout data.

**Tasks:**
- `dsp/ChebyshevSet.h` (153-coefficient triangle, triangular Clenshaw, D_max mask with the 2-diagonal raised-cosine taper), `dsp/ChebyshevProjector.h/.cpp` (Chebyshev–Gauss quadrature, adaptive M, separable contraction, fit metric).
- `ChebyshevScheduler` (Core 7): 50 ms timer, `ChebKey` (terrain, F clamped ≤ 2, Mod X / Y with ModWheel / Aftertouch folded, blur, edge, image generation), 50 ms debounce, one cancellable job per oscillator, `callAsync` publish into `chebPtr[osc]`, `retire`, `chebGeneration`, `chebFit`; only active while Quality = Bandlimited.
- Oscillator: Clenshaw path at OS = 1, Pitch Track inert, F clamp, per-voice D_max from K and f_note, one-block crossfade on pointer change; per-voice terrain-destination bypass in this mode.
- Readout data: `partialsAtC4 = min(16, D_max(C4)) · K` and fit % exposed as atomics for Stage 3.
- Harness gate H6 Bandlimited rows; a Chebyshev-swap storm test (key changes every 20 ms for 5 s under 16 held notes: no allocation on the audio thread, no NaN, no click > 0.1 outside crossfades, ≤ 1 job in flight per oscillator, zero leaks under ASan).

**Test Criteria:**
- [ ] H6: Bandlimited + Ellipse / Limaçon / Epitrochoid 3/5/7 / Hypocycloid 3/5/7 at C2 / C4 / C6: non-harmonic ≤ −90 dB and max harmonic index = D_max · K (DSP-02, FUNC-06, QUAL-01)
- [ ] Fit readout matches the Stage 0 table within 2 % (Sine Product 100 % at F = 1, ≈ 24 % at F = 4 is unreachable because of the clamp — the clamp is asserted)
- [ ] Swap storm: zero audio-thread allocations, no NaN, superseded jobs cancelled, old sets freed ≥ 2 generations later, no leak
- [ ] ModWheel → Terrain Freq route changes the set within 100 ms; an LFO → Terrain Mod X route is inert in this mode and live in 2× (documented behaviour)
- [ ] Feedback > 0 or a non-K orbit flips the "approximate" flag
- [ ] `prepareToPlay` / `setStateInformation` publish nothing (assert)

**Requirements verified:** DSP-02, FUNC-06 (Bandlimited half), QUAL-01 (Bandlimited half), PERF-01 (scheduler)
**Commit:** `feat(O-Strata): Phase 2.4 — Chebyshev bandlimited mode, coefficient scheduler, atomic swap`

---

### Phase 2.5: PNG terrain path (DSP side), edge gates, harness consolidation

**Goal:** the `Imported…` terrain works end-to-end through a processor API (UI in Stage 3): decode, blur, edge, bilinear read, Chebyshev projection at import, publish / retire.

**Tasks:**
- `TerrainImage` (Core 8) + import job (decode via `ImageFileFormat`, ≤ 1024² downsample, luminance, separable Gaussian blur, edge function, 64² node-matched projection, 64 × 64 view heightmap), `imagePtr[osc]` publish, `imageGeneration`, Blur / Edge re-run with 150 ms debounce; processor API `importTerrainImage(osc, MemoryBlock bytes, name)` and `importTerrainFile(osc, File)` (message thread).
- Oscillator image path: bilinear over the pre-blurred copy, Mirror / Window, F scaling through the edge rule.
- Harness gates H10 (persistence half — the bytes path only; state child arrives in 4.1, so H10 here asserts identical renders for identical bytes) and H11; the complete `tests/render-harness/README` listing H1–H11 with their commands (executable as written — memory `pattern_recorded_gate_command_not_executable_as_spelled`).
- `stages/2-dsp/SUMMARY.md` records every measured number (H6 table, H7 total / baseline / delta, latencies).

**Test Criteria:**
- [ ] A 512² greyscale PNG imported through the API renders; Blur 0 → 1 lowers the spectral centroid monotonically; Mirror vs Window differ audibly at r = 1 (FUNC-07 DSP half)
- [ ] H11: hard-edged PNG, Mirror and Window, orbit crossing the border: no edge partial above −40 dB (DSP-04)
- [ ] Bandlimited + image: fit % reported; non-harmonic ≤ −90 dB with Ellipse (the image path is exact once projected)
- [ ] Import job: decode + blur + projection ≤ 100 ms for 1024² on M-series; never on the audio or message thread (assert)
- [ ] Identical bytes ⇒ identical 1 s render SHA-256 across two processor instances
- [ ] All of H1–H9 green in one `render-harness --all` run; runtime ≤ 3 min

**Requirements verified:** DSP-04, FUNC-07 (DSP half), FUNC-08 (bytes determinism), all Stage-2 IDs re-verified
**Commit:** `feat(O-Strata): Phase 2.5 — PNG terrain path, edge gates, harness consolidated`

---

## Stage 3: GUI Phases

### Phase 3.1: Oscillator panels, Terrain tab controls, readout, ≋ cycle view, i18n

**Goal:** the v2 mockup styled and bound: Terrain▾ / Orbit▾ / Quality, all knobs, greyed Blur / Edge, the readout, the Synth-tab ≋ last-cycle view; localised in en / fr / zh-Hans.

**Tasks:**
- `ui/public/index.html` + CSS from `mockups/v2-ui.html` (O-Prism class names, Garamond stack, vine-arc knobs), Terrain tab replacing the Wavetable tab slot.
- Native functions: `getTerrainStatus(osc)` → `{quality, partialsAtC4, fitPct, exact, sourceMissing, terrainModRate}`; events `terrainCycle` from `CycleCapture` (base64 Float32, 512 points) via `emitEventIfBrowserIsVisible`, never `evaluateJavascript` for the new pushes.
- Mod-destination dropdown: the 46 names through the existing `getModDestNames` native function with the i18n mapping the tuning / mod tabs already use; relabelled entries 1/2.
- i18n keys for every new label, tip and readout string (glossary first, then lint; French reviewed by Taylor; zh-Hans through the corpus rules).
- UI gates: check-ui-labels (including width-pinned `data-i18n`), tips render check, boot-all-uis, measure-ui font faces via CDP.

**Test Criteria:**
- [ ] Every one of the 34 new parameters moves from the UI and follows host automation; preset apply updates all controls
- [ ] Readout shows "2× · N partials at C4" / "Bandlimited · fit 99 % · exact" / "approximate" according to the processor atomics; Blur / Edge grey unless Imported
- [ ] ≋ view shows the last cycle; ⬡ view placeholder present (3D in 3.2)
- [ ] i18n lint exit 0 in three languages; no untranslated `data-i18n`; readout nodes are never localised (third arm of the localize test)
- [ ] UI gates green; no visual regression on the inherited tabs (screenshot diff against O-Prism's tabs)

**Requirements verified:** UI-04 (controls half), FUNC-05 (list in the UI)
**Commit:** `feat(O-Strata): Phase 3.1 — Terrain tab, oscillator panels, readout, cycle view, i18n`

---

### Phase 3.2: 3D terrain view (WebGL2 + Canvas 2D), playhead, feedback trail, preset re-push

**Goal:** UI-01, UI-02, UI-04, PERF-03.

**Tasks:**
- `ui/public/js/terrain-view.js` from `webgl-3d/terrain-proto.html`: R32F heightmap texture (64 × 64 from `terrainHeightmap`), orbit polyline computed in JS from the orbit parameters (formulas ported from `dsp/Orbits.h`; golden check against a C++ dump of 512 points per orbit at m ∈ {0, 0.5, 1}), scan point from `terrainPlayhead`, trail from `terrainCycle` when Feedback > 0; Canvas 2D fallback in the same file; `webglcontextlost` / `restored`; no GLSL `flat`; Retina backing per draw; redraw only on dirty state.
- Processor → editor: `terrainHeightmap` (≤ 10 Hz on change), `terrainPlayhead` (30 Hz, change-gated), `terrainCycle` (30 Hz while visible), `terrainStatus`; re-push after preset apply / `setStateInformation` (`chebGeneration` / `imageGeneration` / preset counter change).
- "WebGL unavailable" placeholder as a `data-i18n` node (en / fr / zh-Hans).
- PERF-03 instrumentation: in-page `performance.now()` around draw, mean / max over 300 frames, reported through a debug native function.

**Test Criteria:**
- [ ] Renders in Standalone, Logic (WKWebView) and a Windows host (WebView2); forcing `getContext('webgl2')` → null shows the Canvas 2D fallback + localised placeholder in all three languages; `WEBGL_lose_context` recovers (UI-01, UI-02)
- [ ] Playhead advances with θ at 30 Hz; unchanged state sends no event; the trail is visible only when Feedback > 0 and matches the harness's captured trajectory (UI-01)
- [ ] After a preset apply and after a session restore with the editor open, the view shows the new terrain within one push interval (UI-04)
- [ ] PERF-03: ≤ 2 ms mean per frame at DPR 2 in WKWebView and WebView2 (Chromium numbers are not evidence)
- [ ] Orbit polyline golden: JS vs C++ ≤ 1e-4 for all 11 orbits × 3 m values

**Requirements verified:** UI-01, UI-02, UI-04, PERF-03
**Commit:** `feat(O-Strata): Phase 3.2 — 3D terrain view, playhead, feedback trail, preset re-push`

---

### Phase 3.3: View interaction and PNG import (chooser + drag-and-drop)

**Goal:** UI-03, FUNC-07 complete.

**Tasks:**
- Drag on the view → Orbit Centre X/Y (unproject to the y = 0 plane), wheel `{passive:false}` → Orbit Size, alt-drag → Rotation, alt-click → reset; all through `Juce.getSliderState` with `sliderDragStarted` / `setNormalisedValue` / `sliderDragEnded`; document-level move / up (pointer capture does not work in the JUCE WebView).
- Import… button → `FileChooser::launchAsync` (SafePointer, no `complete()` inside the callback) → `importTerrainFile`; drag-and-drop on either view (macOS `webkitGetAsEntry`, Windows `dataTransfer.files`) → base64 chunks (≤ 1 MB) → `importTerrainImage`; "Drop PNG" overlay; progress from the job's atomic; the amber "Source missing" notice bound to `terrainStatus.sourceMissing`.
- Terrain dropdown switches to `Imported…` on a successful import; Blur / Edge un-grey.

**Test Criteria:**
- [ ] Dragging the view moves Centre X/Y through the relay with one host undo step; wheel edits Size; alt-drag edits Rotation; alt-click resets (UI-03)
- [ ] PNG loads via chooser and via drag-and-drop on macOS and Windows; Blur and Edge are audible and visible in the 3D view (FUNC-07)
- [ ] A 10 MB PNG is refused with a notice; an undecodable file keeps the previous terrain with a notice
- [ ] Hidden-view completion gate: importing while the editor is closed still publishes (completions are dropped only for UI events, never for the import job)

**Requirements verified:** UI-03, FUNC-07
**Commit:** `feat(O-Strata): Phase 3.3 — view interaction, PNG import via chooser and drag-and-drop`

---

## Stage 4: Polish / Validation

### Phase 4.1: Persistence, fallback notice, factory presets

**Goal:** FUNC-08, QUAL-03, FUNC-11.

**Tasks:**
- `terrainImports` save / restore (ARCHITECTURE "State Persistence"): raw PNG base64 ≤ 2 MB per oscillator, else path + SHA-256 + name; string-typed property reads; preset-manager `customSave` / `customLoad`; `imageGeneration++` on load; missing / changed source → Sine Product fallback + notice with "Locate…".
- Factory presets ≥ 15 (`FactoryPresets.cpp` rewritten; factory version bump): the five BRIEF use cases (breathing pad, feedback lead, microtonal Bandlimited bell, BYO-terrain placeholder using a library terrain, phase-distortion organ) plus coverage of every terrain, every orbit family, feedback ≥ 3, Bandlimited ≥ 3; categories per preset-manager layout; every preset passes H2 (harness loads `FactoryPresets::build`).
- Harness H10 full (state child), H2 over presets.

**Test Criteria:**
- [ ] Save / reload a preset with an embedded PNG: identical 1 s render SHA-256 (FUNC-08)
- [ ] Above the cap: path + SHA; file present → regenerates; missing → fallback + visible notice, never silence or crash (FUNC-08, QUAL-03)
- [ ] All ≥ 15 factory presets load, pass H2, and cover terrains × orbits × feedback × Bandlimited as listed (FUNC-11)
- [ ] Session restore with the editor open re-pushes the view (UI-04 regression)
- [ ] Preset file with two embedded 2 MB PNGs loads in < 1 s

**Requirements verified:** FUNC-08, FUNC-11, QUAL-03
**Commit:** `feat(O-Strata): Phase 4.1 — PNG persistence, fallback notice, factory presets`

---

### Phase 4.2: CI Windows build, listening pass, changelog, install

**Goal:** release-ready v1.0.0.

**Tasks:**
- CI Windows VST3 build (WebView2 static linking, `withUserDataFolder()` inherited from O-Prism); pluginval strictness 10 on Windows in CI (catches NaN that auval misses).
- Listening pass: the Phase 2.3 `tests/exports/` terrain × orbit grid, the factory presets in Logic / Live / Standalone; tune library defaults (orbit-mod ranges, feedback damp law constants) only if something is dull or harsh — defaults only, every change re-runs H2 / H3 / H6 / H7.
- Final pluginval + auval on macOS; cross-DAW smoke; `auval -a | grep -i strata` after a fresh install with the dual-variant sweep.
- CHANGELOG v1.0.0 (lists Decisions 5, 6, 8, 11 as user-visible behaviour: constant 1-sample latency, no wavetable mode, 2× default, Bandlimited-mode limits); NOTES.md (Terrain attribution, licence); PLUGINS.md → ✅ Working / 📦 Installed after `build-and-install.sh`.

**Test Criteria:**
- [ ] CI Windows build green; VST3 loads in a Windows host with the 3D view (COMPAT-02)
- [ ] pluginval strictness 10 VST3 + AU pass; auval pass (COMPAT-01 regression)
- [ ] Listening pass signed off on every terrain × orbit default and every factory preset (QUAL-04)
- [ ] No stale `-dev` / release variant shadowing after install
- [ ] Open question on PERF-02 wording resolved and reflected in REQUIREMENTS.md

**Requirements verified:** COMPAT-02, QUAL-04, COMPAT-01 (regression), all remaining
**Commit:** `release(O-Strata): v1.0.0 — CI Windows, listening pass, changelog`

---

## Implementation Flow Summary

```
Stage 0  ✓ research + plan (v2, 2026-09-08)
Pre-1    mockup v2 → parameter-spec.md v2 (lock)                       ← NEXT
Stage 1  second pass: re-parameterise the fork, COMPAT-01
Stage 2  2.1 core at 1× + libraries + mod wiring + wavetable deletion + harness
         2.2 pitch tracking + feedback
         2.3 2×/4× oversampling + aliasing + CPU gates
         2.4 Bandlimited mode + scheduler
         2.5 PNG path + edge gates + harness consolidation
Stage 3  3.1 controls + readout + cycle view + i18n
         3.2 3D view + playhead + trail + re-push
         3.3 interaction + import
Stage 4  4.1 persistence + presets
         4.2 CI Windows + listening + release
```

---

## Implementation Notes

### Thread Safety
- APVTS reads via cached atomics; the only new audio-thread reads of shared pointers are `chebPtr[]` / `imagePtr[]` with `load(acquire)` once per block.
- Publish and retire on the message thread only; the generation-counted reaper (existing) frees ≥ 2 block generations after retirement — this also covers the one-block Chebyshev crossfade.
- `FilterDesign` and the Superellipse LUT only in `prepare`; import / projection only in the ThreadPool.
- `CycleCapture`: single writer (display voice), single reader (editor timer).

### Performance
- Model (ARCHITECTURE "CPU budget model"): default patch ≈ 101 ns per partial per base sample before the Phase 2.3 savings (≈ 15 % oscillator delta at 16 × 2), ≈ 74 ns after (≈ 11.4 %); Bandlimited ≈ 88 ns; worst 2× ≈ 165 ns. Unison cap 4.
- The gate is the oscillator delta (H7), total and baseline printed.

### Latency
- Distortion oversampler (existing, bypass-aware) + constant 1 sample for the oscillator path; `setLatencySamples` from `prepareToPlay` and the 500 ms timer.

### Denormal Protection
- `juce::ScopedNoDenormals` in `processBlock` (existing); the decimator allpasses and the DC blocker run under it.

### Known Challenges
- Aliasing on high-K orbits at 2× (Risk 1) — fallbacks ordered in ARCHITECTURE.
- CPU over 12 % on first measurement (Risk 2) — two planned savings, then the float phase path.
- Bandlimited-mode expectations (Risk 3) — the readout is the mitigation; F-lattice in v1.1.
- The symmetry gate is not monotone in Terrain Freq — every factory preset is gated individually; the harness prints neighbouring-centre h1 values for fixing.
- Two sessions share this checkout: path-scope every commit (`git commit -- plugins/O-Strata PLUGINS.md`), re-check `git status --short` immediately before each commit.

---

## Dependencies

### Existing Code to Reuse
- `plugins/O-Strata/Source/` (verified fork): voice loop, mod matrix, filters, FX, tuning, preset manager, WebView shell, reaper (`PluginProcessor.cpp:994-1024`), Sync / Window / Bend / FM warp code (`dsp/WavetableOscillator.cpp:183-268`, copied into `TerrainOscillator` before the file is deleted).
- `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/kernels.h` (halfband allpass structure, Clenshaw, orbit / terrain kernels as reference), `alias.cpp` (exact-cycle spectral harness), `webgl-3d/terrain-proto.html` (view).
- `plugins/O-Bowed/tests/render-harness/` (console-app pattern), `stages/1-foundation/smoke/` (headless processor pattern, `JUCE_WEB_BROWSER=0`).
- `superseded-baked-v1/ARCHITECTURE.md` Core 5 / Core 8 / State Persistence / 3D view (cited, adapted).

### External Dependencies
- JUCE 8.0.14 (juce_dsp `FilterDesign`, `FastMathApproximations`; juce_cryptography; juce_graphics PNG) — all already linked.
- scala-tuning-engine v3.0.1, preset-manager v1.0.6, note-expression module — inherited.

---

## Critical Path

Mockup v2 → spec v2 lock → Stage 1 second pass (COMPAT-01) → 2.1 (parity + symmetry + harness) → 2.2 → **2.3 (the CPU / aliasing gates decide whether the default design holds)** → 2.4 → 2.5 → 3.1 → 3.2 → 3.3 → 4.1 → 4.2. Phase 2.3 is the decision point: if H6 / H7 need the fallbacks, apply them there before Bandlimited and images are layered on.

---

## Requirements Traceability

| ID | Requirement (short) | Stage / Phase | Gate |
|----|---------------------|---------------|------|
| FUNC-01 | Live terrain oscillator, θ parity | 2.1 | H1 |
| FUNC-02 | 6 analytic terrains, Mod X / Y | 2.1 | centroid-delta check |
| FUNC-03 | 11 orbits, Size / Aspect / Rotation / Centre / Orbit Mod | 2.1 | centroid monotonic, Orbit Mod delta |
| FUNC-04 | Trajectory feedback bounded | 2.2 | H3 |
| FUNC-05 | 10 new destinations per osc, per-sample; 46-entry list | 1 (list), 2.1 (wiring), 3.1 (UI) | H5 + sideband proof |
| FUNC-06 | Quality Bandlimited / 2× / 4× | 2.3, 2.4 | H6, H8, latency check |
| FUNC-07 | PNG import, Blur, Edge, off-thread decode | 2.5 (DSP), 3.3 (UI) | import checks |
| FUNC-08 | PNG persists, ≤ 2 MB, path + SHA above | 2.5 (bytes), 4.1 (state) | H10 |
| FUNC-09 | Inherited sections unchanged | 1 (IDs), 2.1 (regression) | params.tsv diff |
| FUNC-10 | Tuning engine | 2.1 (regression) | Scala / 31-EDO pitch check |
| FUNC-11 | ≥ 15 factory presets, symmetry-gated | 4.1 | H2 over presets |
| DSP-01 | Pitch-tracked terrain frequency | 2.2 | H4 |
| DSP-02 | Bandlimited Chebyshev mode | 2.4 | H6 (Bandlimited rows), swap storm |
| DSP-03 | Per-oscillator oversampling | 2.3 | H6, rate sweep |
| DSP-04 | Clamp, pre-blur, edge, isfinite | 2.2 (clamp), 2.5 (edge) | H3, H11 |
| DSP-05 | No pow / std::function / alloc; Quality no-alloc | 2.1, 2.3 | grep gate, H8 |
| DSP-06 | Symmetry rule | 2.1 | H2 |
| DSP-07 | Saturation, identity at 0 | 2.2 | bypass identity |
| UI-01 | 3D view, playhead, trail | 3.2 | host checks |
| UI-02 | Canvas 2D fallback, localised placeholder | 3.2 | forced-null check |
| UI-03 | Drag / wheel / alt-drag through relay | 3.3 | undo-step check |
| UI-04 | Re-push after preset / restore | 3.1, 3.2, 4.1 | push-interval check |
| PERF-01 | RT-safe processBlock | 2.1, 2.4 | H8, scheduler asserts |
| PERF-02 | CPU ≤ 12 % (oscillator delta), unison cap 4 | 2.3 | H7 |
| PERF-03 | 3D view ≤ 2 ms | 3.2 | in-page timing |
| COMPAT-01 | pluginval 10 + auval after re-parameterise | 1 (4.2 regression) | pluginval / auval |
| COMPAT-02 | CI Windows VST3 | 4.2 | CI |
| QUAL-01 | Aliasing ≤ −60 dB (2×) / ≤ −90 dB (Bandlimited) | 2.3, 2.4 | H6 |
| QUAL-02 | No zipper on any FUNC-05 destination | 2.1 | H5 |
| QUAL-03 | Missing PNG → fallback + notice | 4.1 | H10 missing-file branch |
| QUAL-04 | Listening pass | 4.2 | sign-off |

---

## References

- Creative brief: `plugins/O-Strata/.planning/BRIEF.md` (v2)
- Requirements: `plugins/O-Strata/.planning/REQUIREMENTS.md` (v2.0.0)
- Parameter spec: `plugins/O-Strata/.planning/parameter-spec-draft.md` (v2) → `parameter-spec.md` v2 after mockup
- DSP architecture: `plugins/O-Strata/.planning/research/ARCHITECTURE.md` (v2)
- Evidence: `plugins/O-Strata/.planning/evidence/replan-proposal-2026-09-08.md`, `critique-check-2026-09-08.md`
- Research: `research/wavetable-synthesis-3d-geometry.md` §1, §5, §6, §7.2, §7.3; prototypes under `research/wavetable-synthesis-3d-geometry-prototypes/`
- Superseded plan: `plugins/O-Strata/.planning/superseded-baked-v1/` (v1.1 Baked source candidate)
- O-Prism — engine, shell, publish / reaper, preset system: `plugins/O-Prism/.planning/research/ARCHITECTURE.md`, `plugins/O-Prism/.planning/ROADMAP.md`
- O-Bowed — render-harness console-app pattern: `plugins/O-Bowed/tests/render-harness/`
