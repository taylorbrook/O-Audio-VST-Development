# Stage 3: GUI — Research

**Researched:** 2026-07-24
**Domain:** JUCE 8 WebView UI (WebBrowserComponent + Web*Relay bindings), Ouaricon Naturalist aesthetic
**Confidence:** HIGH — every claim below verified against local JUCE 8.0.14 sibling implementations in this repo (no external sources needed)

---

## User Constraints (from CONTEXT.md — locked, do not re-litigate)

- **D7:** Standalone audition deferred → Stage 4 entry gate. Stage 3 touches **no DSP**.
- **D8:** Aesthetic = **Ouaricon Naturalist** (`.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`).
- **D9:** Layout = grouped sections as signal flow: **TIME** (syncMode, noteDivision/delayTime swap) | **GRAIN** (grainSize, density) | **FEEDBACK** (feedback, lowCut, highCut) | **OUTPUT** (width, mix).
- **D10:** Visualization = **none** — knobs + readouts only; no C++→JS polling bridge, no editor Timer.
- **UI-01:** Two-way binding for all 10 params. **UI-02:** syncMode swaps noteDivision↔delayTime visibility, no dead controls.
- Readouts via `SliderState.getScaledValue()` (6 of 10 params skewed) — never a JS min/max map.

---

## Answers to the 3 Open Questions

### Q1: Mockup production path → **Direct-author production files; skip the ui-mockup two-agent workflow**

Evidence [VERIFIED: local codebase]:
- **No recent Stage-3 sibling used the ui-mockup workflow.** O-simpleGrain, O-simpleSampler, and O-simpleBeatmaker all have **no `.planning/mockups/` directory** — each direct-authored `Source/ui/public/{index.html, css/styles.css, js/app.js}` by cloning the previous sibling's field-guide base (sampler SUMMARY: "cloned grain field-guide base verbatim"; beatmaker: "copied js/juce/* from O-simpleSubtractive").
- The ui-mockup skill (Phase A: `v[N]-ui.yaml` + `v[N]-ui-test.html`; Phase B: 5 scaffolding files incl. PluginEditor boilerplate + CMakeLists) exists to iterate on **undecided** aesthetics/layouts. Here D8/D9/D10 fully constrain the design, and the editor/CMake scaffolding it produces is inferior to cribbing the proven O-simpleGrain editor directly.
- CONTEXT.md itself anticipated this: "a non-interactive single-pass mockup is expected to suffice."

**Recommendation:** In Phase 3.1, author `Source/ui/public/index.html` + `css/styles.css` + `js/app.js` directly from the Naturalist aesthetic doc, and satisfy the ROADMAP's "mockup finalized first" requirement by **opening index.html in a browser against a ~20-line JUCE-bridge stub** (mock `js/juce/index.js` exporting fake `getSliderState`/`getComboBoxState`) before any C++ wiring. That browser render IS the mockup review — one pass, visually verified, and it doubles as the TDZ/label-overwrite regression check the memory patterns mandate.

### Q2: Window size → **Fixed 940 × 440**

Survey of sibling `setSize` calls [VERIFIED: grep across plugins/*/Source/PluginEditor.cpp]:

| Plugin | Size | Character |
|---|---|---|
| O-Marimba | 600×400 | few knobs, Naturalist default |
| O-Freeze | 550×530 | small capture effect |
| O-AnalogSaturation | 600×450 | knob row |
| O-Comp | 620×360 | knob row |
| O-simpleGrain | 900×760 | knobs + 4 viz canvases + keyboard |
| O-simpleSampler | 980×720 | waveform editor + viz |
| O-GrainScatter | 900×850 | granular + viz |

O-ReverseDelay has **9 visible controls** (8 knobs + syncMode segmented toggle + one of noteDivision/delayTime in the shared slot) in **4 group panels on a single left→right row** (matches D9's signal-flow reading). Budget at Naturalist spacing (55–60 px knobs, 25 px in-group gaps, 40–60 px group separation, 20–30 px edge margins): TIME ≈ 190 px, GRAIN ≈ 185 px, FEEDBACK (3 knobs) ≈ 270 px, OUTPUT ≈ 185 px + separators/margins ≈ **~900–940 wide**. Height: header/title (~70) + group panels (label + knob + readout ≈ 180–200) + generous margins ≈ **~420–460**.

**Recommendation: `setSize(940, 440)`** — single row of four framed group panels, botanical overlay bleeding off the right edge at 0.35 opacity behind the OUTPUT group. No viz means no reason to go taller; wider-than-tall reads as a hardware delay unit and keeps the "specimen page" framing.

### Q3: noteDivision control style → **Naturalist-styled `<select>` dropdown bound via WebComboBoxRelay; syncMode as a two-segment button pair**

- The aesthetic doc is explicit [VERIFIED: aesthetic.md §Application Guidelines]: "Choice parameters (2–3 options) → toggle buttons in row; Choice parameters (4+ options) → dropdown styled to match aesthetic." noteDivision has **13** choices → dropdown; syncMode has 2 → toggle-button pair.
- A stepped knob for 13 unlabeled detents is a poor fit — dotted/triplet names ("1/8D", "1/4T") must be readable at a glance, and a dropdown shows the full menu.
- The proven binding already exists in O-simpleGrain `app.js` `bindCombo()` [VERIFIED: plugins/O-simpleGrain/Source/ui/public/js/app.js:237]: populate `<option>`s from `st.properties.choices` (arrives via `propertiesChangedEvent` — **never hardcode the 13 strings in JS**; the C++ StringArray is the source of truth), sync with `st.getChoiceIndex()`, write with `st.setChoiceIndex(sel.selectedIndex)`.
- **syncMode** is an `AudioParameterChoice` (Free/Sync), **not** a bool — it must use `WebComboBoxRelay` (WebToggleButtonRelay is for bool params only). Render it as two Naturalist green segmented buttons (FREE | SYNC) driven by the same ComboBox state (`getChoiceIndex`/`setChoiceIndex`), which both looks right and gives UI-02 a crisp switch.

**UI-02 swap mechanism (pure JS, zero native functions):** TIME group contains a fixed-dimension `.time-slot` div holding BOTH the delayTime knob wrap and the noteDivision select wrap. Both stay relay-bound at all times (relays/attachments are cheap and always live — no rebinding churn, no dead controls). A listener on the syncMode state toggles a `hidden` class:

```js
const sync = Juce.getComboBoxState("syncMode");
function refreshTimeSlot() {
  const isSync = sync.getChoiceIndex() === 1;   // {Free, Sync}, default 1 = Sync
  divisionWrap.classList.toggle("hidden", !isSync);
  delayWrap.classList.toggle("hidden",  isSync);
}
sync.valueChangedEvent.addListener(refreshTimeSlot);
sync.propertiesChangedEvent.addListener(refreshTimeSlot);  // initial state after page load
refreshTimeSlot();
```

Identical slot dimensions for both wraps → zero layout shift on switch. Note the **default is Sync (index 1)** — first open shows the noteDivision dropdown.

---

## Naturalist Template Summary

**Path:** `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` (with `metadata.json`) [VERIFIED]

- **Background:** aged paper `#F5E6D3`/`#EBD9C7`, panels `#D4C4B0`, container frame `3px solid #5C4033` + heavy drop shadow (framed-specimen look).
- **Typography:** Garamond serif; title 22–26 px w/ 2–3 px letter-spacing; UPPERCASE labels 9–11 px w/ 0.5–1 px spacing; subtle white emboss text-shadow.
- **Knobs:** 55–65 px "botanical seed cross-section" — conic-gradient 10-segment cream pattern (`#F5DEB3`/`#E8D5B7`), walnut dividers/border `#8B7355`, cream core `#FFF8DC`, inset+drop shadows; indicator = 2–3 px stem line rotating with value; hover scale 1.05.
- **Buttons/toggles:** green tint `rgba(139,168,112,0.3)` + border `#3C5C1A`; active `rgba(107,142,35,0.6)` + `#2C3E10`; optional fleuron ❦ at 0.3 opacity.
- **Botanical overlay:** ONE transparent PNG per plugin, right side, height 60–75 %, opacity 0.3–0.4, `pointer-events:none`, slight bleed off the right edge. Source library: `/Users/taylorbrook/Dev/Ouaricon Audio Images/[category]/*.png` → copy to `Source/ui/public/img/`. Effects (delay/modulation) guideline: flora / insects / birds. O-simpleGrain used `insects.png`; pick **birds** or **flora** for variety (final pick at 3.1 mockup).
- **9-param guidance:** compact 55 px knobs, 25 px gaps, grouped sections — matches D9 exactly.
- Full CSS variable block provided in the doc (§Example Color Codes) — copy verbatim.

---

## Parameter → Relay/Control Binding Table

From actual `Source/PluginProcessor.cpp createParameterLayout()` [VERIFIED: lines 26–106]. 10 params = **8 WebSliderRelay + 2 WebComboBoxRelay + 0 toggle relays**.

| # | ID | Type | Range / Choices | Default | Skew | Relay | Control | Readout format |
|---|----|------|-----------------|---------|------|-------|---------|----------------|
| 1 | `delayTime` | Float | 50–2000 ms, step 0.01 | 500 | **setSkewForCentre(316)** | WebSliderRelay | knob (TIME slot, visible in Free) | `getScaledValue()` → "500 ms" (≥1000 → "1.25 s" optional) |
| 2 | `syncMode` | Choice | {Free, Sync} | 1 (Sync) | — | **WebComboBoxRelay** | 2-segment button pair | segment highlight from `getChoiceIndex()` |
| 3 | `noteDivision` | Choice | 13: 1/16…1/1 incl. D/T | 6 (1/4) | — | **WebComboBoxRelay** | styled `<select>` (TIME slot, visible in Sync) | option text from `properties.choices` |
| 4 | `grainSize` | Float | 50–500 ms, step 0.01 | 200 | **setSkewForCentre(158)** | WebSliderRelay | knob (GRAIN) | "200 ms" |
| 5 | `density` | Float | 0–100 %, step 0.1 | 60 | linear | WebSliderRelay | knob (GRAIN) | "60 %" |
| 6 | `feedback` | Float | 0–100 %, step 0.1 | 40 | linear | WebSliderRelay | knob (FEEDBACK) | "40 %" |
| 7 | `lowCut` | Float | 20–2000 Hz, step 0.01 | 100 | **setSkewForCentre(200)** | WebSliderRelay | knob (FEEDBACK) | "100 Hz" |
| 8 | `highCut` | Float | 500–20000 Hz, step 0.01 | 8000 | **setSkewForCentre(3162)** | WebSliderRelay | knob (FEEDBACK) | "8.0 kHz" (<1000 → Hz) |
| 9 | `width` | Float | 0–100 %, step 0.1 | 60 | linear | WebSliderRelay | knob (OUTPUT) | "60 %" |
| 10 | `mix` | Float | 0–100 %, step 0.1 | 35 | linear | WebSliderRelay | knob (OUTPUT) | "35 %" |

**Skew flags:** 4 params carry non-unity skew (delayTime, grainSize, lowCut, highCut). CONTEXT.md says "6 of 10 skewed" — the actual shipped code has **4** (density/feedback/width/mix are linear; the draft spec may have differed — bind to what exists). Either way the rule stands: readouts and knob-angle mapping come from `getScaledValue()`/`setNormalisedValue()` on the state object; the JS carries **format-only** functions (unit suffix, decimals), never range constants. **Verification spot-check (ROADMAP 3.2 criterion): delayTime knob at 12 o'clock must read ≈ 316 ms, not 1025 ms.**

**Note:** `parameters` is a **public member** on `ReverseDelayProcessor` [VERIFIED: PluginProcessor.h:47] — the editor accesses `processorRef.parameters.getParameter(id)` directly (no `getAPVTS()` accessor needed, unlike grain).

---

## Reference Implementation: crib from **O-simpleGrain**, subtract

`plugins/O-simpleGrain/Source/PluginEditor.{h,cpp}` + `Source/ui/public/` is the canonical Stage-3 pattern [VERIFIED: read in full], already re-proven by O-simpleSampler (cloned it, pluginval@5 green incl. editor tests) and O-simpleBeatmaker (pluginval@10 green). For O-ReverseDelay, take:

- **Keep:** member-order pattern (relays → webView → attachments), Options chain (`withNativeIntegrationEnabled` + `withKeepPageLoadedWhenBrowserIsHidden` + `withResourceProvider`), `withOptionsFrom(*relay)` loop, bare-path `getResource()`, 3-arg attachments with `nullptr` undoManager + `jassert(param)`, `goToURL(getResourceProviderRoot())`, `#if JUCE_WINDOWS withWinWebView2Options(...withUserDataFolder(tempDir/"OReverseDelay_WebView").withStatusBarDisabled().withBuiltInErrorPageDisabled())`, `js/juce/{index.js, check_native_interop.js}` copied **byte-identical**, `app.js` `bindKnob`/`bindCombo` functions.
- **Drop:** everything else — drag-drop streaming module, FileChooser, on-screen keyboard/`uiMidi`, viz analyzer, Timer, `withInitialisationData`, all 9 of grain's native functions. This plugin's editor is **pure relays** — the simplest WebView editor in the suite.
- **Only native function:** `getParameterDefaults` (for dblclick-reset readout defaults — memory pattern: `properties` has no default field). Returns `{id: engineeringDefault}` for all 10 params via `param->convertFrom0to1(param->getDefaultValue())`. That makes the native-fn grep-diff trivially 1 ≡ 1.

---

## WebView Integration Checklist (CMake + editor files)

Current state [VERIFIED: CMakeLists.txt]: no WebView flags, **no existing binary-data target** (namespace `BinaryData` is technically free — O-simpleBeatmaker precedent), harness compiles `PluginProcessor.cpp` with `JUCE_WEB_BROWSER=0`, `createEditor()` currently returns `GenericAudioProcessorEditor` from inside PluginProcessor.cpp.

1. **`juce_add_plugin(OuariconReverseDelay ...)`** — add `NEEDS_WEB_BROWSER TRUE` and `NEEDS_WEBVIEW2 TRUE` (target name is **OuariconReverseDelay**, not the folder name — memory: build_script_target_name_vs_folder).
2. **Compile definitions** — add `JUCE_WEB_BROWSER=1` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (BOTH required or Windows WebView silently blanks; set now for Windows-readiness even though macOS-only today — grain set them at Foundation, we retrofit at 3.1).
3. **UI binary data** —
   ```cmake
   juce_add_binary_data(OuariconReverseDelay_UIResources
       NAMESPACE UIBinaryData
       HEADER_NAME UIBinaryData.h
       SOURCES Source/ui/public/index.html
               Source/ui/public/css/styles.css
               Source/ui/public/js/app.js
               Source/ui/public/js/juce/index.js
               Source/ui/public/js/juce/check_native_interop.js
               Source/ui/public/img/<botanical>.png)
   ```
   Use `UIBinaryData` even though no collision exists **today** — Stage 4 presets or future embedded assets would otherwise claim `BinaryData` and collide (suite convention; grain/sampler precedent). Link it PRIVATE; place after `juce_generate_juce_header`.
4. **Editor files** — new `Source/PluginEditor.{h,cpp}`; add `PluginEditor.cpp` to `target_sources` of the plugin only (**never** the harness). Member order: relays → `unique_ptr<WebBrowserComponent>` → attachments (destroy-in-reverse; wrong order = release-build crash on reload).
5. **`createEditor()` harness guard** — in PluginProcessor.cpp:
   ```cpp
   juce::AudioProcessorEditor* ReverseDelayProcessor::createEditor()
   {
   #if JUCE_WEB_BROWSER
       return new ReverseDelayEditor(*this);
   #else
       return new juce::GenericAudioProcessorEditor(*this);   // harness build
   #endif
   }
   ```
   with `#include "PluginEditor.h"` inside the same `#if JUCE_WEB_BROWSER` guard. Harness CMakeLists already defines `JUCE_WEB_BROWSER=0` and lists only PluginProcessor.cpp [VERIFIED: tests/render-harness/CMakeLists.txt:23,40] — this guard is the only change it needs. The header's own comment already mandates staying editor-include-free [VERIFIED: PluginProcessor.h:13].
6. **Resource provider** — bare-path exact matching (`url == "/"` → index.html, `"/css/styles.css"`, `"/js/app.js"`, `"/js/juce/index.js"`, `"/js/juce/check_native_interop.js"`, `"/img/<botanical>.png"`); `charset=utf-8` on text types; return `std::nullopt` otherwise. **Never** strip a scheme — paths arrive already bare.
7. **HTML** — `<script type="module" src="js/app.js">`; app.js starts with `import * as Juce from "./juce/index.js";` and passes the **`Juce` module namespace** to any helper (never `window.__JUCE__`).
8. **Editor size** — `setSize(940, 440)`; WebView bounds = full local bounds in `resized()`.
9. **Re-run harness** after the editor lands (Phase 3.1 gate): `cmake -DOUARICON_BUILD_TESTS=ON` build + run `O-ReverseDelay-render-test` → exit 0.
10. **Build/install** — `./scripts/build-and-install.sh O-ReverseDelay` (handles AU cache + dual-variant sweep); auval `aufx ORvD OuDv`; pluginval strictness 5 at 3.2 (strictness 10 ×3 is the Stage-4 gate).

---

## Curated Pitfalls (relevant to THIS plugin only)

Not applicable here (skip): drag-drop/webkitGetAsEntry, async FileChooser SafePointer, canvas replaced-element sizing, offline-render AsyncUpdater CC gaps, retired-map reapers — this UI has no files, no canvases, no native-fn data plumbing beyond defaults.

| # | Pitfall (memory pattern) | Concrete mitigation for O-ReverseDelay |
|---|---|---|
| 1 | **Render harness breaks on WebView editor** (`pattern_render_harness_breaks_on_webview_editor`) | Checklist items 4–5 + 9. Harness sources already exclude editor files; the `#if JUCE_WEB_BROWSER` guard in createEditor is the one edit. Re-run harness as a 3.1 test criterion AND again at Stage 4 entry. |
| 2 | **Dual binary-data namespace collision** (`critical_dual_binary_data_namespace_collision`) | `NAMESPACE UIBinaryData` + `HEADER_NAME UIBinaryData.h` (BOTH — distinct header alone still collides symbols). |
| 3 | **`Juce` ES-module namespace, not `window.__JUCE__`** (`critical_juce_webview_namespace_vs_postmessage`) | `import * as Juce` in app.js; all state getters through it. No `window.__JUCE__` anywhere except JUCE's own `check_native_interop.js`. |
| 4 | **Resource provider receives bare paths** (`critical_webview_resource_provider_and_schemes`) | Exact `==` matching on `"/..."` strings (grain's `getResource` verbatim). Never hard-code `juce://` schemes. |
| 5 | **JS top-level init TDZ kills the whole UI silently** (`pattern_module_toplevel_init_tdz`) | Single `init()` invoked at the **bottom** of app.js after all module-level `let/const`; verify in a browser against the ~20-line bridge stub before first C++ build (this is also the Q1 mockup step). |
| 6 | **Knob readouts from a JS min/max map drift from C++ skew** (`pattern_webview_knob_readout_scaled_value`) | All readouts + knob angle from `st.getScaledValue()`/normalised value; JS holds unit-format functions only. Spot-check: delayTime midpoint ≈ 316 ms (ROADMAP 3.2 criterion). |
| 7 | **Dblclick-reset needs defaults via native fn** (same pattern) | One `getParameterDefaults` native fn (properties carry no default). If dblclick-reset is cut, cut the fn too — do not hardcode defaults in JS. |
| 8 | **Native-fn bridge gaps fail silently** (`pattern_webview_native_fn_bridge_gap`) | grep-diff `getNativeFunction` (JS) vs `withNativeFunction` (editor) — expected surface: exactly 1 (`getParameterDefaults`). Port `ui_frontend_check.js` (below) to pin this. |
| 9 | **Shared JS updater overwrites HTML-authored labels** (`pattern_js_state_updater_overwrites_html_labels`) | syncMode segment buttons: author FREE/SYNC text in HTML, have `refresh()` toggle classes/`aria-pressed` only — never write `textContent` onto authored labels. Render the page (browser stub), don't just read it. |
| 10 | **Combo choices not present at first load** | grain's `bindCombo` handles it: build `<option>`s inside a refresh subscribed to BOTH `propertiesChangedEvent` and `valueChangedEvent` [VERIFIED: app.js:255–263]. Same for the UI-02 initial visibility state. |
| 11 | **Windows WebView2 runtime** (`critical_webview2_runtime_gotchas_windows`, `critical_webview2_static_linking`) | `withUserDataFolder(temp/"OReverseDelay_WebView")` + `withStatusBarDisabled` + `withBuiltInErrorPageDisabled` under `#if JUCE_WINDOWS`; both CMake flags (checklist 1–2). macOS-only today, but wire it now — retrofits are how 34/35 plugins ended up missing the flags. |

**Frontend regression script:** port `plugins/O-Contrabass/tests/ui_frontend_check.js` [VERIFIED] → `plugins/O-ReverseDelay/tests/ui_frontend_check.js`, adapting paths/regexes (external `js/app.js`, no preset/tuning modules). Run: `node plugins/O-ReverseDelay/tests/ui_frontend_check.js` (exit code = failed assertions). It statically pins pitfalls 5/6/7/8 + resource-provider closure. Run it at the 3.2 gate.

---

## Phase 3.1 / 3.2 Scope (aligned with ROADMAP)

**Phase 3.1 — Layout and Basic Controls**
1. Author `Source/ui/public/{index.html, css/styles.css, js/app.js}` from the Naturalist doc (Q1 path): 940×440, four framed group panels TIME|GRAIN|FEEDBACK|OUTPUT, seed-cross-section knobs, syncMode segment pair, noteDivision `<select>`, botanical PNG copied from `/Users/taylorbrook/Dev/Ouaricon Audio Images/` (birds or flora).
2. Copy `js/juce/{index.js, check_native_interop.js}` byte-identical from O-simpleGrain.
3. Browser-stub render check (mockup gate + TDZ/label checks).
4. CMake wiring (checklist 1–3), `PluginEditor.{h,cpp}` (grain clone minus extras), createEditor guard.
5. Gate: WebView opens at 940×440, all 9 visible controls render per layout; **harness still builds and passes**.

**Phase 3.2 — Parameter Binding and Interaction**
1. All 10 relays + attachments live; readouts via `getScaledValue()`; knob drag (relative vertical, grain's `bindKnob`).
2. UI-02 swap listener (pure JS, Q3 mechanism); default state = Sync → dropdown visible.
3. `getParameterDefaults` native fn + dblclick-reset.
4. Gates: two-way binding (drag→DSP, host automation→UI, preset→all controls); swap leaves no dead controls; delayTime midpoint ≈ 316 ms; native-fn grep-diff clean (1≡1); `ui_frontend_check.js` exit 0; build-and-install + auval + pluginval@5 (VST3+AU, editor tests).

**Out of scope (Stage 4):** standalone audition (D7 entry gate), factory presets/OuariconPresetManager, pluginval@10 ×3, CHANGELOG.

---

## Sources

All HIGH confidence — local repository, read this session:
- `plugins/O-ReverseDelay/Source/PluginProcessor.{h,cpp}` (param contract, public `parameters`, harness constraint comment)
- `plugins/O-ReverseDelay/CMakeLists.txt`, `tests/render-harness/CMakeLists.txt`
- `plugins/O-simpleGrain/Source/PluginEditor.{h,cpp}`, `Source/ui/public/js/app.js`, `CMakeLists.txt` (reference implementation)
- `plugins/O-simpleSampler/.planning/stages/3-gui/SUMMARY.md`, `plugins/O-simpleBeatmaker/.planning/stages/3-gui/SUMMARY.md` (Stage-3 precedent: direct authoring, validation gates)
- `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` (visual system)
- `plugins/O-Contrabass/tests/ui_frontend_check.js` (regression-script port source)
- Project memory patterns cited inline (MEMORY.md keys)
