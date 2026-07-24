# Stage 3: GUI - Verification

## Verification Date

2026-07-24

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

1. **UI-01** — two-way binding for all 10 parameters (controls drive DSP; host automation / preset changes update the UI)
2. **UI-02** — Sync/Free conditional time control: `syncMode` swaps the visible control between `noteDivision` and `delayTime`, with no dead controls in either mode
3. Knob readouts computed from `SliderState.getScaledValue()` — never a JS min/max map (4 skewed params)
4. **D8/D9/D10** — Ouaricon Naturalist aesthetic, grouped signal-flow layout (TIME | GRAIN | FEEDBACK | OUTPUT), no visualization
5. Survive the WebView pitfall set: TDZ discipline, HTML-authored label preservation, bare-path resource provider, `Juce` ES-module namespace, dual-BinaryData namespace, WebView2 static linking + user-data folder, relay→WebView→attachment member order
6. Render harness must survive the WebView editor landing (no Stage-2 DSP regression)

### Deliverables (from SUMMARY.md, confirmed by inspection and re-execution)

1. `Source/PluginEditor.h/.cpp` — 8 `WebSliderRelay` + 2 `WebComboBoxRelay`, one native fn (`getParameterDefaults`), bare-path resource provider, member order relays → webView → attachments
2. `Source/ui/public/` — `index.html`, `css/styles.css`, `js/app.js`, byte-identical `js/juce/` bridge pair, `img/birds.png`
3. `CMakeLists.txt` — `NEEDS_WEB_BROWSER`/`NEEDS_WEBVIEW2`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `juce_add_binary_data` with `NAMESPACE UIBinaryData`
4. `Source/PluginProcessor.cpp` — `createEditor()` and the `PluginEditor.h` include both inside `#if JUCE_WEB_BROWSER`
5. `tests/ui-stub/` (bridge stub + server) and `tests/ui_frontend_check.js` (45 static assertions)
6. Commit `48432ee`

### Goal Achievement

| Goal | Status | Evidence (re-run at verify, not quoted from SUMMARY) |
|------|--------|------|
| UI-01 two-way binding | ✅ Achieved | All 8 readouts equal `createParameterLayout()` defaults on open (500 ms / 200 ms / 60 % / 40 % / 100 Hz / 8.0 kHz / 60 % / 35 %); `syncMode`=Sync, `noteDivision`=1/4 (index 6). Real Standalone restored a **non-default** saved state (FREE, 317 ms) and the UI rendered it — C++→JS direction in the real bridge. pluginval **Editor Automation** passed at strictness 10 on both formats (host→UI direction). |
| UI-02 Sync/Free swap | ✅ Achieved | `.time-slot` box **identical** in both modes: `x:117 y:198 w:86 h:100` → zero layout shift. Sync shows `SELECT.division-select`; Free shows the `delayTime` knob. Both relay-bound at all times — no dead control. |
| getScaledValue() readouts | ✅ Achieved | At normalised 0.5 every skewed param lands **exactly** on its `setSkewForCentre` value: delayTime 316 ms, grainSize 158 ms, lowCut 200 Hz, highCut 3162 Hz (3.2 kHz); linear mix/density 50 %. A JS range map would have read 1025 ms for delayTime. |
| Naturalist aesthetic / layout (D8–D10) | ✅ Achieved | Rendered at 940×440: aged-paper ground, seed cross-section knobs, four framed panels TIME \| GRAIN \| FEEDBACK \| OUTPUT in signal-flow order, bird overlay behind OUTPUT. No visualization, no Timer, no polling bridge. |
| WebView pitfall set | ✅ Achieved | 45/45 `ui_frontend_check.js` assertions pass, covering every pattern listed in goal 5. Real WKWebView render confirms the resource provider and ES-module bridge work end to end. |
| No DSP regression | ✅ Achieved | Render harness rebuilt and re-run at verify: **33/33 probes PASS**, `ALL PROBES PASSED (0 failures)`. The harness compiles `PluginProcessor.cpp` with `JUCE_WEB_BROWSER=0` and links — proving the `createEditor` guard holds. |

## Requirements Verification

**Stage:** 3-gui
**Requirements for this stage:** 2 total (0 must, 1 should, 1 nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: All 10 parameters controllable from the UI with readouts from `SliderState.getScaledValue()` | should | ✅ Complete | 8 knobs + 2 combos bound via relay/attachment pairs (code inspection); defaults match APVTS exactly on open; skewed midpoints exact; dblclick-reset returns all 8 knobs to engineering defaults from off-default positions; pluginval Editor Automation green on VST3 + AU |
| UI-02: Sync/Free mode switch shows the relevant time control | nice | ✅ Complete | Slot bounding box byte-identical across modes; correct control visible in each; FREE/SYNC labels survive binding with `aria-pressed` state; first open = Sync (C++ default index 1) |

**Requirements Summary:**
- ✅ Complete: 2
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 0
- ❌ Failed: 0

All 14 project requirements are now complete across stages 1–3, except those Stage 4 re-confirms (COMPAT-*).

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + Standalone) | ✅ Pass | `ninja` reports up to date against committed sources; all three bundles present |
| Render harness (`O-ReverseDelay-render-test`) | ✅ Pass | Re-run at verify: 33/33 probes, `ALL PROBES PASSED (0 failures)` |
| `node tests/ui_frontend_check.js` | ✅ Pass | 45/45 assertions, exit 0 |
| Native-fn grep-diff | ✅ Pass | JS `app.js:266` ≡ C++ `PluginEditor.cpp:114` — exactly 1 ≡ 1 (`getParameterDefaults`) |
| Stub fixture ↔ C++ range parity | ✅ Pass | `juce-stub.js` RANGES compared line-by-line against `createParameterLayout()`: all 8 ranges, skew centres, and defaults match; combo defaults 1 / 6 match |
| Browser-stub render | ✅ Pass | Exactly 940×440, zero page overflow, **zero JS console errors** (only a stub-server `favicon.ico` 404 — HTTP-only, absent in the WebView path) |
| Real WKWebView render (Standalone) | ✅ Pass | Window 942×498 (940×440 editor + standalone bar); page renders identically to the stub; knob angle matches the real normalised value |
| `auval -v aufx ORvD OuDv` | ✅ Pass | **AU VALIDATION SUCCEEDED** |
| pluginval strictness 10 — VST3 | ✅ Pass | **SUCCESS**; Editor, Automation, Editor Automation, Parameters, Parameter thread safety, Fuzz parameters all ran |
| pluginval strictness 10 — AU | ✅ Pass | **SUCCESS**; same editor test coverage. `Current program is -1` warning is the standard no-programs AU notice, benign |
| Install state | ✅ Pass | `O-ReverseDelay-dev.vst3` + `.component` present in system folders; no alternate-variant orphans |

## Human Verification

- [ ] **D7/D6 Standalone audition — Stage 4 entry gate** (deferred by decision D7): smear / wash / width by ear, including the ~−7.3 dB/generation wash-decay finding and whether a feedback-tap makeup constant is wanted. DSP-only; does not affect Stage 3.
- [ ] Load in a real DAW (Logic / Ableton) and confirm the WebView renders and automation round-trips in-host.
- [ ] Optional Stage-1 carryovers: DAW mono→stereo listen, session save/reload round-trip.

## Issues Found

1. **Synthetic click on the live Standalone could not be delivered** — macOS returned accessibility error `-25208`, so the interactive JS→C++ direction was not re-driven in the real WebView at verify. Not a defect and not blocking: that direction was exercised at execute, and the evidence survives physically — the Standalone reopened at **FREE / 317 ms**, a non-default state that could only have been written through the UI→parameter path and read back through the parameter→UI path. pluginval's Editor Automation test covers the host→UI direction programmatically on both formats.

2. **`favicon.ico` 404 in the browser stub** — an artifact of serving over plain HTTP; the WebView resource provider returns `std::nullopt` for unmatched paths, which is correct. No action.

3. **Standalone settings retain the execute-session state** (`~/Library/Application Support/O-ReverseDelay-dev.settings` → FREE / 317 ms). Standalone-only; does not affect VST3/AU instances, which open at APVTS defaults. Left in place as verification evidence.

No defects were found in the shipped code.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Blockers:** None

**Carried into Stage 4:**
- D7 audition is the **required entry gate** before any Stage-4 DSP edit
- Re-run the render harness at Stage-4 entry (`pattern_render_harness_breaks_on_webview_editor`)
- Factory presets must be authored in engineering units + `convertTo0to1` — 4 skewed params (`pattern_factory_preset_normalized_ignores_skew`)
- Any new `juce_add_binary_data` target must not claim `NAMESPACE BinaryData` (`UIBinaryData` is taken)
- pluginval ×3 repeat gate remains Stage 4's
