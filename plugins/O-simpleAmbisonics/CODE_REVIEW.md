---
phase: stage-4-polish
reviewed: 2026-10-14
version_reviewed: 1.0.0
depth: deep
files_reviewed: 22
files_reviewed_list:
  - plugins/O-simpleAmbisonics/CMakeLists.txt
  - plugins/O-simpleAmbisonics/Source/PluginProcessor.cpp
  - plugins/O-simpleAmbisonics/Source/PluginProcessor.h
  - plugins/O-simpleAmbisonics/Source/PluginEditor.cpp
  - plugins/O-simpleAmbisonics/Source/PluginEditor.h
  - plugins/O-simpleAmbisonics/Source/MeterFramePayload.h
  - plugins/O-simpleAmbisonics/Source/dsp/SpatialEngine.h
  - plugins/O-simpleAmbisonics/Source/dsp/DistanceModel.h
  - plugins/O-simpleAmbisonics/Source/dsp/ShEncoder.h
  - plugins/O-simpleAmbisonics/Source/dsp/ShRotator.h
  - plugins/O-simpleAmbisonics/Source/dsp/BinauralConvolver.h
  - plugins/O-simpleAmbisonics/Source/dsp/BinauralFilterData.h
  - plugins/O-simpleAmbisonics/Source/dsp/DecoderMixer.h
  - plugins/O-simpleAmbisonics/Source/dsp/StereoDecoder.h
  - plugins/O-simpleAmbisonics/Source/dsp/TripleBuffer.h
  - plugins/O-simpleAmbisonics/Source/ui/public/index.html
  - plugins/O-simpleAmbisonics/Source/ui/public/js/i18n.js
  - plugins/O-simpleAmbisonics/tests/render-harness/main.cpp
  - plugins/O-simpleAmbisonics/tests/pedalboard/verify_2_1.py
  - plugins/O-simpleAmbisonics/tests/ui-probes/frame-probe.mjs
  - plugins/O-simpleAmbisonics/tests/ui-probes/lesson-row-budget.mjs
  - plugins/O-simpleAmbisonics/scripts/generate_sh_icons.mjs
findings:
  critical: 1
  warning: 6
  info: 21
  total: 28
status: draft
open_findings: pending (Stage 4 Part 1)
sources: .planning/stages/4-polish/critic/{dsp,ui,architecture,foundation}.json (merged
  critic/unified.json: 3 blocking reports of ONE defect, 7 warnings, 25 notes / info);
  .planning/stages/3-gui/VERIFICATION.md (carried items)
---

# O-simpleAmbisonics v1.0.0: Code Review Report

**Reviewed:** 2026-10-14 (HEAD `a1886588`, the Stage 2–3 output)
**Depth:** deep — all four domain critics (dsp, ui, architecture, foundation); the plugin's first real review (Stage 2's dsp-critic was a smoke stub).
**Triage rule (PLAN P14):** blocker → fix; warning → fix unless it touches a frozen contract (none did — no STOP); note / info → fix when user-visible or a real defect AND local, else "accepted by design" / "reviewed, not changed" with a reason. Every DSP fix carries a harness gate plus a negative control that FAILS, and a PERF-01 re-run.

## Summary

| Tier | Found | Fix | By design / not changed | Folded into |
|---|---|---|---|---|
| Critical (blocker) | 1 (×3 critics) | 1 | — | T3 |
| Warning | 6 (+1 dup) | 6 | — | T3 (4), T6 (2) |
| Info / note | 21 | 10 | 11 | T3, T6, T8, T14 |
| Carried (Stage 3) | 3 | 3 | — | T4, T5 |

## Critical

### WR-01 (UI-001 / ARCH-001 / FND-001): Windows editor has no WebView2 backend
`Source/PluginEditor.cpp:254-264` — the options never call `.withBackend (Backend::webview2)`; on Windows `defaultBackend` is the legacy IE control, which ignores `withWinWebView2Options` and has no resource provider → "page not found" in every Windows host. Every macOS gate passes regardless. **Verdict: fix (T3)** — the O-simpleSubtractive v1.7.1 shape; static gate `grep -c withBackend Source/PluginEditor.cpp` = 1 added to R-STATIC S5.

## Warnings

### WR-02 (DSP-001 / ARCH-002): one NaN / Inf input sample silences the plugin for the session
`Source/dsp/DistanceModel.h:131-136` with the conditioner `Source/PluginProcessor.cpp:304-310`. The air-LP state latches NaN (0·NaN = NaN even at d ≤ 1 m); the block scrub then clears every block until the next prepare. **Verdict: fix (T3)** — non-finite conditioned samples → 0 in the conditioner; harness gate + NC.

### WR-03 (DSP-002): `AudioProcessor::reset()` not overridden
`Source/PluginProcessor.h` — VST3 `setProcessing (false)` / AU `Reset` leave the convolver, delays and air-LP state alive; a restart plays up to ≈ 6.7 ms of the previous tail. `SpatialEngine::reset()` exists and is allocation-free. **Verdict: fix (T3)** — override `reset()`; harness gate + NC.

### WR-04 (DSP-003): Input Mode switches in one sample
`Source/PluginProcessor.cpp:282, 307-308` — Mono sum ↔ Left only on differing L / R steps the source → click. **Verdict: fix (T3)** — a seeded 20 ms ramp on the left-only weight, exact branches at both ends (digests and the P1b anchors unchanged); harness click gate + splice NC.

### WR-05 (FND-002; DSP-005 / ARCH-004 notes): tail length reported as 0
`Source/PluginProcessor.h:76` — 64-sample latency + the 256-tap (at 48 k) MagLS FIR ring after the input stops; hosts that trust the tail can cut it. **Verdict: fix (T3)** — the real tail from the prepared rate; harness gate (measured decay ≤ reported) + NC (0 fails).

### WR-06 (FND-003; ARCH-005 note): lessons reset Input Mode
`Source/PluginProcessor.cpp:461-465` — only `outputLevel` is excluded; R-D3 makes `inputMode` user-owned. **Verdict: fix (T6)** — one user-owned set read by the apply core, the modified check and ST6 (amended).

### WR-07 (ARCH-003): lesson ids live in four hand-kept copies; a misspelled id looks applied
`PluginProcessor.cpp:431-440`, `index.html:988-995, 2418-2424`, harness ST6, `lesson-row-budget.mjs:65`. **Verdict: fix (T6 / T8 / T10)** — one table (`PresetRecipes.h`); the lit button and caption come from the C++ `presetState` push (a refused id lights nothing); R-STATIC S3 asserts page `data-preset` ids = table ids minus `init` and `FACTORY_LABELERS` keys = table ids.

## Info

| ID | Critic | Where | Finding | Verdict |
|---|---|---|---|---|
| IN-01 | DSP-004 | `PluginProcessor.cpp:298` | `processBlock` before `prepareToPlay` spins (maxBlock 0) | fix (T3) — clear + return |
| IN-02 | DSP-006 | `DistanceModel.h:69-72` | 0.2 m (+8.9 dB) × order-3 bank peak (+6.5 dB) ≈ +15 dBFS from 0 dBFS | accepted by design — a float output, no safety stage in a teaching tool; no factory recipe reaches the corner (G-ORDER3-DIST); CHANGELOG Known limitations |
| IN-03 | DSP-007 | `BinauralFilterData.h:58` | onsets 50–84 samples vs the planned ≈ 8 guard → brief comb during the 20 ms decoder fade | reviewed, not changed — re-trim moves the verdict digests; recorded in the ARCHITECTURE Stage-4 amendment (T14) |
| IN-04 | DSP-008 | `BinauralConvolver.h:130, 245` | K = 32 at 352.8 / 384 k trips a Debug-only jassert (limit 16) | fix (T3) — `kConvMaxK` 32 (an expectation, never a buffer bound) |
| IN-05 | DSP-009 | `BinauralConvolver.h:350` | FFTFallback per-instance SpinLock, uncontended | reviewed, not changed |
| IN-06 | UI-002 | `index.html:2547` | a session restored into an OPEN editor keeps the page language until reopen | accepted by design — language is not a parameter; reopen applies it |
| IN-07 | UI-003 | `index.html:2592` | DPR change between displays without a resize leaves blurry canvases | fix (T3) — `matchMedia` resolution listener → `applyUiScale` |
| IN-08 | UI-004 | `index.html:1945, 1951, 2211` | canvas ring labels ≈ 3.0:1, dB ruler ≈ 4.3:1 | fix (T3) — label alpha raised to ≥ 0.75 |
| IN-09 | UI-005 | `index.html:1254-1264` | knobs lack aria-valuemin/max/now, Home / End / PageUp / PageDown | fix (T3) |
| IN-10 | UI-006 | `i18n.js:695` | fr captions use decimal commas, readouts points | reviewed, not changed — settled at the native French review (fr stays `reviewed: false`) |
| IN-11 | UI-007 | `index.html:1306` | right / middle click on a knob opens a host gesture | fix (T3) — `e.button !== 0` → return |
| IN-12 | UI-008 | `PluginEditor.h:82` | 9.5–10 px captions ≈ 6 px at minScale 0.6 | accepted by design — fixed composition; fit-to-display picks the scale |
| IN-13 | UI-009 | `index.html:2418-2424` | lesson light / caption stale after edits and reopen | fix (T8) — `presetState` drives both (P11) |
| IN-14 | ARCH-005 | `PluginProcessor.cpp:455-480` | user-owned exclusion is a single string compare | fix (T6) — with WR-06 |
| IN-15 | ARCH-006 | `TripleBuffer.h:62-67` | a publish between the two exchanges can re-show an older frame for one tick | reviewed, not changed — display-only; shared sibling copy, revisit at module extraction |
| IN-16 | ARCH-007 | `ARCHITECTURE.md` | contract text predates Stages 2–4 | fix (T14) — Stage 4 amendments + implementation deltas |
| IN-17 | ARCH-008 | `index.html:744, 830, 1887` | C++ facts as page literals | partly fixed — `#chipLatency` retired (R-D1); the rest reviewed, not changed (latency and the distance range are frozen contracts) |
| IN-18 | FND-004 | `PluginEditor.cpp:398` | the open-time fit is persisted as editorScale | accepted by design — CONTEXT "save / reopen keeps the window size"; ARCHITECTURE A1 records it |
| IN-19 | FND-005 | `PluginProcessor.cpp:413-418` | a restore without / with a rejected editorScale keeps the previous instance's scale | fix (T3) — store 0 (fit-to-display), as documented |
| IN-20 | FND-006 | `CMakeLists.txt:18-32` | registers as plain Fx | fix (T3) — `VST3_CATEGORIES "Fx" "Spatial"` |
| IN-21 | FND-007 | `PluginProcessor.cpp:379` | pluginVersion stamped, never read | reviewed, not changed — the migration hook site for the first post-1.0.0 range change (ARCHITECTURE A1) |

Clean (no finding): DSP-010 / ARCH-009 — SN3D / ACN encoder and the AmbiX sign, the rotation recursion, UPOLS layout, bank fades, TripleBuffer peak gating, the resampler DC calibration, latency at one call site, 12 frozen parameters.

## Carried (Stage 3 VERIFICATION)

| ID | Item | Verdict |
|---|---|---|
| C-01 | `verify_2_1.py` V3 Binaural arm asserts the 2.1 single-sample response | fix (T4) — V3 narrowed to Stereo; Binaural onset → `verify_2_3.py` V2 |
| C-02 | en `label.encReadN` ellipsised at order 3 | fix (T5) — "(3+1)² = 16 ch"; header-fit H1 + NC |
| C-03 | chevron / puck overlap when azimuth ≈ yaw | fix (T5) — chevron drawn after the puck + tangential slide; frame-probe A14 + NC |

## Upstream (not plugin code; not called)

`modules/persistence/preset-manager` v1.0.9 — none of these is reached from this plugin (R-STATIC S2 = 0):
- the module's XML state path re-applies a stale `<CustomState>` child on a second reopen (memory `critical_preset_manager_stale_customstate_child`);
- its JSON apply resets every parameter to default and then SETS every saved key, including `outputLevel`;
- `savePresetToFile` has no factory-name guard (memory `pattern_savepresettofile_has_no_factory_guard`).

## Resolution log

Part 1 (Stage 4 T3–T11, 2026-10-14). "Resolved" = fixed in v1.0.0 with the named gate green and, for a DSP fix, its negative control FAILING as designed. Final statuses at T14.

| Finding | Status | Where / gate |
|---|---|---|
| WR-01 | resolved | `PluginEditor.cpp` `.withBackend (webview2)`; R-STATIC S5 `grep -c withBackend` = 1 (Windows load proven by the CP2 CI run only if authorized) |
| WR-02 | resolved | conditioner scrub; harness `S4 WR-02` (4 / 4 renders bit-equal to the zeroed input) + `S4 WR-02-NC` (engine-direct NaN latches → FAILS as designed) |
| WR-03 | resolved | `reset()` override; `S4 WR-03` (silence after reset all 0; tone after reset bit-equal to fresh) + `S4 WR-03-NC` (tail to frame 383 without reset) |
| WR-04 | resolved | 20 ms Left-only ramp with exact endpoints; `S4 WR-04` ratio 1.0026 vs the same-motion reference (2.52 / 2.10 vs the static pair, the 20 ms ramp's own corner) + `S4 WR-04-NC` splice ratio 579 |
| WR-05 | resolved | tail = (64 + L) / fs; `S4 WR-05` (48 k Binaural measured 317 ≤ reported 320) + `S4 WR-05-NC` |
| WR-06 | resolved | `isUserOwnedParam` (outputLevel, inputMode) read by the apply core, the modified check, the defs and the JSON parser; ST6 (amended), G-LESSON[owned] + G-LESSON-NC, G-PRESET-USER[roundtrip] + NC |
| WR-07 | resolved | `PresetRecipes.h` the one table; `presetState` lights the lesson (a refused id lights nothing); R-STATIC S3; lesson-row-budget [P11] arms |
| IN-01 | resolved | processBlock guard; `S4 IN-01` + `S4 IN-01-NC` (liveness) |
| IN-04 | resolved | `kConvMaxK` 32; `S4 IN-04` (K 30 / 32) + `S4 IN-04-NC` |
| IN-07 | resolved | `watchPixelRatio()` (matchMedia resolution → applyUiScale); boot-all-uis 0 errors (DPR switch itself: CP1 / CP3 eye-check) |
| IN-08 | resolved | ring / dB ruler label alpha 0.75 (≈ 5.8:1 on the well) |
| IN-09 | resolved | aria-valuemin / max / now; Home / End / PageUp / PageDown each one gesture |
| IN-11 | resolved | knob pointerdown primary button only |
| IN-13 | resolved | P11 (`presetState` drives the light + caption); lesson-row-budget [P11] arms |
| IN-14 | resolved | with WR-06 |
| IN-17 | partly resolved | `#chipLatency` retired (R-D1); the latency sentence moved into the Decoder tip; the rest reviewed, not changed |
| IN-19 | resolved | restore stores 0 when absent / rejected; `S4 IN-19` |
| IN-20 | resolved | `VST3_CATEGORIES "Fx" "Spatial"` |
| C-01 | resolved | `verify_2_1.py` V3 Stereo only; 22 / 22 ALL PASS on the installed VST3 |
| C-02 | resolved | `label.encReadN` "(n+1)² = N ch / can. / 声道"; header-fit H1 ×3 languages + `encreadn` NC |
| C-03 | resolved | chevron after the puck + `probe-anchor:chevron-slide`; frame-probe A14 (gap 8.25 px) + `chevron` NC |
| IN-02, IN-06, IN-12, IN-18 | accepted by design | reasons in the Info table |
| IN-03, IN-05, IN-10, IN-15, IN-21 | reviewed, not changed | reasons in the Info table |
| IN-16 | open → T14 | ARCHITECTURE Stage-4 amendments |
