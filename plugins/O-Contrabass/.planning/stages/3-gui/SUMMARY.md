# Stage 3: GUI — Execution Summary

**Date:** 2026-07-10/11
**Plan:** PLAN.md rev-1 (16 tasks, decisions D1–D7 locked)
**Result:** All 16 tasks complete on the automatable surface; DAW-in-the-loop checks listed under "Manual verification pending".

---

## Per-task status

| # | Task | Status |
|---|------|--------|
| 1 | Render-harness protection + baseline goldens | DONE FIRST — `createEditor()` guarded `#if JUCE_WEB_BROWSER` (+ guarded `PluginEditor.h` include), `PluginEditor.cpp` removed from harness sources, harness rebuilt, **19/19 goldens byte-identical** before any UI work |
| 2 | Copy UI files | DONE — `v1-ui.html` → `Source/ui/public/index.html`; JUCE 8.0.9 `index.js` + `check_native_interop.js` → `js/juce/`; import path verified |
| 3 | Replace PluginEditor with scaffolding | DONE — `v1-PluginEditor.{h,cpp}` → `Source/`; member order relays(31)→webView→attachments(31) verified; **31/31 relay IDs grep-diff exact match** vs `createParameterLayout()` |
| 4 | CMake merge | DONE — `juce_add_binary_data(OContrabass_UIResources …)` + link; WebView flags NOT duplicated (present since Stage 1); single default-`BinaryData` target |
| 5 | Resource provider audit + build/install/smoke | DONE — bare-path equality + MIME audit (6 provider entries, `.js` → `application/javascript`); `withUserDataFolder()` present (R6); built VST3+AU+Standalone; installed via `build-and-install.sh`; Standalone ran 10 s clean (no resource-miss logs, no crash); no `vh`/`vw` |
| 6 | Release destruction-order gate (Logic ×10) | PARTIAL — Release build installed; Logic open/close ×10 is **manual** (below) |
| 7 | 31-param binding validation | PARTIAL — static checks pass (`getScaledValue()` readouts, `propertiesChanged` refresh, `paramDefaults` dblclick, detent/wheel logic in JS); DAW interaction checks **manual** |
| 8 | Preset-manager v1.0.4 adoption (D6) | DONE — canonical CMake include; `OuariconPresetManager presetManager { parameters, "O-Contrabass" }`; tuning custom save/load callbacks; session state routed through `getStateAsXml/setStateFromXml` (backward compatible); 10 native fns; canonical `js/preset-manager.js` wired to mockup preset bar |
| 9 | Tuning tab (D3) | DONE — Main/Tuning tab bar in 42 px header (O-MicrotonalSampler pattern); `#tuning-container` lazy-init; **`new TuningPanel(container, Juce)`** (ES-module namespace); 20 tuning native fns (O-Wind reference); `loadEmbeddedTuning` appends period (post-fix canonical module verified); REFERENCE_PITCH ↔ masterTune coherence via APVTS param as single source of truth; parchment CSS-variable overrides; TUNING_SYSTEM label reviewed (D1 → "Scala") |
| 10 | Scala picker `.scl`-only (D1) | DONE — footer picker + panel `loadScalaFile` filter `*.scl`; labels "Load .scl…"; SafePointer + bare-return-on-null; `fileDialogOpen` re-entry guard; UAF scenario **manual** |
| 11 | `ui_frontend_check.js` port + bridge gate | DONE — ported to `tests/ui_frontend_check.js` (6 check groups, 14 assertions) — **14/14 PASS**; bridge gate **32 JS = 32 C++** (2 mockup + 10 preset + 20 tuning), zero asymmetry |
| 12 | vuLevel real feed | DONE — `std::atomic<float> outputRmsDb` stored at END of processBlock AFTER the output-gain loop (read-only tap); editor emits JSON `{"db":N}` @30 Hz |
| 13 | bowState feed (D5) | DONE — per-voice relaxed viz atomics (speed/pressure/β published in Step 6 with the exact values pushed to the bow model; active set in `noteStarted`, cleared at BOTH `clearCurrentNote` sites; monotonic start ordinal); processor `getBowStateViz()` picks most-recently-started active voice; JS eases dot to feed when active, knob fallback at silence, axis-clamped |
| 14 | Body-spectrum table fix (R1/D7) | DONE — mockup table `[58,74,…]` replaced with BodyResonator truth (freq/Q/gain tables + `sizeScalar`/`qScalar`/`gDb` formulas, per-mode qEff); dirty-flag redraw kept; `bodyModes` event intentionally not implemented |
| 15 | Timer hygiene + CPU | DONE — `!webView->isShowing()` early-out atop `emitEventIfBrowserIsVisible`; wedge rAF, spectrum dirty-flag, VU eased ~30 fps; ADDED: rAF draws fully skipped while Tuning tab active (canvases display:none) |
| 16 | Full regression bar + annotations | DONE — goldens re-run post Tasks 12/13 DSP-file edits: **19/19 byte-identical**; auval **SUCCEEDED**; pluginval-10 **SUCCESS**; NOTES.md + CHANGELOG.md annotated (D1/D2/D4/R5); Logic smoke **manual** |

## Verification results (verbatim key lines)

```
reproduce-goldens.sh (run 1, Task 1 baseline):  OK: all 19 goldens reproduce byte-identical
reproduce-goldens.sh (run 2, Task 16 exit):     OK: all 19 goldens reproduce byte-identical
auval -a | grep -i contrabass:  aumu OCbs OuDv  -  Ouaricon Audio Development: O-Contrabass-dev
auval -v aumu OCbs OuDv:        AU VALIDATION SUCCEEDED.
pluginval --strictness-level 10 --validate O-Contrabass-dev.vst3:  SUCCESS
ui_frontend_check.js:           == ALL CHECKS PASSED ==  (14/14, bridge 32 JS = 32 C++)
```

**pluginval note:** the FIRST run timed out in "Editor Automation" (30 s) — cold-start
WKWebView process spawn immediately after the cache-clear install. Two subsequent runs
SUCCEEDED, including at the **default 30 s timeout** (Editor Automation completed in
14 s; full battery 17 s). Recorded as a cold-start flake, not a defect.

## Files created

- `Source/ui/public/index.html` (mockup v1 + Tasks 8/9/10/13/14/15 edits)
- `Source/ui/public/js/juce/index.js`, `js/juce/check_native_interop.js` (JUCE 8.0.9)
- `Source/PluginEditor.h` / `Source/PluginEditor.cpp` (scaffolding + 30 new native fns)
- `tests/ui_frontend_check.js`

## Files modified

- `Source/PluginProcessor.h` — presetManager member + accessor, `outputRmsDb`, `BowStateViz`
- `Source/PluginProcessor.cpp` — createEditor guard, tuning custom-state callbacks, session state via preset manager, RMS tap, `getBowStateViz()`, TUNING_SYSTEM label (D1)
- `Source/BowedContrabassVoice.h/.cpp` — viz atomics + accessors, Step-6/noteStarted/clearCurrentNote stores (read-only)
- `CMakeLists.txt` — preset-manager include dir; binary-data target (+ canonical module JS/CSS)
- `tests/render-harness/CMakeLists.txt` — PluginEditor.cpp removed
- `NOTES.md`, `CHANGELOG.md` — Stage 3 entry + D1/D2/D4/R5 annotations

## Decisions applied

- **D1** `.scl`-only (picker filters, labels, TUNING_SYSTEM choice label → "Scala"; TUN parser → v1.1 backlog)
- **D2** STRING_TENSION ships bound-but-inert, annotated in NOTES/CHANGELOG
- **D3** full Tuning tab via shared tuning-panel.js + header tab bar
- **D4** Dorico distribution artifacts logged → Stage 4
- **D5** per-voice viz atomics + most-recently-started active-voice selection
- **D6** preset-manager v1.0.4, canonical CMake include + canonical JS
- **D7** body spectrum pure-JS recompute from BodyResonator truth

## Deviations from plan (with justification)

1. **Canonical-path embedding instead of copying module JS/CSS.** Plan said "copy
   canonical js/preset-manager.js / tuning-panel.js / tuning-panel.css" into
   `Source/ui/public/`. Instead `juce_add_binary_data` references the canonical module
   paths directly (O-Wind pattern, `O-Wind/CMakeLists.txt` UIResources block) — served
   at the same provider paths (`/js/…`, `/css/…`). Rationale: vendored copies drift
   (7 plugins have drifted; O-Bells is 662 lines off canonical); this stays current at
   every build.
2. **`selectNext/PreviousPreset` return the neighbour NAME without loading.** O-Wind's
   registration loads AND returns; the canonical preset-manager.js then loads again
   (double load). Ours matches the JS module's documented contract.
3. **Body-spectrum display axis fMax 1200 → 1600 Hz** so DSP-true mode 8
   (1200/sizeScalar, up to ~1412 Hz at small Size) stays on-plot across the Size range.
4. **Session state now routed through OuariconPresetManager**
   (`getStateAsXml`/`setStateFromXml`) — required so tuning state survives DAW reload,
   not just presets. Backward compatible with pre-Stage-3 plain-APVTS XML (verified
   against the module's `hasTagName` path).
5. **rAF tab gating added** (Task 15 extension): all three canvases live on the Main
   tab; draws are skipped entirely while the Tuning tab is active.

## Manual verification pending (needs a human in a DAW)

- **Task 6:** Logic — open/close plugin editor ×10 in the Release build
  (destruction-order UAF gate; Debug builds hide it).
- **Task 7:** all 24 knobs drag/wheel/dblclick; 4 detune fine-tuners (±25¢ detent,
  wheel 10¢ / shift 1¢); ACTIVE_STRINGS stepper; TUNING_SYSTEM dropdown gating the
  Load button; NOTE_EXPRESSION toggle; **6 skewed params (BOW_SPEED, BOW_PRESSURE,
  BRIGHTNESS, VIBRATO_RATE, SLOW_LFO_RATE, REFERENCE_PITCH) read identical in UI vs
  DAW generic view**; dblclick-reset = spec defaults; host automation + state recall
  update the UI; drone "awake" glow at INFINITE_SUSTAIN/SUB_HARMONICS > 0.
- **Task 10 UAF test:** open picker → close plugin window → choose/cancel — no crash.
- **Task 16 Logic smoke:** instantiate, play E1 drone, automate BOW_SPEED, reload project.
- **Visual QA:** 7 sections + tab bar render at 1000×650, console clean (Inspect),
  Tuning tab layout under the parchment overrides, preset bar save/prev/next against
  `~/Library/O-Contrabass/Presets/`, tuning state survives a preset round-trip,
  Schelleng dot moves with EXPRESSION_MACRO while knobs are still, spectrum tracks
  Size/Damping, VU tracks output level, no CPU spikes.
- **Windows:** flags present since Stage 1 (`NEEDS_WEBVIEW2` + static linking +
  `withUserDataFolder`), but no Windows build was run.

## Known issues / notes

- pluginval first-run "Editor Automation" timeout = cold WKWebView spawn flake (see
  above); passes at default timeout when warm.
- STRING_TENSION inert (D2); `.tun` dead route removed from UI (D1); Dorico packaging
  → Stage 4 (D4); registry.yaml stale vs module.yaml (R5) — all annotated in NOTES.md.
- The scaffold's `.withBackend(webview2)` option is a no-op on macOS (WKWebView);
  it selects WebView2 on Windows.
- C++→JS events: `vuLevel` + `bowState` @30 Hz, both behind the
  `!webView->isShowing()` early-out + `emitEventIfBrowserIsVisible`.
