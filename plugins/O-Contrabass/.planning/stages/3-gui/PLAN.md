# Stage 3: GUI — Execution Plan

**Date:** 2026-07-10
**Revision:** rev-1
**Inputs:** CONTEXT.md rev-1, RESEARCH.md (DEEP, 2026-07-10), mockup v1 (finalized, commit 19f51d9), `v1-integration-checklist.md` (execute playbook)
**Window:** 1000×650 fixed, non-resizable · **Params:** 31 (parameter-spec.md authoritative)

---

## Goal

Integrate the finalized mockup v1 into O-Contrabass as a production WebView GUI: all 31 parameter bindings live, three real-data visualizations (Schelleng wedge with DSP-true operating point, DSP-true body spectrum, post-limiter VU meter), preset-manager v1.0.4 preset bar, and a **full Tuning tab** (shared tuning-panel.js, O-MicrotonalSampler-style) — with the 19-entry byte-identical goldens bar preserved throughout (GUI must not touch DSP).

## Plan-Phase Decisions (locked)

| # | Decision | Resolution |
|---|----------|------------|
| D1 | `.tun` route (R2) | **Restrict picker filter + labels to `.scl` for v1.0.** Verified: TuningEngine 2.1.0 has NO TUN parser (`TuningEngine.h` — only `loadScalaFile`); O-Bowed/O-Wind ship the same dead `*.tun` filter (`O-Bowed/PluginEditor.cpp:352`). AnaMark TUN parser → v1.1 backlog as a shared-module upgrade (benefits O-Bowed/O-Wind/O-Bassoon too). TUNING_SYSTEM choice label reviewed in Task 9 for "Scala/TUN" wording. |
| D2 | STRING_TENSION (R4) | **Ships inert, user-confirmed.** Knob stays visible + bound (state round-trips); annotated known-inert in NOTES.md + CHANGELOG. DSP wiring → v1.1 backlog (activating it changes default timbre — default 0.5 is not a no-op; needs its own goldens re-baseline per `pattern_activating_dead_param_default_timbre`). |
| D3 | Tuning UI scope | **USER SCOPE EXPANSION (supersedes RESEARCH recommendation):** full Tuning tab in v1.0, like O-MicrotonalSampler — canonical `tuning-panel.js` (intervals table, 24+ embedded library, scale generator, visualizations, .scl/.kbm I/O, octave stretch, HTML export). Hosted via a Main/Tuning tab bar in the 42 px header (O-MicrotonalSampler `index.html:38-40` pattern); Main tab keeps the finalized single-view grid + microtonal footer strip unchanged. Controlled amendment to mockup v1 — no re-run of ui-mockup workflow (shared component with its own CSS; parchment-palette overrides only). |
| D4 | Dorico-compatible tunings | DSP side is **already wired** (Phase 2.6c: VST3 Note Expression live, D9 cache-compose, D10 gate, FUNC-05 Y/Z). UI contribution = NOTE_EXPRESSION toggle (bound in mockup) + Tuning tab driving the same TuningEngine the NE path reads. Dorico **distribution artifacts** (Playback Template / EndpointConfig / .doricolib — required, per `critical_dorico_distribution_mechanism` a bare .doricoexpmap is NOT ingested) → Stage 4 packaging task, logged in NOTES.md. |
| D5 | bowState voice selection (R3) | Per-voice relaxed atomics + processor publishes the **most-recently-started active voice** (fixes `getActiveVoice()` voice-0 hardcode at `PluginProcessor.cpp:339-344`; kNumVoices=4). Same single-writer pattern as `lastSafeDepth` (`BowedContrabassVoice.h:193`). |
| D6 | preset-manager | v1.0.4 (module.yaml authoritative), **CMake include** of `modules/persistence/preset-manager/cpp` (O-Wind pattern, only style that stays current) + canonical `js/preset-manager.js` (10 native fns; constructor gets the `Juce.getNativeFunction` factory + `onConfirmDelete` hook — `window.confirm` is dead in WKWebView). |
| D7 | Body spectrum | Pure JS recompute (Q3) — no data feed. Mockup mode table replaced with `BodyResonator.h:65-70` truth: freqs {60,98,115,175,235,340,700,1200}, Q {14,11,9,8,7,6,5,2.5}, gains {−2,0,−1,−3,−4,−5,−7,−6} dB + the three `recomputeCoefficients()` formulas. |

## Tasks

### Phase 3.1 — Harness protection, layout, bindings

1. [ ] **Render-harness protection + baseline goldens (DO FIRST — checklist §0)**
   - Guard `createEditor()` with `#if JUCE_WEB_BROWSER` (`Source/PluginProcessor.cpp:332-335`, currently unguarded); remove `Source/PluginEditor.cpp` from `tests/render-harness/CMakeLists.txt` sources; rebuild harness (`-DOUARICON_BUILD_TESTS=ON`); run 19-entry `reproduce-goldens.sh` — **all byte-identical** before any UI work.
   - Files: `Source/PluginProcessor.cpp`, `tests/render-harness/CMakeLists.txt`
   - Depends on: none

2. [ ] **Copy UI files (checklist §1)**
   - `Source/ui/public/`: mockup `v1-ui.html` → `index.html`; JUCE 8.0.9 frontend `index.js` + `check_native_interop.js` → `js/juce/`; verify `import * as Juce from "./js/juce/index.js"` path.
   - Files: `Source/ui/public/index.html`, `Source/ui/public/js/juce/*`
   - Depends on: Task 1

3. [ ] **Replace PluginEditor with scaffolding (checklist §2)**
   - `v1-PluginEditor.{h,cpp}` → `Source/PluginEditor.{h,cpp}`. Verify member order **relays (31) → webView → attachments (31)** (attachments call `evaluateJavascript()` in dtors — must die before webView); init order matches declaration order; grep-diff all 31 relay IDs vs `createParameterLayout()` (case-sensitive).
   - Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
   - Depends on: Task 2

4. [ ] **CMake merge (checklist §3)**
   - Add `juce_add_binary_data(OContrabass_UIResources ...)` + link. Do NOT duplicate WebView flags (already present since Stage 1 at `CMakeLists.txt:19-20,81-82`). Single default-`BinaryData`-namespace target only.
   - Files: `CMakeLists.txt`
   - Depends on: Task 2

5. [ ] **Resource provider audit + Debug build/install/smoke (checklist §5–§6)**
   - Bare-path equality + MIME audit (`/`, `/index.html`, `/js/juce/*.js` → `application/javascript`; every later file gets provider entry + binary-data source). Verify `withUserDataFolder()` present in options chain (R6; O-Bowed ref `PluginEditor.cpp:77-83`). `ninja O-Contrabass_VST3 O-Contrabass_AU` → `./scripts/build-and-install.sh O-Contrabass` → Standalone: 7 sections at 1000×650, console clean, no `vh`/`vw`, no scrollbars.
   - Files: `Source/PluginEditor.cpp`
   - Depends on: Tasks 3, 4

### Phase 3.2 — Interaction, presets, tuning tab

6. [ ] **Release destruction-order gate (checklist §7)**
   - Release build; open/close editor ×10 in Logic — no crash/freeze (Debug hides the relays→webView→attachments UAF).
   - Depends on: Task 5

7. [ ] **31-param binding validation (checklist §8)**
   - All 24 knobs + 4 detune fine-tuners (±25¢ detent, wheel 10¢/shift 1¢) + ACTIVE_STRINGS stepper + TUNING_SYSTEM dropdown + NOTE_EXPRESSION toggle. **`getScaledValue()` spot-check on the 6 skewed params** (BOW_SPEED, BOW_PRESSURE, BRIGHTNESS, VIBRATO_RATE, SLOW_LFO_RATE, REFERENCE_PITCH) vs DAW generic view. Dblclick-reset via `getParameterDefaults` matches parameter-spec defaults. Host automation + state recall update the UI. Drone "awake" glow at INFINITE_SUSTAIN/SUB_HARMONICS > 0.
   - Depends on: Task 5

8. [ ] **Preset-manager v1.0.4 adoption (D6)**
   - CMake: include `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp` (`O-Wind/CMakeLists.txt:57` pattern). C++: `OuariconPresetManager presetManager { apvts, "O-Contrabass" }` + Custom Save/Load callbacks round-tripping tuning state (Scala path, custom intervals, tonic, octave stretch). Register the 10 native fns (ref `O-Wind/Source/PluginEditor.cpp:191-230`). JS: copy canonical `js/preset-manager.js`, wire the mockup preset bar (prev/next/name/Save, `v1-ui.html:1499-1505`). No "/" in preset names (filename verbatim).
   - Files: `CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `Source/ui/public/js/preset-manager.js`, `Source/ui/public/index.html`
   - Depends on: Task 5

9. [ ] **Tuning tab (D3 — user scope expansion)**
   - HTML: Main/Tuning tab bar in the 42 px header (O-MicrotonalSampler `Resources/ui/index.html:38-40,587`); `#tuning-container` tab body; copy canonical `modules/tuning/scala-tuning-engine/js/tuning-panel.js` + `snippets/tuning-panel.css`; `const { TuningPanel } = await import(...)`; **`new TuningPanel(container, Juce)` — the `Juce` ES-module namespace, never `window.__JUCE__`** (O-Wind `index.html:1594-1599` is the corrected reference). Parchment-palette CSS overrides to stay in-family.
   - C++: register the ~20 tuning native fns (getTuningIntervals/setTuningIntervals/setSingleInterval/setTonicNote/getTonicNote/loadScalaFile/saveScalaFile/loadKBMFile/saveKBMFile/get+setMasterTune/get+setOctaveStretch/generateEDO/generateHarmonicSeries/generateRank2/applyGeneratedScale/getEmbeddedTuningList/loadEmbeddedTuning/exportTuningHTML/getTuningName) — ref `O-Wind/Source/PluginEditor.cpp:191-510`. All FileChooser completions: SafePointer + **bare `return` on null** (no `complete()` — UAF).
   - Coherence: REFERENCE_PITCH param ↔ panel masterTune, TUNING_SYSTEM param ↔ panel mode, both directions (O-Wind reference). Embedded-tuning loads must include the period (`pattern_embedded_tuning_period_dropped` — verify module copy is post-fix).
   - Review TUNING_SYSTEM choice label wording re "Scala/TUN" (D1).
   - Files: `Source/ui/public/index.html`, `Source/ui/public/js/tuning-panel.js`, `Source/ui/public/css/tuning-panel.css`, `Source/PluginEditor.cpp`
   - Depends on: Task 5

10. [ ] **Scala picker + .scl-only restriction (checklist §9, D1)**
    - Footer-strip `Load` button: filter `*.scl` only, label ".scl"; routes through `TuningEngine::loadScalaFile`. Cancel path completes `false` (editor alive). **UAF test:** open picker → close plugin window → choose/cancel — no crash.
    - Files: `Source/PluginEditor.cpp`, `Source/ui/public/index.html`
    - Depends on: Task 9

11. [ ] **`ui_frontend_check.js` port + bridge gate (checklist §4)**
    - Port from `backups/O-MicrotonalSampler/v1.23.7/Source/tests/ui_frontend_check.js` (checks 1/2/6/8 generic; adjust paths/regexes to O-Contrabass registration style). Run the `getNativeFunction` vs `withNativeFunction` grep-diff — surface now ≈ 32 fns (2 mockup + 10 preset + ~20 tuning); **any asymmetry = dead control**.
    - Files: `tests/ui_frontend_check.js`
    - Depends on: Tasks 8, 9, 10

### Phase 3.3 — Visualizations (real data feeds)

12. [ ] **vuLevel real feed**
    - `std::atomic<float>` RMS dB on the processor, stored at end of processBlock **after the output-gain loop** (`PluginProcessor.cpp:315` — true post-saturator/width/limiter/gain signal). Replace the −20 dB placeholder emit; keep JSON `{"db":N}` contract (JS expects JSON, not O-AnalogEQ's bare float).
    - Files: `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`
    - Depends on: Task 5

13. [ ] **bowState feed (D5)**
    - Per-voice relaxed atomics: `effectiveBowSpeed`/`effectiveBowPressure` (post-LFO/macro/MPE, `BowedContrabassVoice.cpp:519-556`) + β (`:878`) + active flag + start-ordinal. Processor picks most-recently-started active voice; 30 Hz timer emits `bowState` `{"v":,"p":,"b":,"active":}` (event name reserved at `v1-PluginEditor.cpp:272-273`). JS: dot eases to feed when active, falls back to SliderState values at silence. Read-only taps — goldens-safe; never insert/reorder signal-path arithmetic.
    - Files: `Source/BowedContrabassVoice.{h,cpp}`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `Source/ui/public/index.html`
    - Depends on: Task 12

14. [ ] **Body-spectrum table fix (R1/D7)**
    - Replace mockup JS mode table `[58,74,...]` with BodyResonator truth tables + exact formulas (`sizeScalar = 0.85+0.30·size`, `fc = f₀/sizeScalar` clamp 20–0.45·sr, `qScalar = max(0.15, 1−0.85·damping)`, `qEff = max(0.10, q₀·qScalar)`, `gDb = g₀+1.5·(size−0.75)`). Keep dirty-flag redraw. Delete or comment the reserved `bodyModes` event.
    - Files: `Source/ui/public/index.html`
    - Depends on: Task 5

15. [ ] **Timer hygiene + CPU check (checklist §10)**
    - `!webView->isShowing()` early-out on top of `emitEventIfBrowserIsVisible` (O-GrainScatter IN-15). Wedge rAF-driven, spectrum dirty-flag, meter eased. Test criterion: no CPU spikes, 60 fps max, editor hidden ⇒ no emit churn.
    - Files: `Source/PluginEditor.cpp`
    - Depends on: Tasks 12, 13, 14

### Stage exit

16. [ ] **Full regression bar (checklist §11) + annotations**
    - 19-entry `reproduce-goldens.sh` byte-identical (re-run — Tasks 12/13 added processor/voice code) · `auval -a | grep -i contrabass` SUCCEEDED (aumu OCbs OuDv) · pluginval strictness 10 SUCCESS · Logic smoke: instantiate, E1 drone, automate BOW_SPEED, reload project, editor open/close ×10 Release.
    - NOTES.md/CHANGELOG: STRING_TENSION known-inert (D2), .tun deferred + TUN-parser v1.1 backlog (D1), Dorico distribution artifacts → Stage 4 (D4), registry.yaml staleness note (R5).
    - Depends on: all

## Success Criteria

- [ ] Standalone + Logic AU render the full UI at 1000×650: 7 sections, tab bar, no blank WebView, console clean
- [ ] All 31 params bidirectional (UI ↔ APVTS ↔ host automation ↔ state recall); 6 skewed params read identical in UI and DAW generic view; dblclick-reset = spec defaults
- [ ] Tuning tab fully functional: intervals table live, embedded library loads (period intact — audible retune), scale generator applies, .scl/.kbm round-trip, octave stretch, HTML export; NE + tuning drive Dorico microtonal playback unchanged (Phase 2.6c goldens still green)
- [ ] Preset bar: save/load/prev/next against `~/Library/O-Contrabass/Presets/`; tuning state survives preset round-trip
- [ ] Schelleng dot tracks DSP-true effective bow state when a voice is active (moves with EXPRESSION_MACRO/LFO while knobs are still); body spectrum peaks match BodyResonator (60 Hz mode 1, moves with Size/Damping); VU tracks real post-limiter level
- [ ] Bridge gate clean: every JS `getNativeFunction` name has a C++ registration (≈32 fns), `ui_frontend_check.js` passes
- [ ] **19-entry goldens byte-identical at stage exit** + auval SUCCEEDED + pluginval 10 SUCCESS + Logic editor ×10 no crash
- [ ] No CPU spikes from visualizations; hidden editor emits nothing

## Risks Carried Into Execute

| Risk | Mitigation in plan |
|------|--------------------|
| Tab-bar amendment destabilizes finalized mockup layout | Main tab DOM untouched; tab bar confined to existing 42 px header; Tuning tab is a sibling section (O-MicrotonalSampler pattern) |
| 20 new tuning fns → bridge-gap surface triples | Task 11 gate is mandatory after Tasks 8–10; ui_frontend_check.js in `tests/` for re-runs |
| REFERENCE_PITCH param vs panel masterTune dual-source fight | Explicit coherence sub-task in Task 9 with O-Wind as the working reference |
| Voice atomics touch DSP files | Read-only relaxed stores only; goldens re-run in Task 16 (and immediately after Task 13 if paranoid — cheap) |
| MTS-ESP is a Stage-2 stub (FUNC-07 v1.1-logged) | TUNING_SYSTEM dropdown ships the choice; no new UI promises — panel mode coherence treats MTS-ESP as present-but-stub |
