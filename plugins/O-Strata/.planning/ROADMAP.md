# O-Strata - Implementation Plan

**Date:** 2026-09-07
**Complexity Score:** 5.0 (Very High, Capped — raw 23.0)
**Strategy:** Phase-based implementation (staged DSP and GUI)

**Contracts:** `BRIEF.md`, `REQUIREMENTS.md` (27 IDs), `parameter-spec-draft.md` (217 params), `research/ARCHITECTURE.md`. Every phase below names the requirement IDs it verifies; the traceability table in REQUIREMENTS.md (stage-1: COMPAT-01; stage-2: FUNC-01..05, 09, 10, 12, DSP-*, PERF-01/02, QUAL-01/02; stage-3: FUNC-06/07, UI-*, PERF-03; stage-4: FUNC-08, FUNC-11, COMPAT-02, QUAL-03/04) is honoured phase by phase.

---

## Complexity Factors

- **Parameters:** 217 parameters (217/5 = 43.4, capped at 2.0) = **2.0**
  - 171 inherited from O-Prism v1.24.0 + 46 new Geometry parameters (23 × Osc A/B)
- **Algorithms:** 16 DSP components = **16**
  - New (7): MeshSlicer, VolumeOrbitSampler, TerrainOrbitSampler, FrameAligner + FrameNormaliser, GeometryBakeScheduler, GeometryImporter (OBJ / STL / PNG parsers + welder + blur), BuiltInGeometry (procedural meshes + analytic terrains)
  - Inherited unchanged (9): WavetableOscillator + mipmap engine, SubOscillator, NoiseGenerator, SVFFilter ×2, ADSREnvelope ×2, UnisonEngine, EffectsChain (5 FX), LFO ×4 + ModulationMatrix, TuningEngine (scala-tuning-engine v3.0.1)
- **Features:** 5 points
  - FFT / frequency domain (cross-correlation alignment + mipmaps) (+1)
  - Modulation system (4 LFOs, mod matrix, filter envelope — inherited) (+1)
  - External MIDI control (synth voice, note expression, mod wheel / aftertouch — inherited) (+1)
  - File I/O (OBJ/STL/PNG import, gzip+base64 state embedding, Scala/KBM — new + inherited) (+1)
  - Real-time visualization (3D geometry view + 30 Hz playhead in WebView) (+1)
- **Raw Total:** 2.0 + 16 + 5 = **23.0**
- **Final Score:** min(23.0, 5.0) = **5.0** (capped at maximum)

**Assessment:** Raw 23.0 is the highest in the catalogue (O-Prism 14.0). The score overstates *audio-thread* risk — the baked design adds no audio code — and understates *integration* work: a new threading system, three parsers, two-branch persistence, a WebGL view and three-language UI strings on top of the largest existing engine. The phasing therefore front-loads the pipeline plumbing (2.1) so every later phase is a generator or a view dropped into a proven harness.

---

## Stages

- Stage 0: Research and Planning — COMPLETE (2026-09-07)
- Stage 1: Foundation (fork, params, build, validation) — Next (requires UI mockup + full `parameter-spec.md` first)
- Stage 2: DSP — 5 phases (see below)
- Stage 3: GUI — 3 phases (see below)
- Stage 4: Polish / Validation — 2 phases (see below)

**Workflow ≠ research numbering:** research §7.4 "Stage 1 baked generators" = workflow Stage 2; research "Stage 3 3D view" = workflow Stage 3; research "Stage 2 live terrain oscillator" = v1.1 (not in this plan).

---

## Stage 1: Foundation

**Goal:** A buildable, validating O-Strata that is O-Prism v1.24.0 minus the wavetable library plus 46 inert geometry parameters. No generator code yet; the oscillators play the sine placeholder.

**Tasks:**
- Fork `plugins/O-Prism/` → `plugins/O-Strata/` (Source, ui, CMakeLists, CHANGELOG stub, NOTES). `juce_add_plugin(O-Strata … PLUGIN_CODE OuSt PRODUCT_NAME "O-Strata${OUARICON_DEV_SUFFIX}" VERSION 1.0.0 …)` — `VERSION`, never `PLUGIN_VERSION` (memory: `critical_plugin_version_keyword_ignored_by_juce`). `OuSt` verified unused across `plugins/*/CMakeLists.txt`.
- Rename classes / namespaces (`OPrismAudioProcessor` → `OStrataAudioProcessor`, binary-data target `O-Strata_UIResources`, `PrismParamIds` may keep its name or become `StrataParamIds` — one decision, applied everywhere).
- Remove the library code paths (ARCHITECTURE Decision 5): `WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor`, `wavetable-editor.js/.css`, `oscATable` / `oscBTable`, the 14 native functions, their i18n rows (en/fr/zh-Hans), the user-wavetable ValueTree child, the `pOscATable/pOscBTable` caches, `resolveActiveTable` → `oscTablePtr[osc]`. Keep `WavetableGenerator`; initial table = `generateProceduralTable(Sine)`.
- Add the 46 geometry parameters to `createParameterLayout()` (draft IDs, ranges, defaults from `parameter-spec.md`), update `oscIds()` suffix list (drop `Table`, add 23), so `allSliderIds()` = 170 and relays are generated. Choice lists append-only with trailing `Imported`. Bake params are **not** added to `modSlot?Dst`.
- Link `juce::juce_cryptography` (SHA-256). Guard `createEditor()` with `#if JUCE_WEB_BROWSER … #else GenericAudioProcessorEditor` (memory: `pattern_render_harness_breaks_on_webview_editor`) so Stage 2 harnesses build headless.
- Add `GeometryBakeScheduler` **skeleton** (timer, BakeKey hashing, ThreadPool, publish/retire wiring) that bakes the sine placeholder — proves the thread contract before any geometry exists. (Optional here; mandatory in 2.1.)
- Stub `geometryImports` state child (empty) in `getStateInformation` / `setStateInformation`.
- UI: existing O-Prism `index.html` with the wavetable-selection UI removed and the Geometry block rendered from the mockup as plain knobs/dropdowns (full styling in Stage 3).
- CHANGELOG.md `## v1.0.0 (unreleased)`; PLUGINS.md row → 🚧 Stage 1.

**Test Criteria:**
- [ ] VST3 + AU + Standalone build clean on macOS (ninja `O-Strata_VST3 O-Strata_AU`); `./scripts/build-and-install.sh O-Strata` installs with the AU-cache sweep
- [ ] `auval -a | grep -i strata` lists the AU; `auval` passes
- [ ] pluginval strictness 10 passes VST3 and AU (COMPAT-01)
- [ ] Host automation list shows 217 parameters; `oscATable`/`oscBTable` absent; `grep -rn "WavetableFactory\|UserWavetableManager\|oscATable" Source/` returns nothing
- [ ] Param-dump target (`OUARICON_BUILD_TESTS=ON`) prints 217 IDs matching `parameter-spec.md`
- [ ] Notes play the sine placeholder through the unchanged voice/FX path; tuning tab loads a Scala file (smoke for FUNC-09/10, formally verified in 2.1)
- [ ] State save/reload round-trips all 217 parameters + tuning + uiLanguage

**Requirements verified:** COMPAT-01
**Commit:** `feat(O-Strata): Stage 1 foundation — fork of O-Prism v1.24.0, 46 geometry params, library removed`

---

## Stage 2: DSP Phases

All Stage 2 gates run in **offline harnesses** (`pattern_offline_dsp_render_harness`); the repo has no unit-test framework and CI runs no tests, so these are the only automated gates and they run locally. Two console targets under `plugins/O-Strata/tests/`, gated by `OUARICON_BUILD_TESTS`:
- `O-Strata_BakeHarness` — links the generator sources only (no processor, no editor): golden comparison, adjacent-frame RMS, negative control, NaN/peak, orbit clamp, timing.
- `O-Strata_RenderHarness` — instantiates the processor headless (`JUCE_WEB_BROWSER=0`, GenericAudioProcessorEditor fallback): drives MIDI, renders, measures QUAL-01/QUAL-02, pumps the message loop (`JUCE_MODAL_LOOPS_PERMITTED=1`) so the scheduler's timer and `callAsync` publish run.

### Phase 2.1: Bake pipeline end-to-end (trivial generator)

**Goal:** FrameSet → alignment → normalisation → mipmaps → atomic publish → retire, driven by the debounced scheduler, using the analytic Sine Product terrain with a fixed ellipse as the only generator. Every later phase plugs a generator into this.

**Tasks:**
- `FrameSet`, `FrameAligner` (FFT xcorr + polarity, chained, frame-0 canonical start), `FrameNormaliser` (DC → peak → drive → DC → align → peak), `WavetableData` fill + `generateMipmaps` + guard.
- `GeometryBakeScheduler` complete: 50 ms poll, BakeKey (active-family hashing), 150 ms debounce, ThreadPool (2 threads, named), cancellation, `callAsync` publish with weak token, `retireTable`, `bakeProgress`, `bakeGeneration`, destructor `removeAllJobs`.
- `TerrainOrbitSampler` minimal: Sine Product + Ellipse + Radius sweep (full terrain family in 2.4).
- Harness-only `alignmentEnabled` switch for the negative control.
- Rewire `getActiveOscInfo` / `getActiveOscFrame` to `oscTablePtr[osc]`.

**Test Criteria:**
- [ ] Changing any active-family bake parameter yields a new table within 150–250 ms; changing an inactive-family parameter yields no bake (BakeKey unchanged) (FUNC-01)
- [ ] 50 key changes in 100 ms → exactly one bake result published, superseded jobs cancelled, zero leaks (address sanitizer run) (PERF-02 scheduling)
- [ ] Audio at 16 voices continues without dropouts during continuous bakes (render harness: no block > 2× nominal time, no discontinuity) (PERF-02)
- [ ] Position 0→1 sweeps frame 0→N−1 with linear interpolation, N ∈ {64, 128, 256} (FUNC-01, FUNC-12)
- [ ] Sine Product / ellipse table: no NaN, global peak = 1.0, per-frame |DC| < 1e-4, per-frame peak normalisation absent from the code (DSP-05)
- [ ] Alignment on: adjacent-frame RMS ≤ 0.05; harness `alignmentEnabled=false` measurably raises it (DSP-04 mechanism proven before meshes)
- [ ] Shape Drive 0 is bit-identical to bypass; drive 1 raises partial count (DSP-07)
- [ ] `oscTablePtr` written only on the message thread (assert never fires under pluginval strictness 10) (PERF-01)
- [ ] Parameter IDs for envelope, filter, LFO, mod matrix, FX, tuning and global sections match O-Prism v1.24.0 (`params.tsv` diff = the two removed + 46 added) (FUNC-09)
- [ ] Tuning tab loads a Scala file and a 31-EDO generator; rendered pitch equals O-Prism's on the same tuning (FUNC-10)

**Requirements verified:** FUNC-01, FUNC-09, FUNC-10, FUNC-12, DSP-01 (pipeline path), DSP-04, DSP-05, DSP-07, PERF-01, PERF-02 (scheduling half)
**Commit:** `feat(O-Strata): Phase 2.1 — bake pipeline, scheduler, alignment/normalisation`

---

### Phase 2.2: Mesh slicer + built-in mesh library + golden test

**Goal:** The novel core. `MeshSlicer` with slab index, edge-key chaining, loop policy with hysteresis, arc-length unwrap (XY with φ, Centroid Distance behind the flatness floor), empty-frame handling; `BuiltInGeometry` procedural meshes matching the Python parametrisation.

**Tasks:**
- `TriangleMesh` (welded, normalised), `BuiltInGeometry::{sphere(200×100), torus, twistedStar(nu=128,nz=64,points=5,inner=0.45,outer=1.0,twist=π,height=2), crescent, torusKnot(2,3), fbmBlob(200×100)}`.
- `MeshSlicer` per ARCHITECTURE "Algorithm Details → Mesh slice → single cycle" (all 8 notes).
- Ugly-mesh corpus in the bake harness: open cylinder, two disjoint shells, flipped winding, sliver triangles, zero-area mesh.
- Regenerate the golden: run `research/wavetable-synthesis-3d-geometry-prototypes/mesh-slice/mesh_slice_wavetable.py` (its WAV outputs are not committed) and store `twisted_star_d_128.sha256` + a harness fixture path (goldens tracked as checksum only, memory: `pattern_golden_tracked_as_checksum_only`).

**Test Criteria:**
- [ ] Twisted star, 128 frames, Centroid Distance, Largest: C++ frames match the Python `twisted_star_d_128` within 1e-3 RMS per frame (DSP-02)
- [ ] Torus: 2 loops per slice, Largest with hysteresis never flips loops frame-to-frame (chosen-centroid jump < 0.1 across all frames) (FUNC-02, DSP-03)
- [ ] Sphere at φ ∈ {0, 45, 90}°: THD < −60 dB (pure sine) (FUNC-02); Centroid Distance on the sphere hits the flatness floor and falls back to XY (DSP-02)
- [ ] Twisted star Tilt X = 45° vs 0°: spectral centroid differs by > 10 % (FUNC-02)
- [ ] Crescent φ = 0° vs 90°: two distinct tables (frame-0 correlation < 0.9) (FUNC-02)
- [ ] Negative control: `alignmentEnabled=false` raises torus adjacent-frame RMS from ≤ 0.034 to ≥ 0.4 (DSP-04)
- [ ] All six meshes at 256 frames: no NaN, no frame > 0 dBFS, adjacent-frame RMS ≤ 0.05 (FUNC-05, QUAL-02 precondition)
- [ ] Ugly-mesh corpus: bakes complete with warning flags set, no crash, no NaN (FUNC-06 robustness, ahead of import UI)
- [ ] Sum policy on the torus: adjacent RMS ≤ 0.05; concatenation not present in the code

**Requirements verified:** FUNC-02, FUNC-05 (mesh half), DSP-02, DSP-03, DSP-04 (negative control)
**Commit:** `feat(O-Strata): Phase 2.2 — mesh slicer, built-in mesh library, golden test`

---

### Phase 2.3: Volume generator

**Goal:** `VolumeOrbitSampler` with the five fields, four orbits, three sweep axes, tanh shaping; deterministic 3D gradient noise.

**Tasks:**
- SDF torus/box/sphere, gyroid, fBm (custom noise, constant seed), domain scale by Field Detail.
- Torus-knot (2,3)/(3,5)/(5,7) and Lissajous-knot orbits; Orbit Scale / Knot Phase / Z Offset sweeps.

**Test Criteria:**
- [ ] Torus Knot (2,3) through Torus SDF: max adjacent-frame RMS ≤ 0.05 (the §7.1 sign-flip case is fixed by the polarity test) (FUNC-03)
- [ ] fBm Noise: Field Detail 0.3 → 0.8 increases the count of partials above −40 dB (FUNC-03)
- [ ] Every field × orbit × sweep-axis combination at defaults: no NaN, peak = 1.0, bake ≤ 200 ms at 256 frames
- [ ] Same parameters twice → identical bytes (determinism for FUNC-08)

**Requirements verified:** FUNC-03
**Commit:** `feat(O-Strata): Phase 2.3 — volume orbit generator (SDF, gyroid, fBm)`

---

### Phase 2.4: Terrain generator (analytic + PNG sampling)

**Goal:** Full `TerrainOrbitSampler`: five analytic terrains, four orbits, four sweep axes, aspect/rotation/centre, radius clamp, PNG heightmap sampling with Gaussian pre-blur and Mirror/Window edges. PNG *decode* from bytes is implemented here (harness feeds files); the import *UI* is Stage 3.3.

**Tasks:**
- Analytic terrains (clean-room formulas per ARCHITECTURE), trig-polynomial orbits + superellipse, clamp-by-radius with debug assert.
- `TerrainMap` from PNG bytes via `juce::ImageFileFormat::loadFrom` → luminance float grid; separable Gaussian blur cache; bilinear sampler; Mirror / Window.

**Test Criteria:**
- [ ] Ellipse over Sine Product, Sweep Axis = Radius: spectral centroid monotonically non-decreasing across frames (FUNC-04)
- [ ] Orbit never leaves [−1,1]² for a grid of Sweep Range × Centre × Aspect × Rotation (assert + harness max |x|,|y| ≤ 0.999) (FUNC-04, DSP-06)
- [ ] 512² hard-edged PNG, Mirror, blur 0.2: no partial above −40 dB attributable to the edge crossing (compare against the same orbit on the mirrored-continuous analytic extension) (DSP-06)
- [ ] Window mode: terrain → 0 at |x|,|y| = 1; orbit tangent to the edge produces no discontinuity
- [ ] All five analytic terrains × four orbits at defaults: no NaN, peak = 1.0, adjacent RMS ≤ 0.05 (FUNC-05 terrain half)
- [ ] Blur / Edge parameters are inert for analytic terrains (BakeKey excludes them when `Terrain ≠ Imported`)

**Requirements verified:** FUNC-04, FUNC-05 (terrain half), DSP-06
**Commit:** `feat(O-Strata): Phase 2.4 — terrain orbit generator, analytic library, PNG sampling`

---

### Phase 2.5: Offline harness gates (quality + performance)

**Goal:** Turn the REQUIREMENTS acceptance criteria into PASS/FAIL harness runs and export the library WAVs for the Stage 4 listening pass.

**Tasks:**
- Render harness: every library entry (6 meshes × defaults, 5 fields × default orbit, 5 terrains × default orbit) at C2, C4, C6; non-harmonic energy measurement (harmonic-bin masking, 8192-pt FFT, Blackman-Harris); Position sweep 0→1 over 2 s at C4 with sample-step detector.
- Bake harness timing on Apple M-series: fBm blob 256 frames (generator + conditioning, mipmaps reported separately), synthetic 1 M-triangle UV sphere including parse from a generated OBJ file.
- Export `tests/exports/<entry>.wav` of level-0 frames (not committed; listening pass artefact).
- Debug-allocator run of `processBlock` under continuous bakes.

**Test Criteria:**
- [ ] Non-harmonic energy at C6 ≤ −90 dB relative to the fundamental on every library entry; no NaN at C2/C4/C6 (QUAL-01, DSP-01)
- [ ] Position sweep: no sample-to-sample step > 0.1 in the rendered output on every library entry (QUAL-02)
- [ ] Adjacent-frame RMS after alignment ≤ 0.05 on the whole built-in library (QUAL-02)
- [ ] fBm blob (40 k tris) 256-frame bake: generator + conditioning ≤ 100 ms; full bake including mipmaps reported (target ≤ 200 ms) (PERF-02)
- [ ] 1 M-triangle OBJ: parse + bake ≤ 2 s (FUNC-06 budget, ahead of the import UI)
- [ ] Zero allocations in `processBlock` under the debug allocator while bakes run; 16 voices, no dropouts (PERF-01, PERF-02)
- [ ] The oscillator source files diff clean against O-Prism v1.24.0 except table-source plumbing (`git diff --stat` on `WavetableOscillator.*`, `PrismVoice.*`) (DSP-01)

**Requirements verified:** DSP-01, PERF-01, PERF-02, QUAL-01, QUAL-02 (+ FUNC-06 timing precondition)
**Commit:** `test(O-Strata): Phase 2.5 — bake + render harness gates (QUAL-01/02, PERF-01/02)`

---

## Stage 3: GUI Phases

Prerequisite: the finalized mockup (`mockups/v*-ui.yaml` → `index.html`) from the `/start O-Strata` mockup step, Ouaricon Naturalist brand as O-Prism, 1200 × 800.

### Phase 3.1: Oscillator panel Geometry controls, stacked-frame view, bake progress

**Goal:** Each oscillator panel shows the Geometry block, one family at a time, bound through the generated relays; the Serum-style stacked-frame view keeps working on the published table; a bake progress indicator per oscillator.

**Tasks:**
- Copy mockup HTML; per-family control groups shown/hidden by `osc?GeoSource` (all 46 relays exist; hidden ones stay bound). Blur/Edge greyed when Terrain ≠ Imported; φ greyed in Centroid Distance mode.
- Choice dropdowns for the 10 choice params per osc via the existing choice-relay pattern; library names and family names as `data-i18n` labels.
- `bakeProgress` event (10 Hz while baking) → progress ring/bar (O-TextureForge precedent); `bakeGeneration` change → stacked-frame view refetch via `getActiveOscFrame`.
- Convert O-Prism's held-notes `evaluateJavascript` push to an `emitEventIfBrowserIsVisible` event (Decision 7).
- i18n: new en strings; **fr** through `scripts/i18n-fr` glossary + lint gate (exit 2 on violation, `reviewed:true` only after Taylor reads them); **zh-Hans** through the rollout rules (`scripts/i18n-zh-glossary.js`, lint Z5, `reviewed:'bt'`).

**Test Criteria:**
- [ ] Every geometry control moves its parameter and follows host automation (two-way, relay round-trip)
- [ ] Switching Source Type swaps the visible family; hidden controls keep their values; a knob in a hidden family never triggers a bake
- [ ] Stacked-frame view shows the new table within one push after a bake (UI-04)
- [ ] Bake progress visible for a 1 M-triangle bake; hidden when idle (UI-04)
- [ ] `check-ui-labels` / i18n lints pass for en/fr/zh-Hans; French glossary lint exit 0; no prose in JS
- [ ] boot-all-uis reports no late/dead tip bindings

**Requirements verified:** UI-04
**Commit:** `feat(O-Strata): Phase 3.1 — geometry panel, stacked-frame view, bake progress, i18n`

---

### Phase 3.2: 3D geometry view (WebGL2 + Canvas 2D), playhead, preset re-push

**Goal:** `geometry-view.js` per ARCHITECTURE "3D Geometry View": mesh + plane + contour, volume point cloud + orbit + scan point, terrain wireframe + orbit + scan point; 30 Hz playhead; re-push after preset apply.

**Tasks:**
- Port `webgl-3d/terrain-proto.html` into `ui/public/js/geometry-view.js` (WebGL2 + Canvas 2D in one file; `webglcontextlost`/`restored`; no `flat`; retina backing).
- Processor/editor: view caches built in the bake job (64×64 heightmap, decimated ≤ 8 k-tri mesh, ≤ 4096-point cloud, contour + orbit polylines); `geoView` base64 Float32 event on `bakeGeneration` change; `geo/mesh-<osc>-<gen>.bin` resource route; `geoPlayhead` change-gated 30 Hz event from the editor timer.
- Re-push on `importRevision`/`bakeGeneration` change (covers preset apply and session restore with the editor open).
- Placeholder `placeholder.webglUnavailable` as a `data-i18n` node in en/fr/zh-Hans.
- PERF-03 instrumentation: debug native fn returning mean/max draw time over 300 frames.

**Test Criteria:**
- [ ] WebGL2 view renders in Standalone (WKWebView), in Logic (AU, WKWebView) and in a Windows host (WebView2) (UI-01)
- [ ] Forcing `getContext('webgl2')` to null → Canvas 2D fallback + localised placeholder in all three languages (UI-02)
- [ ] `WEBGL_lose_context` simulated: view recovers after `webglcontextrestored`, no stuck frame (UI-02)
- [ ] Playhead advances with Position/phase at 30 Hz; unchanged state sends no event (verified by event counter) (UI-01)
- [ ] After a preset apply the view shows the new geometry within one push interval (UI-05)
- [ ] Draw time ≤ 2 ms mean at DPR 2 in WKWebView and WebView2; if exceeded, Canvas 2D becomes the default (PERF-03)
- [ ] Full table never crosses as text (bridge payload audit: largest event ≤ 32 KB; mesh via `.bin`)

**Requirements verified:** UI-01, UI-02, UI-05, PERF-03
**Commit:** `feat(O-Strata): Phase 3.2 — 3D geometry view (WebGL2 + Canvas 2D), playhead, preset re-push`

---

### Phase 3.3: View interaction, OBJ/STL/PNG import (drag-and-drop + chooser)

**Goal:** Drag/wheel on the view edits parameters through the APVTS; users bring their own geometry.

**Tasks:**
- Drag → `Juce.getSliderState(id)` with `sliderDragStarted` / `setNormalisedValue` / `sliderDragEnded` (terrain: `osc?TerCX/CY`; mesh: `osc?MeshTiltX/Y`); wheel `{passive:false}` → `osc?TerSweepRange` / `osc?VolSweepRange`; document-level move/up.
- Drag-and-drop onto the oscillator panel: macOS `webkitGetAsEntry` content streaming, base64 chunks (≤ 4 MB per call) → `importGeometryChunk` / `importGeometryCommit(osc, name)`; same JS path on WebView2.
- File chooser button: `FileChooser::launchAsync` with `SafePointer`, no `complete()` on the null path; reads bytes on the message thread; parse in the bake job (Decision 6, thread assert).
- Import notices (parse error, open-mesh warning, degenerate) as localised events; `Imported` entry auto-selected on success.
- SHA-256, name, size recorded in `importState` (persistence branches finished in 4.1).

**Test Criteria:**
- [ ] Dragging the terrain view moves Orbit Centre X/Y through the relay with drag-start/drag-end notifications (host shows a single undo step) (UI-03)
- [ ] Dragging the mesh view moves Slice Tilt X/Y; wheel edits Sweep Range; no page scroll (UI-03)
- [ ] OBJ, STL ASCII, STL binary and PNG load via chooser and via drag-and-drop on macOS and Windows (FUNC-06, FUNC-07)
- [ ] Non-manifold / open mesh imports bake with a visible warning, no crash; 1 M-triangle OBJ imports and bakes ≤ 2 s with the UI responsive (FUNC-06)
- [ ] Parser thread assert never fires; `processBlock` never touches import code (FUNC-06)
- [ ] PNG import honours Image Blur and Edge Mode (visible in the 3D view and the table) (FUNC-07)

**Requirements verified:** FUNC-06, FUNC-07, UI-03
**Commit:** `feat(O-Strata): Phase 3.3 — view interaction, OBJ/STL/PNG import`

---

## Stage 4: Polish / Validation

### Phase 4.1: Persistence, fallback notice, factory presets

**Goal:** FUNC-08 two-branch persistence with the 2 MB cap (Decision 1), QUAL-03 fallback, ≥ 5 factory presets per family.

**Tasks:**
- `geometryImports` save/restore: gzip + base64 ≤ 2 MB per source (PNG stored raw), else path + SHA-256 + name; string-typed property reads (`isVoid`/`toString`); preset-manager `customSave`/`customLoad` carry the same tree; `importRevision++` on load.
- Missing/changed source → library default of the family + localised notice with "Locate…" (chooser).
- Factory presets: ≥ 5 Mesh, ≥ 5 Volume, ≥ 5 Terrain (library sources only, 0 bytes of imports), categories aligned with O-Prism's preset-manager layout; `FactoryPresets.cpp` rewritten; factory version bump.

**Test Criteria:**
- [ ] Save/reload a preset with an imported OBJ under the cap: SHA-256 of the baked level-0 frames identical (FUNC-08)
- [ ] Above the cap: state holds path + SHA; reload with file present regenerates; file missing → library fallback + visible notice, never silence or crash (FUNC-08, QUAL-03)
- [ ] Session restore with the editor open re-pushes the view (UI-05 regression)
- [ ] All 15+ factory presets load, bake without warnings, and each family is represented ≥ 5× (FUNC-11)
- [ ] Preset file with two embedded 2 MB sources loads in < 1 s on an SSD

**Requirements verified:** FUNC-08, FUNC-11, QUAL-03
**Commit:** `feat(O-Strata): Phase 4.1 — import persistence, fallback notice, factory presets`

---

### Phase 4.2: CI Windows build, listening pass, changelog, install

**Goal:** Release-ready v1.0.0.

**Tasks:**
- CI Windows VST3 build (WebView2 static linking flag, `withUserDataFolder()` inherited from O-Prism); pluginval strictness 10 on Windows in CI (catches NaN auval misses, memory: `pattern_ci_pluginval10_catches_latent_nan`).
- Listening pass on `tests/exports/*.wav` from Phase 2.5 and in-DAW on the factory presets; tune library defaults (e.g. torus knot tube radius, fBm blob amplitude) if anything is dull or harsh — defaults only, no algorithm changes without a harness re-run.
- Final pluginval + auval on macOS; cross-DAW smoke (Logic AU, Live VST3, Standalone).
- CHANGELOG.md v1.0.0; NOTES.md; PLUGINS.md → ✅ Working / 📦 Installed after `build-and-install.sh`.

**Test Criteria:**
- [ ] CI Windows build green; VST3 loads in a Windows host with the 3D view (COMPAT-02)
- [ ] pluginval strictness 10 VST3 + AU pass; auval pass (COMPAT-01 regression)
- [ ] Listening pass signed off on every library entry (QUAL-04)
- [ ] `auval -a | grep -i strata` after a fresh install; no stale `-dev`/release variant shadowing
- [ ] CHANGELOG lists the two Stage 0 decisions (cap, APVTS bake params) as user-visible behaviour

**Requirements verified:** COMPAT-02, QUAL-04, COMPAT-01 (regression), all remaining
**Commit:** `release(O-Strata): v1.0.0 — CI Windows, listening pass, changelog`

---

## Implementation Flow Summary

```
Stage 1: Foundation
  |-- Fork O-Prism v1.24.0, PLUGIN_CODE OuSt, remove wavetable library, +46 geometry params
  |-- juce_cryptography, JUCE_WEB_BROWSER-guarded createEditor, pluginval/auval (COMPAT-01)
  |
Stage 2: DSP (5 phases)
  |-- 2.1: FrameSet + alignment + normalisation + mipmaps + scheduler, trivial sine terrain (FUNC-01/09/10/12, DSP-04/05/07, PERF-01/02)
  |-- 2.2: Mesh slicer + built-in meshes + Python golden + ugly-mesh corpus (FUNC-02/05, DSP-02/03/04)
  |-- 2.3: Volume generator (FUNC-03)
  |-- 2.4: Terrain generator, analytic + PNG sampling (FUNC-04/05, DSP-06)
  |-- 2.5: Harness gates QUAL-01/02, PERF-01/02, DSP-01, 1 M-tri timing
  |
Stage 3: GUI (3 phases)
  |-- 3.1: Geometry panel per family, stacked-frame view, bake progress, i18n en/fr/zh-Hans (UI-04)
  |-- 3.2: 3D view WebGL2 + Canvas 2D, playhead, preset re-push, PERF-03 (UI-01/02/05, PERF-03)
  |-- 3.3: View drag/wheel → APVTS, OBJ/STL/PNG import (FUNC-06/07, UI-03)
  |
Stage 4: Polish / Validation (2 phases)
  |-- 4.1: Persistence cap + fallback notice, factory presets (FUNC-08/11, QUAL-03)
  |-- 4.2: CI Windows, listening pass, changelog, install (COMPAT-02, QUAL-04)
```

**Each phase gets a path-scoped git commit** (`git commit -- plugins/O-Strata PLUGINS.md`), after re-checking `git branch --show-current` and `git status --short` immediately before committing.

---

## Implementation Notes

### Thread Safety
- No new audio-thread code. Publish on the message thread only; `prepareToPlay` publishes nothing; `importLock` never on the audio thread; retired tables freed by the existing two-generation reaper. See ARCHITECTURE "Thread Boundaries".

### Performance
- Bake budget is background-only; the audio budget is O-Prism's. The mipmap stage (~90 ms at 256 frames) is the largest fixed bake cost and is inherited code; Frame Count 64/128 exist for fast iteration.

### Latency
- Zero added; distortion oversampling latency inherited and reported as before.

### Denormal Protection
- `ScopedNoDenormals` inherited; bake output `isfinite`-guarded.

### Known Challenges
- Imported geometry robustness (open/non-manifold meshes) — mitigated by welding, open-chain closure, warnings and the ugly-mesh corpus (2.2).
- 1 M-triangle budget — slab index + background parse; decimate-on-import fallback documented in ARCHITECTURE risks.
- WebGL2 in WKWebView/WebView2 — Canvas 2D always available; PERF-03 measured in both real hosts (3.2).
- Three-language UI strings for ~60 new labels/tips — fr and zh-Hans go through their lint gates; do not hand-translate outside the glossary.
- Harness fragility — the render harness must keep building after Stage 3 (`JUCE_WEB_BROWSER` guard in Stage 1) and must be re-run in 4.2.

---

## Dependencies

### Existing Code to Reuse
- `plugins/O-Prism/` v1.24.0 (entire plugin as the fork base)
- `modules/tuning/scala-tuning-engine` v3.0.1, `modules/persistence/preset-manager` v1.0.6, vendored note-expression module
- O-TextureForge: `emitEventIfBrowserIsVisible` event pattern, background-thread progress indicator, `placeholder.webglUnavailable` i18n row, `webkitGetAsEntry` drag-and-drop JS
- O-simpleFM / O-simpleGrain render-harness CMake pattern

### External Dependencies
- JUCE 8.0.14 (local `/Users/taylorbrook/JUCE`), modules: O-Prism's 13 + `juce_cryptography`
- Python 3 + numpy for regenerating the Stage 2.2 golden from the committed prototype script

---

## Critical Path

Stage 1 → 2.1 (pipeline) → 2.2 (slicer + golden) → 2.5 (gates) → 3.2 (view in real WebViews) → 4.1 (persistence) → 4.2. Phases 2.3, 2.4, 3.1 and 3.3 hang off that spine and can be reordered if a gate blocks.

---

## References

- `plugins/O-Strata/.planning/research/ARCHITECTURE.md` (this plan's contract)
- `plugins/O-Strata/.planning/REQUIREMENTS.md` (27 IDs, acceptance criteria)
- `research/wavetable-synthesis-3d-geometry.md` §3, §4, §5, §7.1, §7.3, §7.4
- `research/wavetable-synthesis-3d-geometry-prototypes/` (mesh-slice golden, webgl-3d view, terrain-bench)
- `plugins/O-Prism/.planning/research/ARCHITECTURE.md`, `ROADMAP.md`, `params.tsv`
- `troubleshooting/patterns/juce8-critical-patterns.md`
