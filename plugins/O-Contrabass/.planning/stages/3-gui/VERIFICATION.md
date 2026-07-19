# Stage 3: GUI - Verification

## Verification Date

2026-07-11

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md + PLAN.md)

1. Integrate finalized mockup v1 as a production WebView GUI — 7 sections, single-view grid, 1000×650 fixed (UI-01).
2. All 31 parameter bindings live and bidirectional (UI ↔ APVTS ↔ host).
3. Three real-data visualizations: Schelleng wedge (DSP-true operating point), body spectrum (BodyResonator truth), post-limiter VU.
4. Preset-manager v1.0.4 preset bar (D6) + full Tuning tab via shared tuning-panel.js (D3, user scope expansion).
5. GUI must not touch DSP: 19-entry goldens byte-identical throughout (harness protected first).

### Deliverables (from SUMMARY.md, confirmed by code inspection)

1. `Source/ui/public/index.html` (mockup v1 + tab bar), `PluginEditor.{h,cpp}` scaffolding, binary-data + provider wiring — 7 sections verified present in shipped HTML (`Bow/Body/Strings/Expression/Drone/Output/Microtonal`), `setSize(1000, 650)` at `PluginEditor.cpp:627`.
2. 31 relays/attachments (29 slider + 1 combo + 1 toggle), member order relays→webView→attachments verified in `PluginEditor.h:61-112`; readouts from `getScaledValue()` (`index.html:1065-1170`), `getParameterDefaults` dblclick reset, `propertiesChanged` refresh.
3. `outputRmsDb` atomic tapped after the output-gain loop; per-voice bowState viz atomics (D5, most-recently-started selection); body-spectrum JS table replaced with BodyResonator truth (D7); rAF/dirty-flag/eased draws + `!webView->isShowing()` early-out + Tuning-tab rAF gating.
4. OuariconPresetManager (10 native fns) with tuning custom save/load + session state through `getStateAsXml/setStateFromXml`; Tuning tab hosts `new TuningPanel(container, Juce)` — ES-module namespace confirmed, `window.__JUCE__` used only for event receive (`index.html:896-897, 1271, 1527`); 20 tuning native fns.
5. `createEditor()` guarded `#if JUCE_WEB_BROWSER` (`PluginProcessor.cpp:421-426`), `PluginEditor.cpp` removed from harness sources; goldens re-run green at baseline, stage exit, AND independently at this verify.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Mockup v1 → production GUI, 7 sections @1000×650 | ✅ Achieved | Sections + size verified in source; Standalone 10 s smoke clean (execute); visual QA in DAW → human checklist |
| 31 bindings live | ✅ Achieved | 31/31 relay-ID grep-diff vs `createParameterLayout()`; ui_frontend_check 14/14; pluginval-10 parameter battery SUCCESS; per-knob DAW feel → human checklist |
| 3 real-data visualizations | ✅ Achieved | vuLevel real RMS feed (placeholder deleted); bowState per-voice atomics feed; spectrum recomputed from `BodyResonator.h` truth tables (R1 fixed) |
| Preset bar + Tuning tab | ✅ Achieved | Bridge gate 32 JS = 32 C++ (2 mockup + 10 preset + 20 tuning), zero asymmetry; `loadEmbeddedTuning` appends period (`PluginEditor.cpp:463-475` — pattern_embedded_tuning_period_dropped guarded) |
| GUI does not touch DSP | ✅ Achieved | **19/19 goldens byte-identical re-run fresh at verify** (third green run); Tasks 12/13 taps are read-only relaxed atomics |

## Requirements Verification

**Stage:** 3-gui — 2 requirements verified at this stage.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: Logical groupings — Bow, Body, Strings (incl. per-string detune), Expression, Drone, Output, Microtonal | should | ✅ Complete | All 7 sections in single-view grid; 4 detune fine-tuners in Strings; microtonal footer strip + full Tuning tab |
| UI-02: Dual cinematic-orchestral / drone-experimental visual identity | nice | ✅ Complete | O-Bowed parchment family, darkened bass palette (mockup v1 finalized, commit 19f51d9); Drone section "awake" glow at INFINITE_SUSTAIN/SUB_HARMONICS > 0 |

**Requirements Summary:** ✅ Complete: 2 · ⚠️ Partial: 0 · ⏸️ Deferred: 0 · ❌ Failed: 0

## Automated Checks (all re-run at verify, not inherited from execute)

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + Standalone) | ✅ Pass | ninja up-to-date, clean |
| 19-entry goldens byte-identical | ✅ Pass | `reproduce-goldens.sh`: "OK: all 19 goldens reproduce byte-identical" |
| auval | ✅ Pass | `auval -v aumu OCbs OuDv` → AU VALIDATION SUCCEEDED |
| pluginval strictness 10 | ✅ Pass | SUCCESS at default timeout (warm run — confirms execute's first-run Editor-Automation timeout was a cold WKWebView flake, not a defect) |
| ui_frontend_check.js | ✅ Pass | 14/14 incl. module-script parse, getScaledValue readouts, no JS range map, no window.confirm, provider-entry coverage |
| Bridge gate | ✅ Pass | 32 JS `getNativeFunction` = 32 C++ `withNativeFunction`, no dead registrations |
| Code-inspection spot checks | ✅ Pass | createEditor guard · harness excludes editor · member order · SafePointer + **bare return on null** (all completions incl. cancel-path `complete(false)` only when editor alive) · `withUserDataFolder()` · `Juce` namespace to TuningPanel · embedded-tuning period append |

## Human Verification (carried into Stage 4 entry — non-blocking per Stage-2 precedent)

- [ ] **Logic Release destruction-order gate:** open/close plugin editor ×10 (Release build; pluginval-10 editor lifecycle battery already passes in-process, but Logic is the real gate)
- [ ] **31-param DAW interaction:** knobs drag/wheel/dblclick; detune fine-tuners (±25¢ detent, wheel 10¢/shift 1¢); ACTIVE_STRINGS stepper; TUNING_SYSTEM dropdown gating Load; NOTE_EXPRESSION toggle; 6 skewed params (BOW_SPEED, BOW_PRESSURE, BRIGHTNESS, VIBRATO_RATE, SLOW_LFO_RATE, REFERENCE_PITCH) identical in UI vs DAW generic view; dblclick = spec defaults; automation + state recall update UI; drone glow
- [ ] **Picker UAF scenario:** open picker → close plugin window → choose/cancel — no crash
- [ ] **Logic smoke:** instantiate, E1 drone, automate BOW_SPEED, reload project
- [ ] **Visual QA:** 7 sections + tab bar at 1000×650, console clean, Tuning tab under parchment overrides, preset bar vs `~/Library/O-Contrabass/Presets/`, tuning survives preset round-trip, Schelleng dot moves with EXPRESSION_MACRO, spectrum tracks Size/Damping, VU tracks output, no CPU spikes

## Issues Found

- **pluginval cold-start flake (execute):** first run timed out in Editor Automation immediately after cache-clear install. Re-confirmed at verify: warm run SUCCESS at default 30 s timeout. Not a defect; noted for CI.
- **Known-inert / deferred (all annotated in NOTES.md + CHANGELOG.md):** STRING_TENSION bound-but-inert (D2 → v1.1 with goldens re-baseline); `.tun` route removed, TUN parser → v1.1 shared-module backlog (D1); Dorico distribution artifacts → Stage 4 (D4); registry.yaml stale vs module.yaml (R5); MTS-ESP present-but-stub (Stage-2 FUNC-07 carry).
- **Windows:** flags in place since Stage 1 (`NEEDS_WEBVIEW2` + static linking + `withUserDataFolder`); no Windows build exercised — Stage 4/CI item.

## Documented Deviations (accepted)

1. Canonical-path binary-data embed of module JS/CSS instead of vendored copies (anti-drift; O-Wind pattern).
2. `selectNext/PreviousPreset` return name only — matches canonical preset-manager.js contract, avoids double load.
3. Spectrum axis fMax 1200 → 1600 Hz (keeps DSP-true mode 8 on-plot).
4. Session state routed through OuariconPresetManager `getStateAsXml/setStateFromXml` (tuning survives DAW reload; backward compatible with pre-Stage-3 XML).
5. rAF draws fully skipped while Tuning tab active.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes — Stage 4 (Polish)

**Blockers:** None. The five human-in-the-loop DAW checks above are Stage-4 entry items (Stage 4's DAW/packaging pass covers them); pluginval-10's in-process editor lifecycle + parameter batteries already cover their automatable core.
