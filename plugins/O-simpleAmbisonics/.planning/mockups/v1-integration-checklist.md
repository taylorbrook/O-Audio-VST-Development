# Stage 3 (GUI) Integration Checklist - v1

**Plugin:** O-simpleAmbisonics
**Mockup Version:** v1 (finalized 2026-10-13, `v1-ui.yaml` `finalized: true`)
**Generated:** 2026-10-13
**Design brief:** `.planning/stages/3-gui/CONTEXT.md` (UI-01 … UI-08) — cadence: **3.1 + 3.2 → build / install → user visual checkpoint → 3.3** (one GSD cycle; single RESEARCH / PLAN / SUMMARY / VERIFICATION, no suffix).

The generated files this checklist integrates:

| File | Destination | Phase |
|---|---|---|
| `v1-ui.html` | `Source/ui/public/index.html` | 3.1 + 3.2 (one page: bindings, visuals, tooltips, i18n are all in it) |
| `v1-i18n.js` | `Source/ui/public/js/i18n.js` | 3.1 (the page imports it; a missing file is a BLANK page) |
| `v1-PluginEditor-TEMPLATE.h/.cpp` | `Source/PluginEditor.h/.cpp` (adapt, do not paste blind) | 3.1 |
| `v1-CMakeLists-SNIPPET.txt` | merged into `CMakeLists.txt` | 3.1 |
| `../parameter-spec.md` | the immutable 12-parameter contract | read-only |

## 0. Entry check (re-read at the boundary)
- [ ] `Source/ui/` holds only `.gitkeep`; `PluginProcessor.cpp` `createEditor()` (~line 352) still returns the Generic editor inline
- [ ] `parameter-spec.md` lists the 12 frozen camelCase ids and matches `createParameterLayout()` byte-for-byte (ranges, skews, steps, choice strings)
- [ ] Stage-2 verdicts recorded before any processor edit: harness 241/241, digest-2.1 `9189d911a1fe98d5`, digest-2.2 `7f35bc9f91e321b9`, auval SUCCEEDED, pluginval s10 VST3 + AU
- [ ] `./scripts/build-and-install.sh O-simpleAmbisonics` works on the Stage-2 tree before anything changes

## 1. Copy UI Files (3.1)
- [ ] `mkdir -p Source/ui/public/js/juce Source/ui/public/img`
- [ ] Copy `v1-ui.html` → `Source/ui/public/index.html`
- [ ] Copy `v1-i18n.js` → `Source/ui/public/js/i18n.js`
- [ ] Copy the JUCE 8 frontend library (`index.js` **and** `check_native_interop.js`, from the JUCE install or a sibling such as O-simpleWavetable `Source/ui/public/js/juce/`) → `Source/ui/public/js/juce/`
- [ ] Copy `insects.png` (the botanical overlay; sibling O-simpleAdditive `Source/ui/public/img/insects.png`) → `Source/ui/public/img/insects.png`
- [ ] EB Garamond is a direct embed from `modules/ui/eb-garamond` via `${CMAKE_SOURCE_DIR}` (nothing to copy); the page links `css/eb-garamond.css` and the stylesheet's `url('../fonts/…')` resolves against `/css/`
- [ ] **Generator deviations from the test mockup, for the designer's eye at the visual checkpoint:**
  - the language `<select>` moved from the header strip INTO the settings popover (`#gear-btn` → `#lang-select` + `#tips-toggle`), per the html-generation contract (exactly one hover-help switch beside `#lang-select`; check-i18n [16])
  - the compass rose's cardinal words (front / left / back / right) are glyphs (▲ · +90° · ±180° · −90°) — canvas text is gate-blind to localization, so the words moved to the DOM key under the well
  - segmented rows are kept for Order (1st/2nd/3rd, bound to the `order` ComboBoxState) and Decoder (Binaural/Stereo); Input stays a `<select>`; Orbit is Off | On over the ToggleState — the design's controls, bound through the contract's state API
  - the SH pattern icons are procedural SVG built with the DOM (never `innerHTML`); the 16 offline-generated SVGs (research item) are a drop-in swap of `shIconSvg()`
  - stations and header chips are **width-pinned** (French ellipsizes instead of pushing); the band tag column is 34 px
  - word readouts (`in front`, `level`, `wide beam`, …) are written through `setLabel` with literal keys so the language sweep owns them; numeric readouts are `FORMAT[id](st.getScaledValue())`
  - `applyFactoryPreset` takes a STABLE id: `pointSource`, `orderStep`, `behindYou`, `overhead`, `rotateRoom`, `farNear`, `slowOrbit`, `speakersHeadphones` (the captions localize; the ids do not)

## 2. Update PluginEditor and Processor Files (3.1)
- [ ] Create `Source/PluginEditor.h` from `v1-PluginEditor-TEMPLATE.h`; verify member order **relays → webView → attachments** (8 `WebSliderRelay`, 3 `WebComboBoxRelay`, 1 `WebToggleButtonRelay`)
- [ ] Create `Source/PluginEditor.cpp` from `v1-PluginEditor-TEMPLATE.cpp`; initialisation order matches declaration order; 3-arg attachments with `nullptr` undoManager; `.withOptionsFrom()` for all 12 relays
- [ ] Natives registered: `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `uiReady`, `applyFactoryPreset` — then `grep -o "getNativeFunction('[^']*')" Source/ui/public/index.html | sort -u` and diff against the `withNativeFunction` names (must be identical; an unregistered native never settles its promise)
- [ ] `getResource()` serves every `juce_add_binary_data` SOURCE by bare path, incl. `/js/i18n.js` as `application/javascript; charset=utf-8` and the four eb-garamond paths (`text/css; charset=utf-8`, `font/woff2`)
- [ ] 30 Hz `juce::Timer`: `meterFrames.read()` every tick, quantise (gains / peaks / mix 1e-3, angles 0.01°), FNV-1a hash-gate, `emitEventIfBrowserIsVisible("meterFrame", …)`; `uiReady` and `applyFactoryPreset` force the next emit. Payload field names are `MeterFrame.h`'s (`azEffDeg`, `elEffDeg`, `azRotDeg`, `elRotDeg`, …) — the page reads exactly those
- [ ] O-simpleWavetable v1.0.1 fix present: editor is an `AudioProcessorParameter::Listener`, `parameterGestureChanged` tracked in `gestureOpen[12]`, `closeOpenGestures()` in the destructor BEFORE the attachments die
- [ ] Window (ui-design-rules Rule 4, resizable pattern): `editorScale` read BEFORE the first `setSize`; numeric `setSize (1180, 780)` literal stays the LAST numeric `setSize` in the file (the headless gates parse it); `setResizable(true, true)`, `setResizeLimits` 0.6×–1.5× of 1180×780, `getConstrainer()->setFixedAspectRatio(1180.0/780.0)`, `fitScaleForDisplay()` for a fresh editor, `editorScale` written in `resized()`
- [ ] `#if JUCE_WINDOWS` → `withWinWebView2Options(... withUserDataFolder(temp/OsimpleAmbisonics_WebView) ...)`
- [ ] **Processor edits (the ONLY ones CONTEXT.md allows; O-simpleAdditive / O-simpleFM `PluginProcessor.h/.cpp` are the reference):**
  - `PluginProcessor.cpp` `createEditor()`: `#if JUCE_WEB_BROWSER` `#include "PluginEditor.h"` above it → `return new OSimpleAmbisonicsAudioProcessorEditor (*this);` / `#else` Generic editor — so `tests/render-harness` (`JUCE_WEB_BROWSER=0`, no editor sources) keeps building
  - `float editorScale = 0.0f;` (message thread; 0 = fit the display; session-lifetime — CONTEXT.md names it in the state XML too: decide in research whether it persists like O-simpleAdditive's `uiLanguage` or stays session-only like its `editorScale`, and grow the `verify_2_*` state round-trip gates accordingly)
  - `std::atomic<int> uiLanguage { 0 };` + `static juce::String languageCode (int)` (`1 → "fr"`, `2 → "zh-Hans"`, else `"en"`) + `static int languageIndex (const juce::String&)` (unknown → 0); persisted as a non-parameter `uiLanguage` property on `apvts.state`, written in `getStateInformation` BEFORE `createXml()`, restored in `setStateInformation` only when present (an older session keeps English)
  - `bool applyFactoryPreset (const juce::String& lessonId)`: reset EVERY parameter to its default (`beginChangeGesture` / `setValueNotifyingHost (getDefaultValue())` / `endChangeGesture`), then the recipe, **never `outputLevel`**; provisional recipes from `v1-ui.yaml` `special_elements.lesson_row.recipes` (Binaural recipes keep headroom: order-3 bank +6.5 dB peak, 0.2 m adds +8.9 dB — Stage 4 refines + adds the FUNC-08 bank); returns false for an unknown id
  - nothing in `processBlock` / the engine
- [ ] **After any processor edit (every time):** `tests/render-harness` 241/241 with digest-2.1 `9189d911a1fe98d5` + digest-2.2 `7f35bc9f91e321b9` asserted; `auval -v aufx OSAm OuDv` SUCCEEDED; pluginval strictness 10 VST3 + AU; `tests/pedalboard/verify_2_1.py`, `verify_2_2.py`, `verify_2_3.py` on the INSTALLED VST3 (the 12-value state round-trip grows by the new state properties — update the gate's expectation, never weaken it)

## 3. Update CMakeLists.txt (3.1)
- [ ] Add `Source/PluginEditor.cpp` + `Source/PluginEditor.h` to `target_sources`
- [ ] Add the `juce_add_binary_data(O-simpleAmbisonics_UIResources …)` block from `v1-CMakeLists-SNIPPET.txt` (the ONLY binary-data target; default `BinaryData` namespace; includes `Source/ui/public/js/i18n.js` — check-i18n [8])
- [ ] Link `O-simpleAmbisonics_UIResources` first in `target_link_libraries`
- [ ] Already present (verify, do not duplicate): `juce::juce_gui_extra`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `NEEDS_WEBVIEW2 TRUE`, `juce_generate_juce_header` after `target_link_libraries`
- [ ] Re-run CMake configure (new sources + binary data)

## 4. Build and Test (Debug, 3.1 + 3.2)
- [ ] Build succeeds with 0 warnings
- [ ] Standalone loads the WebView (not blank, not Times New Roman — EB Garamond resolved)
- [ ] Right-click → Inspect: console shows no JavaScript errors, no `i18n: tip target not found`, no 404
- [ ] `window.__JUCE__` exists; all 12 controls move their parameter and follow host changes
- [ ] The meters move: play audio, pan the source front → left and watch **X (ACN 3) hand off to Y (ACN 1) while W (ACN 0) stays at +1** (UI-01 acceptance); Order 1 → 2 → 3 lights 4 / 9 / 16 rows
- [ ] The puck follows azEff / elEff and NEVER moves with Yaw; the compass rose, beam and rim ticks do
- [ ] Top-down drag sets Azimuth (wraps cleanly across ±180°), side drag sets Elevation; both record as one host gesture

## 5. Build and Install (Release) → user visual checkpoint
- [ ] `./scripts/build-and-install.sh O-simpleAmbisonics` (dual-variant sweep + AU cache clear)
- [ ] **Rebuild the Standalone explicitly** (`ninja O-simpleAmbisonics_Standalone`) — it stays stale under that script (project memory)
- [ ] Release build: no crash on plugin reload (close / reopen the editor 10×) — tests member order
- [ ] Logic Pro: automation writes move the UI; the editor hidden then re-shown re-sends `uiReady` and the meters resume; save / reopen keeps the window size (`editorScale`)
- [ ] **STOP for the user's visual checkpoint** (meters, puck, compass, decoder view by eye) before 3.3

## 6. 3.3 — tooltips, lessons, i18n
- [ ] Tooltip copy is already in `js/i18n.js` (en from the mockup's TIPS table); confirm each anchor in `TIP_BINDINGS` resolves (boot-all-uis below reports DEAD / LATE)
- [ ] The 8 lesson buttons apply through `applyFactoryPreset(id)`; the footer caption changes; `outputLevel` never moves
- [ ] fr: machine draft at `reviewed: false` — Taylor reviews; two `termNote` exemptions (rotation `Pitch` → `Tangage`; the ear-level `level` word) stay until a reviewer says otherwise
- [ ] zh-Hans: machine draft at `reviewed: 'mt'` — back-translate to `'bt'` per the house zh rollout; the same two `termNote`s
- [ ] Width budget in **French first** (project memory): the 8 lesson buttons + the right-aligned hint on one line at 1180 px; station pills and chips are width-pinned so they ellipsize rather than push
- [ ] Choice strings (`Binaural` / `Stereo`, `Mono sum` / `Left only`, `1` / `2` / `3`) stay English in every language (I18N_EXEMPT, scoped)

## 7. WebView-Specific Validation
- [ ] No viewport units in CSS (`100vh`, `100vw`, …); `html, body { height: 100% }`
- [ ] `user-select: none`; context menu + dragstart disabled
- [ ] Resource provider returns every file (no 404s in the inspector's Network tab); correct MIME types (`application/javascript; charset=utf-8` for .js, `font/woff2`)
- [ ] `#tooltip` is a body child OUTSIDE `#plugin` (a transformed ancestor would break `position: fixed`)
- [ ] At 0.6× (min) every text stays legible (9.5 px floor × 0.6 on a Retina display); at 1.5× nothing pixelates (canvases fold `uiScale` into their backing store)

## 8. UI gates (on the REAL tree, before the Stage-3 commit)
Run from the repo root. ALL must pass.

- [ ] `node scripts/check-i18n.js --plugin O-simpleAmbisonics` — exit code = number of failed assertions [1]–[16]; 0 = pass
- [ ] `node scripts/i18n-fr-lint.js --plugin O-simpleAmbisonics` — exit 2 on any finding; 0 = clean
- [ ] `node scripts/i18n-zh-lint.js --plugin O-simpleAmbisonics` — exit 2 on any finding; 0 = clean (entries at `reviewed: 'mt'` are counted as BELOW SHIP BAR, not failed)
- [ ] `node scripts/check-ui-labels.js --plugin O-simpleAmbisonics` — needs Playwright; 0 = pass, n > 0 = n assertions failed (the geometry diff names what a French string pushed)
- [ ] `node scripts/boot-all-uis.js --plugin O-simpleAmbisonics --strict-tips` — needs Playwright; 0 = run completed (read the table: 0 page errors, 0 404s, 0 dead tips), 2 = a DEAD tip binding
- [ ] Optional per-plugin fixtures the gates read if present: `tests/i18n-states.json` (states to measure — settings popover open, Stereo decoder, order 3, orbit on) and `tests/ui-stub/generic-overrides.json` (seed values for the generic bridge stub so readouts measure at real widths)

**Exit 77 from check-ui-labels or boot-all-uis means Playwright could not be resolved and NOTHING was verified. It is not a pass.** Install it (`npx playwright install chromium`) and re-run.

The first three also run in CI on every push to main (`.github/workflows/ui-static-gates.yml`), so a failure there turns main red.

**Finalization-time dry run (2026-10-13, fixture tree, not the real tree):** see the JSON report of the finalization agent for which of these already pass against `v1-ui.html` + `v1-i18n.js` + the two templates. Stage 3 re-runs everything on the real tree.

## 9. Hands-on (Standalone + Logic Pro)
- [ ] Tab reaches every knob, and the arrow keys move it; a clicked knob takes focus
- [ ] Drag a knob out of the window and release outside: the gesture ends and the knob stops following the cursor (pointer capture)
- [ ] Double-click resets each knob to its default (`getParameterDefaults`)
- [ ] The hover-help switch toggles tooltips and persists across closing and reopening the editor (`localStorage` `osamb.tipsEnabled`); the gear and the switch keep explaining themselves while it is off
- [ ] Switching the language to fr and to zh-Hans relabels the page with no clipping; the two `<select>`s and the Decoder segment stay English
- [ ] With macOS Reduce Motion on (System Settings → Accessibility → Display), no transition runs (knob hover scale, station highlight fade, band fade)
- [ ] **Resizable frame:** drag-resize in Logic and Standalone — the aspect holds, the whole page scales (no reflow), the tooltip stays unscaled and clamped to the window, drags still track the pointer at 0.6× and 1.5×, canvas text stays sharp
- [ ] Logic: hide / re-show the plugin window → meters resume within a tick (`uiReady` re-sent on `visibilitychange`)
- [ ] Both decoders at order 3 with the Overhead lesson: no clipping on the output meter (headroom note in `v1-ui.yaml`)

## Parameter List (from parameter-spec.md — 12, IDs frozen)

| # | ID | Type | Range / Choices | Default | Relay | Attachment |
|---|---|---|---|---|---|---|
| 1 | `azimuth` | Float | −180 … +180 °, interval 0 | 0 | `WebSliderRelay` | `WebSliderParameterAttachment` |
| 2 | `elevation` | Float | −90 … +90 °, interval 0 | 0 | `WebSliderRelay` | `WebSliderParameterAttachment` |
| 3 | `distance` | Float | 0.2 … 20 m, step 0.001, skew for centre 2 m | 1.0 | `WebSliderRelay` | `WebSliderParameterAttachment` |
| 4 | `order` | Choice | `1` / `2` / `3` | 0 (`1`) | `WebComboBoxRelay` | `WebComboBoxParameterAttachment` |
| 5 | `yaw` | Float | −180 … +180 °, interval 0 | 0 | `WebSliderRelay` | `WebSliderParameterAttachment` |
| 6 | `pitch` | Float | −180 … +180 °, interval 0 | 0 | `WebSliderRelay` | `WebSliderParameterAttachment` |
| 7 | `roll` | Float | −180 … +180 °, interval 0 | 0 | `WebSliderRelay` | `WebSliderParameterAttachment` |
| 8 | `decoder` | Choice | `Binaural` / `Stereo` | 0 (`Binaural`) | `WebComboBoxRelay` | `WebComboBoxParameterAttachment` |
| 9 | `orbit` | Bool | Off / On | false | `WebToggleButtonRelay` | `WebToggleButtonParameterAttachment` |
| 10 | `orbitRate` | Float | 0.02 … 2 Hz, step 0.001, skew for centre 0.2 Hz | 0.1 | `WebSliderRelay` | `WebSliderParameterAttachment` |
| 11 | `inputMode` | Choice | `Mono sum` / `Left only` | 0 (`Mono sum`) | `WebComboBoxRelay` | `WebComboBoxParameterAttachment` |
| 12 | `outputLevel` | Float | −60 … 0 dB, step 0.1 (−60 = −inf) | 0 | `WebSliderRelay` | `WebSliderParameterAttachment` |

Relays: 12. Attachments: 12. Natives: 5. Events: 1 (`meterFrame`).
