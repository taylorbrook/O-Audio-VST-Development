---
phase: 3-gui
verified: 2026-06-25T00:00:00Z
status: human_needed
score: 13/13 statically-verifiable criteria PASS; 7 criteria DEFERRED to human DAW listen
re_verification: No — initial verification
human_verification:
  - test: "Play a held chord in the standalone/DAW and watch the Grain Cloud canvas"
    expected: "Sepia dots accumulate (read-position × pitch); raising Density thickens the cloud, Position/Pitch Spray widens it (UI-01)"
    why_human: "Requires live MIDI driving the audio-thread grain-event tap; the render path + event plumbing are verified in code but animation is not observable headlessly"
  - test: "Sweep Position/Scan and toggle Freeze while playing"
    expected: "Source-waveform playhead moves with Position+Scan, freeze-pin (snowflake) appears and pins the head; shaded green spray band tracks Position±PositionSpray (UI-02, FUNC-03)"
    why_human: "Live playhead motion + freeze pinning need a running audio thread feeding the GrainCloudFrame"
  - test: "Set Scatter to 0% then 100% on a sustained note and watch the Spectrum"
    expected: "Discrete sidebands / pitched comb at 0%, smearing toward a broadband noise floor at 100% (UI-04, DSP-05 visual)"
    why_human: "Spectrum motion is driven by live output audio through the VizRing → message-thread FFT"
  - test: "Watch the Grain/Overlap/CPU readout while raising Density and Grain Size on a held note"
    expected: "Grains N/192 climbs live, Overlap ×Y grows, CPU bar fills (UI-05)"
    why_human: "activeGrainCount is only non-zero with a running voice; the JS render is verified but the live values are not"
  - test: "Run all 8 concept presets and listen"
    expected: "Each isolates its concept audibly — Single Grain = separated grains, Pitched Buzz = pitched comb, Rect Click = audible clicks, Frozen Pad = sustained shimmer, etc. (FUNC-06 audible half)"
    why_human: "Preset values + the apply path are verified statically; audible character requires a DAW listen (and Stage-4 audit)"
  - test: "Drag a .wav onto the drop zone AND use the Load… picker in the live build"
    expected: "Both load and granulate a user source; oversized files show the 10 s truncation notice (FUNC-05 runtime)"
    why_human: "macOS WebView content-streaming drag-drop cannot be exercised headlessly; the full C++/JS path is verified statically"
  - test: "Open the editor and confirm the field-guide page renders (not blank) on macOS VST3 + AU"
    expected: "Single-page projector-readable field guide; every knob/combo/toggle two-way bound to host automation (UI-06, COMPAT-02)"
    why_human: "WebView render + host-automation round-trip is a visual/interactive check; code wiring + auval SUCCEEDED are confirmed"
---

# Phase 3 (GUI): O-simpleGrain Verification Report

**Phase Goal:** A single-page, projector-readable Ouaricon-Naturalist field-guide WebView — all 18 APVTS params two-way bound, drag-drop + picker source load, four live teaching visualizations + window inset + grain/overlap/CPU readout, per-control tooltips, and 8 concept presets — cross-platform-configured.

**Verified:** 2026-06-25
**Status:** human_needed (all statically-verifiable criteria PASS; runtime/audible criteria DEFERRED to human DAW listen + Stage 4)
**Re-verification:** No — initial verification

## Goal Achievement Verdict

**The Stage-3 GUI goal is achieved at the code + build level.** Every statically-verifiable acceptance gate passes against the actual codebase — not the agents' self-reports. The build is clean (`ninja … → no work to do`, binaries current) and **auval `aumu OsGr OuDv` reports `AU VALIDATION SUCCEEDED` live** (re-run during this verification, not trusted from the SUMMARY). The remaining 7 items are genuinely runtime-only (live audio-driven viz motion, audible preset character, WebView render/automation round-trip) and are correctly DEFERRED to the human DAW listen + Stage 4 validation, NOT counted as failures.

**No defects found.** All 18 params bind, the Load… path is fully wired, all four viz renderers exist and subscribe to the correct events, every control has a tooltip (33 data-tip keys ≡ 33 TIPS keys, exact), and all 8 presets are distinct substantive snapshots with exact C++↔HTML↔JS name parity.

---

## Verified Now vs Deferred — the split

| Verified NOW (code + build + auval) | Deferred to HUMAN / Stage 4 |
|---|---|
| All 18 APVTS params have relay + attachment + matching HTML control | Live host-automation → UI round-trip (visual) |
| Load… 5 native fns + JS drop bind + picker button wired | Drag-drop / picker actually loading a user .wav at runtime |
| 4 viz renderers exist + subscribe to the 4 exact event names | Audio-driven viz animation (cloud accumulation, scope/spectrum motion, playhead/freeze) |
| Window inset recomputed JS-side on windowShape change | — (inset render path verified; visual confirm trivial) |
| Grain/overlap/CPU readout wired to grainMeterUpdate | Live Grains N/192 climbing under load |
| 33 tooltips, exact data-tip ≡ TIPS parity | Tooltip hover surfacing (visual) |
| 8 presets: distinct snapshots, exact name parity, substantive | Audible/visual concept isolation per preset |
| Cross-platform CMake + Windows editor opts + bare-path provider | Windows VST3 render (config-parity only; no Win build) |
| No audio-thread FFT/alloc; scope-copied-before-FFT | — |
| Build clean (VST3+AU), auval SUCCEEDED | — |

---

## Observable Truths (PLAN must_haves)

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | WebView renders field-guide UI in macOS VST3+AU (no blank) | ✓ VERIFIED (render DEFERRED) | Bare-path resource provider `PluginEditor.cpp:40-71`; `webView->goToURL(...)` :221; auval SUCCEEDED. Visual render = human check. |
| 2 | All 18 APVTS params two-way bound | ✓ VERIFIED | 15 sliderIds + 2 comboIds + 1 toggleId `PluginEditor.cpp:82-89`; relays :92-97 (before WebView), attachments :190-218 (after); HTML 15 data-param + 2 combos + 1 toggle |
| 3 | Drag-drop OR Load… picker loads a user source | ✓ VERIFIED (runtime DEFERRED) | 5 fixed native fns `PluginEditor.cpp:118-145`; `bindSourceDrop` app.js:347, `bindLoadButton` :378; decode `convertFromBase64` PluginProcessor.cpp:531 |
| 4 | Four viz + inset + grain/overlap/CPU readout animate at 30 Hz | ✓ VERIFIED (animation DEFERRED) | Timer `startTimerHz(30)` :224; pushes 4 events :255-293; renderers `drawCloud/drawSourceWaveform/drawScope/drawSpectrum/drawWindowInset/drawGrainReadout` app.js |
| 5 | Every control has a tooltip; 8 presets load+isolate one concept | ✓ VERIFIED (audible DEFERRED) | 33 data-tip ≡ 33 TIPS (exact); 8 distinct presets `PluginProcessor.cpp:813-902`, name parity exact |

**Score:** 5/5 must-have truths code-verified (runtime/audible halves deferred to human)

---

## ROADMAP Acceptance-Criterion Table (per checkbox)

### Phase 3.1 — Layout + 18-param binding + cross-platform wiring

| ROADMAP 3.1 criterion | Status | Evidence (file:line) |
| --- | --- | --- |
| WebView opens, single-page, projector-readable (UI-06) | DEFERRED (human-listen/visual) | Provider `PluginEditor.cpp:45-68`, `goToURL` :221, `setSize(900,760)` :223; HTML single-page structure verified `index.html`; **visual render = human** |
| All knobs/menus/toggle two-way bound; relative-drag; freeze via getToggleState | ✓ PASS | Relays `PluginEditor.cpp:92-97` before WebView; attachments :190-218 after (member order `PluginEditor.h:59-70`); `bindKnob` relative ±135° drag app.js:172-231; `bindToggle` `Juce.getToggleState` :272-284 |
| Drag-drop + picker load a source (FUNC-05 wiring) | ✓ PASS (runtime DEFERRED) | 5 native fns registered `PluginEditor.cpp:118-145`; `commitDroppedFile` 3-fn chain app.js:324-345; `bindLoadButton` :378-392 |
| Renders macOS VST3+AU AND Windows VST3 (COMPAT-02) | ✓ PASS (Win by config-parity) | auval SUCCEEDED (live); CMake `IS_SYNTH/NEEDS_MIDI_INPUT/NEEDS_WEB_BROWSER/NEEDS_WEBVIEW2` `CMakeLists.txt:18-23`, static-link :132, `JUCE_USE_CURL=0` :133; Windows `withUserDataFolder("OsimpleGrain_WebView")` `PluginEditor.cpp:174-185` |

### Phase 3.2 — Four live visualizations + overlap/CPU readout

| ROADMAP 3.2 criterion | Status | Evidence (file:line) |
| --- | --- | --- |
| Grain cloud accumulates dots; density thickens, spray widens (UI-01) | DEFERRED (human-listen) | `grainCloudUpdate` push `PluginEditor.cpp:280-288`; `drawCloud` app.js:450-502; **accumulation needs live MIDI** |
| Source waveform: playheads + freeze point + spray range (UI-02) | DEFERRED (human-listen) | `getSourceThumbnail` PluginProcessor.cpp:402-433; `drawSourceWaveform` (playhead/freeze-pin/green spray band) app.js:508-600; **motion needs live audio** |
| Window inset matches shape + redraws on change (UI-03) | ✓ PASS | `drawWindowInset` JS recompute (5 closed-form windows) app.js:718-758; redraw on `windowShape.valueChangedEvent` :993-994; NOT per-frame (Pitfall 4) :295-297 |
| Scope/spectrum: sidebands at scatter 0 → noise (UI-04) | DEFERRED (human-listen) | `scopeUpdate`/`spectrumUpdate` push `PluginEditor.cpp:255-256`; `drawScope` app.js:614, `drawSpectrum` :647; sideband markers/carrierUpdate correctly DROPPED (grep=0); **sync→async needs live audio** |
| Grain/overlap/CPU readout updates live (UI-05) | DEFERRED (human-listen) | `grainMeterUpdate` push `PluginEditor.cpp:292`; `drawGrainReadout` app.js:766-788 (Grains N/192, Overlap=size×density, CPU bar); **live values need running voice** |
| No audio-thread FFT/alloc; scope before FFT (PERF-01) | ✓ PASS | FFT only in `GrainVizAnalyzer::process` (message thread) `VizAnalyzer.h:85-105`; scope copied (:89) BEFORE FFT (:105); processBlock has zero FFT (PluginProcessor.cpp:747 comment confirms copy-only tap) |

### Phase 3.3 — Pedagogical layer

| ROADMAP 3.3 criterion | Status | Evidence (file:line) |
| --- | --- | --- |
| Every parameter has a working hover tooltip (FUNC-07) | ✓ PASS | 33 `data-tip` keys (index.html) ≡ 33 `TIPS` keys (app.js:51-133) — **exact match**; `setupTooltips` wires every `[data-tip]` :858-869; covers 18 controls + load + dropZone + 4 viz + readout + 8 presets |
| Each preset loads + isolates its concept (FUNC-06) | ✓ PASS (audible DEFERRED) | 8 substantive distinct snapshots `PluginProcessor.cpp:813-902`; reset-to-default then apply :793-808; name parity exact: HTML data-preset ≡ C++ `name==` ≡ JS LESSONS keys; relays sync back; **audible isolation = human** |
| Layout stays single-page projector-readable (UI-06) | DEFERRED (visual) | Single `.frame` structure index.html; `setSize(900,760)` :223; **projector readability = human** |

---

## Required Artifacts

| Artifact | Status | Details |
| --- | --- | --- |
| `Source/ui/public/index.html` | ✓ VERIFIED | 15 data-param + 2 combos + 1 toggle + 8 tour-btn + drop zone + 2×2 viz grid + inset + readout + 33 data-tip; `type="module"` scripts |
| `Source/ui/public/css/styles.css` | ✓ VERIFIED | viz-grid/side-rail/source-drop-zone/window-inset/grain-readout/.combo rules present (18 matches); zero right/bottom canvas stretch; calc() sizing :706-707 |
| `Source/ui/public/js/app.js` | ✓ VERIFIED | 18-param bind, 4 canvas renderers + inset + readout, drop+picker, 33-key tooltip map, 8-preset tour; 9 Juce.getNativeFunction, 0 window.__JUCE__ native-fn calls |
| `Source/PluginEditor.cpp` | ✓ VERIFIED | relays→WebView→attachments, bare-path provider, 9 native fns, 30 Hz timer pushing 4 viz events; Windows opts |
| `Source/PluginEditor.h` | ✓ VERIFIED | Member order relays(:59-62)→WebView(:65)→attachments(:67-70) correct |
| `Source/PluginProcessor.cpp` | ✓ VERIFIED | applyFactoryPreset 8 snapshots :813-902; getSourceThumbnail min/max envelope :402-433; drop decode convertFromBase64 :531 |

---

## Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| Editor timerCallback | app.js setupVizEvents | emitEventIfBrowserIsVisible → backend.addEventListener | ✓ WIRED | 4 names match exactly: scopeUpdate/spectrumUpdate/grainCloudUpdate/grainMeterUpdate (C++ :255-292 ↔ JS :706-709) |
| app.js drop bind | PluginProcessor dropSession*/loadSourceFromFileChooser | Juce.getNativeFunction | ✓ WIRED | dropSessionStart/AddFile/CommitFile + loadSourceFromFileChooser all via Juce namespace (app.js:328-336,383) |
| Editor relays+attachments | ParamIDs (18) | WebSliderRelay×15 + WebComboBoxRelay×2 + WebToggleButtonRelay×1, 3-arg attach + nullptr | ✓ WIRED | All 18 IDs bound; `nullptr` undoManager `PluginEditor.cpp:197,207,217`; jassert(param!=nullptr) per loop |

---

## MUST-HOLD Invariants (RESEARCH §) — all 12 held

| # | Invariant | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Member order relays→WebView→attachments | ✓ | PluginEditor.h:59-70 |
| 2 | 3-arg attach + nullptr undoManager + jassert | ✓ | PluginEditor.cpp:190-218 |
| 3 | Resource provider bare-path direct equality + charset | ✓ | :45-68 (no scheme strip) |
| 4 | Juce namespace for params/native-fns; window.__JUCE__.backend for viz events | ✓ | 9 Juce.getNativeFunction, 0 via __JUCE__; viz events on backend (app.js:703-709) |
| 5 | Canvas DPR backing store + calc() sizing | ✓ | makeCanvas app.js:398-411; CSS calc :706-707; 0 right/bottom |
| 6 | base64 = convertFromBase64 | ✓ | PluginProcessor.cpp:531 |
| 7 | No audio-thread FFT/alloc | ✓ | FFT only VizAnalyzer.h:105 (message thread) |
| 8 | Scope copied before FFT | ✓ | VizAnalyzer.h:89 (copy) before :105 (FFT) |
| 9 | Windows WebView2 checklist | ✓ | NEEDS_WEBVIEW2 + static-link + withUserDataFolder |
| 10 | Drop native-fn names FIXED | ✓ | dropSessionStart/AddFile/CommitFile/CommitFolder + loadSourceFromFileChooser unchanged |
| 11 | Unique temp-dir prefix OsimpleGrain_WebView | ✓ | PluginEditor.cpp:182 |
| 12 | Viz event names exact C++↔JS | ✓ | 4 names match |

---

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| AU registered | `auval -a \| grep simplegrain` | `aumu OsGr OuDv … O-simpleGrain-dev` | ✓ PASS |
| AU validates | `auval -v aumu OsGr OuDv` | `AU VALIDATION SUCCEEDED.` | ✓ PASS |
| Build clean | `ninja O-simpleGrain_VST3 O-simpleGrain_AU` | `no work to do` (binaries current) | ✓ PASS |
| VST3 + AU artifacts | `ls …/Release/{VST3,AU}` | both `.vst3` + `.component` present + installed | ✓ PASS |

---

## Anti-Patterns Found

| File | Pattern | Severity | Impact |
| --- | --- | --- | --- |
| (none) | TBD/FIXME/XXX/PLACEHOLDER/"not yet implemented" scan on all 6 modified files | — | Clean — zero debt markers |

---

## Gaps Summary

**No gaps blocking the Stage-3 goal.** Every statically-verifiable acceptance criterion passes against the actual code, the build is clean, and auval validates live. The 7 human_needed items are runtime-only by nature (live audio-driven visualization motion, audible preset character, WebView render + host-automation round-trip, live drag-drop) and were explicitly scoped to the human DAW listen + Stage 4 validation in the plan — they are deferrals, not failures. No binding, tooltip, preset, or wiring is missing or misnamed.

**Recommended next step:** the human checkpoint listen (the 7 items above), then `/plugin-verify O-simpleGrain` Stage-4 validation (pluginval VST3+AU sweep, preset/artifact/aliasing/freeze audit, Windows drag-drop smoke test).

---

_Verified: 2026-06-25_
_Verifier: Claude (gsd-verifier)_
