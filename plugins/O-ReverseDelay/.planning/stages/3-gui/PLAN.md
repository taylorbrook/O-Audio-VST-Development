# Stage 3: GUI — Execution Plan

**Created:** 2026-07-24
**Source of truth:** RESEARCH.md (this directory) — approach locked; CONTEXT.md D7–D10 — decisions locked
**Reference implementation:** `plugins/O-simpleGrain/Source/PluginEditor.{h,cpp}` + `Source/ui/public/` (crib, then subtract drag-drop / keyboard / viz / Timer / withInitialisationData / all 9 grain native fns)

---

## Goal

Ship a working WebView GUI for O-ReverseDelay: fixed **940×440** window, **Ouaricon Naturalist** aesthetic, single row of 4 framed group panels **TIME | GRAIN | FEEDBACK | OUTPUT** (signal-flow order, D9), all 10 parameters two-way bound (UI-01), sync/free conditional time control with zero dead controls (UI-02), no visualization and no C++→JS polling bridge (D10) — with the offline render harness still green and the plugin passing auval + pluginval strictness 10 in both formats.

**Ground truth for param IDs/ranges:** `Source/PluginProcessor.cpp` `createParameterLayout()` (verified this session). 10 params = **8 WebSliderRelay + 2 WebComboBoxRelay** (`syncMode` AND `noteDivision` are Choice params — no toggle relays). **4 skewed params:** `delayTime` (centre 316), `grainSize` (158), `lowCut` (200), `highCut` (3162). CONTEXT/ROADMAP's "6 of 10 skewed" is stale draft-spec language — bind to what exists. `parameters` is a public member on `ReverseDelayProcessor`; the target name is **OuariconReverseDelay** (not the folder name).

---

## Phase 3.1 — WebView Infrastructure + Bindings

### Task 1: Author frontend (direct-author, no ui-mockup workflow)

**Files (create):**
- `Source/ui/public/index.html`
- `Source/ui/public/css/styles.css`
- `Source/ui/public/js/app.js`
- `Source/ui/public/js/juce/index.js` — **byte-identical copy** from `plugins/O-simpleGrain/Source/ui/public/js/juce/index.js`
- `Source/ui/public/js/juce/check_native_interop.js` — byte-identical copy, same source
- `Source/ui/public/img/<botanical>.png` — copy from `/Users/taylorbrook/Dev/Ouaricon Audio Images/` (**birds or flora** — grain used insects; pick for variety)

**Spec:**
- Layout: 940×440 body; header/title (~70 px, Garamond serif, 22–26 px, 2–3 px letter-spacing); one row of 4 framed group panels TIME | GRAIN | FEEDBACK | OUTPUT.
- Naturalist system from `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`: aged paper `#F5E6D3`/`#EBD9C7`, panels `#D4C4B0`, `3px solid #5C4033` frame + drop shadow; copy the doc's CSS variable block verbatim. Seed-cross-section knobs 55–60 px (conic-gradient 10-segment cream, walnut border, stem-line indicator). Botanical PNG right side, 60–75% height, opacity 0.3–0.4, `pointer-events:none`, bleeding off the right edge behind OUTPUT.
- Controls: 8 knobs; `syncMode` = two-segment button pair (FREE | SYNC, green tint per aesthetic, **labels authored in HTML**); `noteDivision` = Naturalist-styled `<select>` (options populated at runtime from `properties.choices` — **never hardcode the 13 strings**).
- TIME group contains a fixed-dimension `.time-slot` div holding BOTH the delayTime knob wrap and the noteDivision select wrap (identical slot dimensions → zero layout shift). Swap logic lands in Task 7; author both wraps + `hidden` class now.
- `<script type="module" src="js/app.js">`; app.js starts `import * as Juce from "./juce/index.js";`. Any helper/panel receives the **`Juce` module namespace** as an argument — never `window.__JUCE__`.
- app.js structure: all module-level `let`/`const` declared first; single `init()` invoked at the **bottom of the file** (TDZ pattern, Pitfall A).

**Depends on:** nothing.

### Task 2: Browser-stub render check (replaces the mockup gate)

**Files (create, throwaway or keep under `tests/`):**
- `tests/ui-stub/juce-stub.js` (~20 lines): mock `getSliderState` / `getComboBoxState` / `getNativeFunction` returning fake state objects (`getScaledValue`, `getNormalisedValue`, `setNormalisedValue`, `getChoiceIndex`, `setChoiceIndex`, `properties` with the 13 division choices, `valueChangedEvent`/`propertiesChangedEvent` with `addListener`).

**Action:** Open `index.html` in a browser with the stub substituted for `js/juce/index.js` (temp import-map or path swap). Verify: page renders 940×440; all 4 panels + 9 visible controls present per D9; syncMode segments read FREE/SYNC (HTML labels intact after JS binds); no console errors (a single ReferenceError = whole-UI death — Pitfall A). This browser render IS the mockup review AND the TDZ/label-overwrite regression check.

**Depends on:** Task 1.
**Gate:** clean console + correct visual layout before any C++ work builds on it.

### Task 3: CMake wiring

**Files (modify):** `plugins/O-ReverseDelay/CMakeLists.txt`

**Action (RESEARCH checklist 1–3):**
1. `juce_add_plugin(OuariconReverseDelay ...)`: add `NEEDS_WEB_BROWSER TRUE` and `NEEDS_WEBVIEW2 TRUE`.
2. Compile definitions: add `JUCE_WEB_BROWSER=1` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (BOTH, now — Windows-readiness; Pitfall E).
3. After `juce_generate_juce_header`:
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
   Link PRIVATE. `NAMESPACE` + `HEADER_NAME` BOTH distinct (Pitfall D).
4. Add `Source/PluginEditor.cpp` to the **plugin** `target_sources` only — never the harness.

**Depends on:** Task 1 (file list).

### Task 4: WebView editor (grain clone minus extras) + all relays/attachments

**Files (create):** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`

**Spec:**
- Member order (destroy-in-reverse; wrong order = release-build crash on reload):
  1. **Relays** — 8× `WebSliderRelay` (delayTime, grainSize, density, feedback, lowCut, highCut, width, mix) + 2× `WebComboBoxRelay` (syncMode, noteDivision)
  2. `std::unique_ptr<juce::WebBrowserComponent>`
  3. **Attachments** — 8× `WebSliderParameterAttachment` + 2× `WebComboBoxParameterAttachment`, 3-arg form with `nullptr` undoManager, `jassert(param)` on each lookup via `processorRef.parameters.getParameter(id)` (public member — no accessor needed).
- Options chain: `withNativeIntegrationEnabled()` + `withKeepPageLoadedWhenBrowserIsHidden()` + `withResourceProvider([this](const auto& url){ return getResource(url); })` + `withOptionsFrom(*relay)` loop over all 10 relays.
- **Exactly ONE native function:** `getParameterDefaults` → returns `{id: engineeringDefault}` for all 10 params via `param->convertFrom0to1(param->getDefaultValue())`.
- Resource provider: **bare-path exact matching** — `url == "/"` → index.html, `"/css/styles.css"`, `"/js/app.js"`, `"/js/juce/index.js"`, `"/js/juce/check_native_interop.js"`, `"/img/<botanical>.png"`; `charset=utf-8` on text types; `std::nullopt` otherwise. **Never strip a scheme; never hard-code `juce://` vs `https://juce.backend`** (Pitfall C). Serve from `UIBinaryData::`.
- `#if JUCE_WINDOWS`: `withWinWebView2Options(...withUserDataFolder(tempDir/"OReverseDelay_WebView").withStatusBarDisabled().withBuiltInErrorPageDisabled())`.
- `goToURL(WebBrowserComponent::getResourceProviderRoot())`; `setSize(940, 440)`; WebView bounds = full local bounds in `resized()`.
- No Timer, no viz analyzer, no drag-drop, no FileChooser, no `withInitialisationData` (D10).

**Depends on:** Task 3.

### Task 5: `createEditor()` harness guard

**Files (modify):** `Source/PluginProcessor.cpp` (nothing else — harness CMake already excludes editor sources and defines `JUCE_WEB_BROWSER=0`)

**Action:**
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
with `#include "PluginEditor.h"` inside the same guard. Do NOT add editor includes to PluginProcessor.h (its header comment mandates this).

**Depends on:** Task 4.

### Task 6: Phase 3.1 gate — build + harness re-run

**Action:**
1. `ninja OuariconReverseDelay_VST3 OuariconReverseDelay_AU OuariconReverseDelay_Standalone` from `build/` — clean compile.
2. Rebuild + run the render harness (`-DOUARICON_BUILD_TESTS=ON`, target `O-ReverseDelay-render-test`) → **exit 0, all Stage-2 assertions still green** (proves no DSP regression; pattern_render_harness_breaks_on_webview_editor).
3. Open Standalone: WebView renders at 940×440, all 9 visible controls per layout, no blank page.

**Depends on:** Tasks 1–5.

---

## Phase 3.2 — Full UI Interaction + Verification Gate

### Task 7: Binding interaction in app.js

**Files (modify):** `Source/ui/public/js/app.js` (+ minor css/html touch-ups)

**Spec:**
- `bindKnob(Juce, id, el, formatFn)` per grain's pattern: relative vertical drag writes `setNormalisedValue`; knob angle from normalised value; **readout text from `st.getScaledValue()` only** — JS carries format-only functions (unit suffix/decimals: ms, %, Hz→kHz≥1000, delayTime→s≥1000 optional). No range constants anywhere in JS (Pitfall B).
- `bindCombo(Juce, id, sel)`: build `<option>`s from `st.properties.choices` inside a refresh subscribed to BOTH `propertiesChangedEvent` and `valueChangedEvent`; sync via `getChoiceIndex()`; write via `setChoiceIndex(sel.selectedIndex)`.
- syncMode segment pair: driven by the same ComboBox state; `refresh()` toggles classes + `aria-pressed` ONLY — **never write `textContent` onto the HTML-authored FREE/SYNC labels** (Pitfall F).
- **UI-02 swap (pure JS, zero native fns):**
  ```js
  const sync = Juce.getComboBoxState("syncMode");
  function refreshTimeSlot() {
    const isSync = sync.getChoiceIndex() === 1;   // {Free, Sync}, default 1 = Sync
    divisionWrap.classList.toggle("hidden", !isSync);
    delayWrap.classList.toggle("hidden",  isSync);
  }
  sync.valueChangedEvent.addListener(refreshTimeSlot);
  sync.propertiesChangedEvent.addListener(refreshTimeSlot);
  refreshTimeSlot();
  ```
  Both controls stay relay-bound at all times — no rebinding, no dead controls. First open (default Sync) shows the noteDivision dropdown.
- Dblclick-reset: fetch defaults ONCE via `Juce.getNativeFunction("getParameterDefaults")`; dblclick sets `setNormalisedValue(convert)` from the engineering default — never a hardcoded JS default table.

**Depends on:** Task 6.

### Task 8: Browser-stub re-check + bridge grep-diff

**Action:**
1. Re-run the Task 2 browser-stub render with interaction: drag a knob (readout updates via stub `getScaledValue`), flip FREE↔SYNC (slot swaps, no layout shift, labels intact), open the division dropdown (13 options from stub choices).
2. grep-diff `getNativeFunction` (JS) vs `withNativeFunction` (PluginEditor.cpp) → expected surface **exactly 1 ≡ 1** (`getParameterDefaults`) (Pitfall G).

**Depends on:** Task 7.

### Task 9: Port and run `ui_frontend_check.js`

**Files (create):** `tests/ui_frontend_check.js` — ported from `plugins/O-Contrabass/tests/ui_frontend_check.js`, adapting paths/regexes (external `js/app.js`; no preset/tuning modules; single native fn).

**Action:** `node plugins/O-ReverseDelay/tests/ui_frontend_check.js` → **exit 0**. It statically pins Pitfalls A/B/G + resource-provider closure (every served path exists in the BinaryData source list and vice versa).

**Depends on:** Task 7.

### Task 10: Stage-3 exit gate — build, install, validate

**Action (in order):**
1. Harness re-run → exit 0 (final DSP-regression proof for the stage).
2. `./scripts/build-and-install.sh O-ReverseDelay` (resolves the OuariconReverseDelay target, AU cache clear + dual-variant sweep).
3. `auval -a | grep -i reversedelay` → plugin listed; full auval on `aufx ORvD OuDv` (confirm subtype from CMakeLists) passes.
4. `pluginval --strictness-level 10` on **both** installed formats (VST3 + AU, editor tests included) → green. (Stage 4 still owns the ×3 repeat gate.)
5. Standalone: editor opens; two-way check — drag each control → DSP/param changes; automate/change a param host-side (Standalone settings or a DAW) → UI follows; **skew spot-check: delayTime knob at 12 o'clock reads ≈ 316 ms, not 1025 ms**; FREE↔SYNC swap leaves no dead controls in either mode.
6. Commit per phase convention; update STATUS.md.

**Depends on:** Tasks 8–9.

---

## Success Criteria (verifiable)

- [ ] **Build:** `ninja OuariconReverseDelay_VST3 OuariconReverseDelay_AU OuariconReverseDelay_Standalone` compiles clean.
- [ ] **Browser-stub render** (mockup-gate replacement): index.html against the ~20-line stub renders the full 940×440 four-panel layout with zero console errors; FREE/SYNC labels survive JS binding.
- [ ] **Harness:** `O-ReverseDelay-render-test` rebuilds and exits 0 after the WebView editor lands (run at Task 6 AND Task 10).
- [ ] **ui_frontend_check.js:** `node plugins/O-ReverseDelay/tests/ui_frontend_check.js` exits 0.
- [ ] **Native-fn surface:** grep-diff `getNativeFunction` vs `withNativeFunction` = exactly 1 ≡ 1 (`getParameterDefaults`).
- [ ] **UI-01:** all 10 controls drive DSP; host-side param changes update the UI; preset/state restore updates all controls.
- [ ] **UI-02:** syncMode swap shows noteDivision (Sync) / delayTime (Free) with zero dead controls and zero layout shift; default first-open state = Sync → dropdown visible.
- [ ] **Skew correctness:** delayTime midpoint readout ≈ 316 ms (getScaledValue path, not a JS map); grainSize/lowCut/highCut readouts match C++ ranges.
- [ ] **Install:** `./scripts/build-and-install.sh O-ReverseDelay` completes (AU cache cleared, dual-variant sweep).
- [ ] **auval:** plugin registered and passes.
- [ ] **pluginval strictness 10:** green on VST3 AND AU, editor tests included.
- [ ] **Standalone:** editor opens at 940×440, Naturalist aesthetic, botanical overlay behind OUTPUT.

---

## Known-Pattern Pitfalls This Plan Must Avoid

| # | Pitfall (memory pattern) | Mitigation baked into tasks |
|---|---|---|
| A | **Top-level init TDZ kills the whole UI silently** (`pattern_module_toplevel_init_tdz`) | app.js: single `init()` at file bottom after all module-level declarations (Task 1); browser-stub render before first C++ build (Task 2) and again after interaction lands (Task 8). |
| B | **JS min/max map drifts from C++ skew** (`pattern_webview_knob_readout_scaled_value`) | All readouts/angles from `getScaledValue()`/normalised value; JS = format-only fns; dblclick defaults via native fn, never hardcoded (Task 7); 316 ms spot-check (Task 10); ui_frontend_check pins it (Task 9). |
| C | **Scheme hard-coding / stripping bare provider paths** (`critical_webview_resource_provider_and_schemes`) | Exact `==` matching on bare `"/..."` paths, grain's `getResource` verbatim; `goToURL(getResourceProviderRoot())`; no `juce://` or `https://juce.backend` literals anywhere (Task 4). |
| D | **Dual BinaryData namespace collision** (`critical_dual_binary_data_namespace_collision`) | `NAMESPACE UIBinaryData` + `HEADER_NAME UIBinaryData.h` — both distinct — even though no second target exists yet (Stage 4 presets would collide) (Task 3). |
| E | **Windows WebView2 blank page** (`critical_webview2_static_linking`, `critical_webview2_runtime_gotchas_windows`) | `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` now (Task 3); `withUserDataFolder` + status-bar/error-page disabled under `#if JUCE_WINDOWS` (Task 4). |
| F | **Shared JS updater overwrites HTML-authored labels** (`pattern_js_state_updater_overwrites_html_labels`) | FREE/SYNC text authored in HTML; refresh toggles classes/`aria-pressed` only; verified by RENDERING the page (Tasks 2, 8), not by reading the HTML. |
| G | **Unregistered native fn fails silently** (`pattern_webview_native_fn_bridge_gap`) | Single-fn surface makes the grep-diff trivial (Task 8); ui_frontend_check pins it permanently (Task 9). |
| H | **Render harness breaks on WebView editor** (`pattern_render_harness_breaks_on_webview_editor`) | Harness already excludes editor sources + defines `JUCE_WEB_BROWSER=0`; only edit is the createEditor guard (Task 5); harness re-run is a hard gate twice (Tasks 6, 10) and again at Stage 4 entry. |
| I | **`window.__JUCE__` instead of the `Juce` ES-module namespace** (`critical_juce_webview_namespace_vs_postmessage`) | `import * as Juce`; helpers receive the namespace as an argument; `window.__JUCE__` appears only inside JUCE's own `check_native_interop.js` (Tasks 1, 7). |
| J | **Target name ≠ folder name** (`build_script_target_name_vs_folder`) | All build commands use **OuariconReverseDelay**; install via build-and-install.sh which resolves the target (Tasks 6, 10). |

---

## Out of Scope (Stage 4)

- Standalone smear/wash/width **audition** (D7 — Stage 4 **entry gate**, incl. the ~−7.3 dB/generation wash-decay finding and possible feedback-tap makeup constant).
- Factory presets / OuariconPresetManager (author in engineering units + `convertTo0to1` — 4 skewed params).
- pluginval strictness-10 **×3** repeat runs, CHANGELOG, packaging.
