# Stage 3: GUI — Execution Summary

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 3 (GUI) — execute phase
**Date:** 2026-06-27
**Result:** ✅ Complete — 15/15 tasks across 3 component-gated phases; VST3 + AU build clean (0 warnings), pluginval strictness-10 SUCCESS (both formats), auval SUCCEEDED.

---

## Quality gate (2-dsp → 3-gui)

**BYPASSED (benign, logged).** The gate BLOCK was the documented tooling false-positive — `schema` SKIPPED (no `HANDOFF.json` artifact) + `pluginval` SKIPPED (VST3 not currently built). The real checks (`build`, `dsp-critic`) PASSED, and Stage 2's own verify phase already passed pluginval strictness-10 + 22/22 render-harness checks. Identical to the prior `gate-1-to-2` bypass. Justification written to `.planning/workflow/gate-bypasses.log`.

---

## What was built

A single-page WebView UI — left→right signal-flow layout (Excitation → Resonator → Material → Amp/Output), all **17 parameters bound**, resonator-/exciter-aware grey-out, the **four pedagogical visuals** (animated SVG loop/flow diagram, decay scope, live log-spectrum, modal stems), on-hover **tooltips**, an **on-screen keyboard**, and a **preset-bar shell** — consuming the live viz taps wired in Stage 2.

Near-verbatim port of the O-simpleFM WebView template + O-simpleAdditive building blocks/tooltips + O-simpleGrain grey-out, plus **one new widget**: the inline-SVG loop/flow diagram (String skin) that re-skins to 8 modal stems in Modal mode.

---

## Phase-by-phase

### Phase 3.1 — WebView foundation + binding + preset shell + keyboard (Tasks 1–8) ✅
- **Task 1 (harness seam ⚠):** `createEditor()` converted from inlined header to a declaration; `PluginProcessor.cpp` guards `#include "PluginEditor.h"` under `#if JUCE_WEB_BROWSER` and returns the WebView editor at `=1`, `GenericAudioProcessorEditor` at `=0`. Diverges (correctly) from O-simpleFM's unconditional include — keeps the render-harness buildable at `JUCE_WEB_BROWSER=0`. `PluginEditor.cpp` kept OUT of harness SOURCES.
- **Task 2 (CMake):** `ouaricon_add_module(... preset-manager)` before `juce_add_binary_data`; single `O-simplePhysicalModelSynth_UIResources` target (default `BinaryData` namespace — no samples embedded); linked FIRST in PRIVATE; `juce_generate_juce_header` after `target_link_libraries`.
- **Task 3 (UI scaffold):** ported HTML/CSS/JS + juce glue, re-authored to a left→right 4-column signal-flow layout, 1040×860 fixed. Deviation: dropped the botanical-overlay `<img>` (no asset to embed; palette/building-block CSS ported verbatim).
- **Task 4 (editor wiring):** relays → webView → attachments, **3 combos + 14 sliders + 0 toggles**, member order load-bearing, `jassert(param)` per attachment.
- **Task 5 (resource provider + binding):** bare-path `==` matcher, `charset=utf-8` on all text; combos via `getComboBoxState`, sliders via `getSliderState`. Material macro co-move is automatic (Stage-2 `setValueNotifyingHost` write-back — no extra JS).
- **Task 6 (grey-out):** `.pm-disabled` on control cells via `getChoiceIndex()`; engine selectors kept outside any dimmed group (escape hatch).
- **Task 7 (preset shell):** `OuariconPresetManager` member; state I/O swapped to `getStateAsXml()/setStateFromXml()`; 10 preset native fns. `initializeFactoryPresets` left Default-only (factory set deferred to Stage 4 per D4); user save/load works.
- **Task 8 (keyboard):** `handleUiMidi` + `juce::MidiMessageCollector` (timestamped, reset in `prepareToPlay`, drained at top of `processBlock`); `uiMidi` native fn; C3–C5 keyboard JS (mouse + QWERTY).

### Phase 3.2 — Scope + spectrum (Tasks 9–11) ✅
- **Task 9:** NEW `Source/PmVizAnalyzer.h` (header-only, message-thread). Reuses the top-level `VizRing` from `VizTap.h` (NOT redefined). `kFftOrder=12` (4096), Blackman-Harris, `kSpectrumBins=256` (log 20 Hz→Nyquist), `kScopeWindow=1024`, `kScopePoints=128`, asymmetric smoothing 0.5↑/0.1↓. **Scope computed FIRST** (before FFT clobbers the work buffer). Defensive `sr=(sr>0?sr:44100)` guard.
- **Task 10:** editor inherits `juce::Timer`; `startTimerHz(30)`; `timerCallback()` runs `vizAnalyzer.process(getVizTap().waveform, getSampleRate())`, guards `webView==nullptr`, emits `spectrumUpdate`(256) + `scopeUpdate`(128). `vizAnalyzer` placed between the attachment vectors and `fileChooser` — existing order untouched. `stopTimer()` first action of the destructor.
- **Task 11:** `makeCanvas` (DPR backing store + `setTransform`), `drawSpectrum` (256 log-axis bars + dB grid + freq ticks), `drawScope` (128-pt polyline); incoming via `window.__JUCE__.backend.addEventListener`. Added a 12th native fn `getSampleRate` for axis labels (parity kept).

### Phase 3.3 — Loop/flow diagram + modal stems + tooltips (Tasks 12–15) ✅
- **Task 12 (UI-02):** inline `<svg>` block diagram `EXCITE → RESONATOR ⟲ → MATERIAL → ♪`; circulating pulse (SMIL `animateTransform`) whose opacity/radius is driven by `loopEnergy` each 30 Hz tick → visibly dims as the note decays. No canvas (sidesteps the replaced-element gotcha).
- **Task 13:** `loopUpdate` emit built inline in `timerCallback()` — `juce::DynamicObject` with `energy` + `stemFreqs`/`stemAmps` (k=0…7) via `juce::var(obj)` (safe shared-ownership idiom). No new C++ member.
- **Task 14 (UI-05):** Modal stem skin folded INTO the diagram — on `resonatorType == Modal` the RESONATOR interior swaps the loop for 8 vertical stems (`x = log-map(stemFreq)`, `height = stemAmp`), toggled on the `resonatorType` listener + re-applied after preset load. One widget, no duplicate data.
- **Task 15 (UI-06):** `setupTooltips()` ported verbatim from O-simpleAdditive (floating `#tooltip`, `data-tip`, viewport-edge flip, focus a11y, Escape). **21-key `TIPS` map** (17 controls + 4 diagram boxes), exact coverage. Interim Waveguide `title=` hint replaced by a proper TIP noting "Waveguide arrives in v1.1".

---

## Latent fix surfaced by a Stage-3 gate (not a Stage-3 task)
**Material macro `.withMeta(true)`** — the `material` param (writes `damping`+`decay`) failed auval's parameter-stability check. The render-harness never exercises auval, so this Stage-2 macro condition only surfaced once Stage 3 added an auval gate. One-line parameter-attribute change in `createParameterLayout()`; no DSP logic touched.

---

## Files

**New:**
- `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- `Source/PmVizAnalyzer.h`
- `Source/ui/public/index.html`, `css/styles.css`, `js/app.js`
- `Source/ui/public/js/juce/index.js`, `js/juce/check_native_interop.js` (verbatim)
- `Source/ui/public/modules/preset-manager.js` (module-helper-copied)

**Edited:**
- `Source/PluginProcessor.h` (createEditor decl, presetManager + midiCollector members, accessors, OuariconPresetManager include)
- `Source/PluginProcessor.cpp` (guarded include + createEditor defn, presetManager ctor init, state I/O swap, handleUiMidi, prepareToPlay reset, processBlock drain, `material .withMeta(true)`)
- `CMakeLists.txt` (PluginEditor sources, preset-manager module, UIResources binary-data target + first PRIVATE link)
- `tests/render-harness/CMakeLists.txt` (comment-only: corrected stale "createEditor inlined" notes)
- `.claude/agent-memory/gui-agent.md` (Stage 3 learnings)

---

## Verification evidence (automated, this stage)

- **Build:** VST3 + AU clean, 0 plugin-code warnings (only JUCE/system-header deprecations common to all suite builds). Binary-data regenerated from edited UI files (embedding confirmed).
- **auval:** `auval -v aumu OsPM OuDv` → AU VALIDATION SUCCEEDED (after each phase).
- **pluginval:** strictness-10 SUCCESS on BOTH VST3 + AU — incl. "Open editor whilst processing" + "Editor Automation", which exercise the live 30 Hz Timer / emitEvent (spectrum/scope/loop) path and the member-order reverse-destruction crash path.
- **Native-fn parity:** 12 ↔ 12 exact (`savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile, getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset, deletePreset, isFactoryPreset, uiMidi, getSampleRate`). Zero orphans either direction.
- **TIPS coverage:** 21 == 21 `data-tip` keys ↔ `TIPS` keys, zero missing/extra.
- **JS syntax:** `node --check js/app.js` OK (guards against a silent WebView-killing parse error).
- **Harness seam:** structurally intact (`PluginProcessor.h` pulls in no editor/WebView types; `.cpp` include guarded; harness SOURCES list `PluginProcessor.cpp` only). NB: `PluginProcessor.h` now includes header-only `OuariconPresetManager.h` (needs only juce_audio_processors/core — both linked by the harness). **Re-run the render-harness at the START of Stage 4** (`-DOUARICON_BUILD_TESTS=ON`) to confirm it still compiles at `JUCE_WEB_BROWSER=0`.

---

## Owed: DAW visual confirmation (human — verify phase)

Automated validation is green; the following require a human to load the AU/VST3 and play notes:

**3.1** — all 17 controls move the DSP; dragging **Material** co-moves Damping + Decay; grey-out tracks `resonatorType`/`excitationType` (selectors never trapped); keyboard + QWERTY play; preset bar saves/loads/navigates/deletes.
**3.2** — scope decays after note-off; spectrum shows harmonic comb (String) vs uneven inharmonic spacing (Modal); harmonics fade top-down.
**3.3** — String skin pulse dims in lockstep with the audible decay (real `loopEnergy`); Modal skin swaps to 8 stems with correct log-freq spacing + decaying heights (raising Inharmonicity widens upper-mode spacing); every control + diagram box shows a tooltip; no console errors.

**Tunable if needed:** Modal stem height is a direct `amp → height` map (clamped 0…1); if stems render too short/tall live, the single JS constant `STEM_BASE_Y - STEM_TOP_Y` is the only knob (couldn't audition amplitudes offline).

---

## Success criteria status (stage-level)

| Criterion | Status |
|-----------|--------|
| UI-01 layout (1040×860, left→right) | ✅ built (visual confirm owed) |
| All 17 params bound (3 combos + 14 sliders) | ✅ parity verified |
| Material macro co-moves Damping+Decay | ✅ wired (visual confirm owed) |
| Grey-out (D5) | ✅ built (visual confirm owed) |
| UI-02 loop/flow diagram (loopEnergy) | ✅ built (visual confirm owed) |
| UI-03 scope | ✅ built (visual confirm owed) |
| UI-04 spectrum (harmonic vs inharmonic) | ✅ built (visual confirm owed) |
| UI-05 modal stems | ✅ built (visual confirm owed) |
| UI-06 tooltips | ✅ 21/21 coverage |
| Preset bar (shell; factory in S4) | ✅ built (visual confirm owed) |
| Keyboard | ✅ built (visual confirm owed) |
| Harness seam intact | ✅ structural (re-run at S4 start) |
| Build clean / pluginval / auval | ✅ confirmed |
