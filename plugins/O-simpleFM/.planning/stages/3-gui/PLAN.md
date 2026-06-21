# Stage 3 (GUI) — PLAN

**Goal:** Replace the generic editor with a single-page Ouaricon-Naturalist WebView UI that binds
all 17 params two-way, renders the live spectrum + oscilloscope, and adds the pedagogical layer
(routing diagram, tooltips, preset tour) — cross-platform, no blank UI.

**Executor:** `gui-agent`. **Mode:** express (direct integration — build UI from scratch using the
botanical theme; no pre-finalized mockup).

**Inputs:** CONTEXT.md, RESEARCH.md (this dir); `Source/PluginProcessor.h` (`OSimpleFM::ParamIDs`);
`FmVizAnalyzer` (`getSpectrum()`/`getScope()`); aesthetic `.claude/aesthetics/ouaricon-naturalist-001/`.
**Reference code:** O-Prism (theme + relay/attachment/resource-provider + member order),
O-Marimba (30 Hz emit + canvas).

---

## Tasks (map to ROADMAP 3.1 → 3.2 → 3.3)

### Task 1 — Phase 3.1: Scaffold + layout + control binding + cross-platform wiring
1. Create `Source/ui/public/{index.html, css/styles.css, js/app.js, img/}` and copy
   `js/juce/index.js` + `js/juce/check_native_interop.js` from O-Prism.
2. `index.html`: single page, `type="module"` scripts, four control groups
   (Operators / Mod Env / Amp Env / Output) + viz panel + routing-diagram container +
   preset-tour bar. Botanical theme markup (overlay img, fleurons, Garamond).
3. `css/styles.css`: full ouaricon-naturalist system (paper bg, seed-knob conic-gradient,
   green toggles, spacing, botanical overlay, DPR-ready canvas sizing).
4. `js/app.js`: bind all 17 controls via `Juce.getSliderState`/`getToggleState` (relative-drag
   knobs), value readouts, tooltip map.
5. Rewrite `PluginEditor.h/.cpp`: member order relays→webView→attachments; keep
   `FmVizAnalyzer` + `Timer`; remove `genericEditor`; `getResource` (bare-path equality);
   Windows `withUserDataFolder`; `goToURL(getResourceProviderRoot())`.
6. CMake: `juce_add_binary_data(O-simpleFM_UIResources …)` + link. (WebView2 flags already set.)

**Done when:** builds VST3+AU; WebView opens (no blank); all 17 controls two-way bound
(drag→DSP, host automation→UI).

### Task 2 — Phase 3.2: Live spectrum + oscilloscope
1. `timerCallback()`: after existing `vizAnalyzer.process(...)`, emit `spectrumUpdate`
   (256 dB bins) + `scopeUpdate` (128 pts) via `emitEventIfBrowserIsVisible`.
2. `app.js`: `window.__JUCE__.backend.addEventListener` for both; DPR-aware canvases;
   spectrum bars (dB→height, log-freq already baked in by analyzer), scope polyline.
3. Make sidebands visually crisp/separated; spectrum smoothing already in analyzer.

**Done when:** raising Mod Index visibly multiplies discrete sidebands; changing Ratio snaps
spectrum harmonic↔inharmonic and morphs the scope live; Feedback smears toward saw/noise; no
audio-thread FFT/alloc; smooth at 30 Hz.

### Task 3 — Phase 3.3: Pedagogical layer
1. Live operator routing diagram (SVG/CSS): MOD→CAR + self-feedback loop; reflect
   `feedback`/`modIndex`/`ratio` state.
2. Hover tooltips on EVERY parameter (plain-language map).
3. Preset tour: E-Piano, Tubular Bell, Brass, Clarinet, Clang Bell — each sets an APVTS
   snapshot via slider/toggle states; each isolates one concept.
4. (Optional) carrier-null annotation at I ≈ 2.405 on the Mod Index control / spectrum.

**Done when:** every param has a working tooltip; routing diagram reflects state; each preset
loads and audibly/visually isolates its concept.

### Task 4 — SUMMARY
Write `stages/3-gui/SUMMARY.md`: files created/modified, control→param map, viz event contract,
theme notes, anything deferred.

---

## Files
- **Create:** `Source/ui/public/index.html`, `css/styles.css`, `js/app.js`,
  `js/juce/index.js`, `js/juce/check_native_interop.js`, `img/<botanical>.png`
- **Rewrite:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Edit:** `plugins/O-simpleFM/CMakeLists.txt` (binary data)
- **Untouched:** PluginProcessor.*, Operator.h, FMVoice.h, FmVizAnalyzer.h (DSP frozen),
  tests/render-harness (regression gate stays green)

## Success criteria (goal-backward — verify in Stage 3 verify)
- [ ] Builds VST3+AU clean; `auval` SUCCEEDED; render harness still 5/5.
- [ ] WebView renders on macOS (VST3+AU); no blank page; classroom-readable botanical UI.
- [ ] All 17 params two-way bound; relative-drag knobs.
- [ ] Spectrum: Mod Index → sidebands bloom; Ratio → harmonic↔inharmonic; Feedback → smear.
- [ ] Scope morphs live with ratio/index/feedback.
- [ ] Routing diagram + tooltips (all params) + 5-preset tour all functional.
- [ ] Cross-platform wiring correct (Windows `withUserDataFolder`; static-link WebView2 flag).

## Risks
- Large `var` array emit cost at 30 Hz × 384 floats — acceptable (O-Marimba precedent); fall back
  to native-fn poll if janky.
- Silent JS ReferenceError blanks UI — open Standalone (`/show-standalone`) to confirm before verify.
- Botanical overlay must not crowd controls/viz — keep left 60-70% clear.
