---
plugin: O-simpleGrain
stage: 3-gui
type: execute
mode: manual
created: 2026-06-25
source: stages/3-gui/RESEARCH.md (primary) + CONTEXT.md (4 locked decisions) + parameter-spec.md (18 params) + ROADMAP.md § Stage 3 Phases
phases: [3.1, 3.2, 3.3]
depends_on: [stage-2-dsp]   # Stage-2 viz taps (getVizRing/getGrainCloudBuffer/getActiveGrainCount) + drop native fns already shipped
requirements: [UI-01, UI-02, UI-03, UI-04, UI-05, UI-06, FUNC-03, FUNC-05, FUNC-06, FUNC-07, COMPAT-02, QUAL-01]
build_verify: |
  ninja O-simpleGrain_VST3 O-simpleGrain_AU
  # then CLAUDE.md cache-clear + dual-variant sweep + install (use ./scripts/build-and-install.sh O-simpleGrain)
  auval -a | grep -i simplegrain     # AU must be SUCCEEDED

must_haves:
  truths:
    - "WebView renders the Naturalist field-guide UI in macOS VST3 + AU (no blank page)"
    - "All 18 APVTS params (15 sliders + 2 combos + 1 toggle) are two-way bound: UI drag → DSP, host automation → UI"
    - "Drag-drop OR the Load… picker loads a user source and the plugin granulates it"
    - "The four live visualizations (cloud, source-waveform, scope, spectrum) animate at 30 Hz + window inset + grain/overlap/CPU readout"
    - "Every control has a plain-language hover tooltip and the 8 concept presets load and isolate one concept each"
  artifacts:
    - path: "Source/ui/public/index.html"
      provides: "Production field-guide layout — 2×2 viz grid + side control rail + preset bar + drop zone"
    - path: "Source/ui/public/css/styles.css"
      provides: "Naturalist palette (FM base) + viz-grid/inset/readout/drop-zone/side-rail/.combo rules"
    - path: "Source/ui/public/js/app.js"
      provides: "18-param bind + 4 canvas renderers + drop bind + tooltip map + preset tour"
    - path: "Source/PluginEditor.cpp"
      provides: "relays→WebView→attachments, resource provider, 7 native fns, 30 Hz timer pushing 4 viz events"
    - path: "Source/PluginProcessor.cpp"
      provides: "applyFactoryPreset (8 snapshots) + getSourceThumbnail native-fn bodies"
  key_links:
    - from: "Source/PluginEditor.cpp (timerCallback)"
      to: "Source/ui/public/js/app.js (setupVizEvents)"
      via: "emitEventIfBrowserIsVisible → backend.addEventListener event names"
      pattern: "scopeUpdate|spectrumUpdate|grainCloudUpdate|grainMeterUpdate|windowInsetUpdate"
    - from: "Source/ui/public/js/app.js (bindSourceDrop)"
      to: "PluginProcessor dropSession* / loadSourceFromFileChooser"
      via: "Juce.getNativeFunction (NOT window.__JUCE__)"
      pattern: "dropSessionStart|dropSessionAddFile|dropSessionCommitFile|loadSourceFromFileChooser"
    - from: "Source/PluginEditor.cpp (relays + attachments)"
      to: "OSimpleGrain::ParamIDs (18)"
      via: "WebSliderRelay×15 + WebComboBoxRelay×2 + WebToggleButtonRelay×1, 3-arg attach + nullptr undoManager"
      pattern: "WebSliderRelay|WebComboBoxRelay|WebToggleButtonRelay"
---

<objective>
Build the production Ouaricon-Naturalist WebView editor for O-simpleGrain — the "Granular
Synthesizer · A Field Guide" — in three sequential phases (3.1 → 3.2 → 3.3). The strategy is
**reuse**: ~90% copy-and-adapt from two shipped siblings (O-simpleFM, O-simpleAdditive) + the
**already-built** Stage-2 viz taps and drop native fns. The only genuinely-new code is four
canvas renderers, two new C++ native fns (`applyFactoryPreset`, `getSourceThumbnail`), and a
thin single-source drop bind.

Purpose: deliver the GUI half of the plugin — projector-readable, 18 params bound, four live
teaching visualizations, load-your-own source, and the 8 concept-preset tour.
Output: production `index.html` + `css/` + `js/`, a full PluginEditor rewrite, two new
processor native fns, the CMake UI-resources block — building clean VST3+AU with auval SUCCEEDED.

> **Stage goal (goal-backward target):** a teacher opens O-simpleGrain in Logic/standalone,
> sees the field guide, plays MIDI, drops their own sound, runs the 8-preset tour, and watches
> the cloud/waveform/scope/spectrum explain granular synthesis in five minutes — all without
> touching a manual.
</objective>

<execution_context>
**Build/install/verify (run at the end of EACH phase — same for all three):**
```
ninja O-simpleGrain_VST3 O-simpleGrain_AU
./scripts/build-and-install.sh O-simpleGrain      # Phase 4 does the cache-clear + dual-variant sweep automatically
auval -a | grep -i simplegrain                    # AU registered + SUCCEEDED
```
If running the steps manually instead of the script, do the full CLAUDE.md sequence:
`killall -9 AudioComponentRegistrar`; `rm -rf ~/Library/Caches/AudioUnitCache/` +
`com.apple.audiounits.cache`; remove BOTH `O-simpleGrain.{vst3,component}` AND
`O-simpleGrain-dev.{vst3,component}` from the system folders (dev↔release variant shadowing —
MEMORY.md) before copying the fresh build in. Visual review is in `/show-standalone` or a DAW
(D2 — first visual review is the live build at the end of 3.1).
</execution_context>

<context>
@plugins/O-simpleGrain/.planning/stages/3-gui/RESEARCH.md      # PRIMARY — reuse map, C++/web blueprints, viz contracts, 12 invariants
@plugins/O-simpleGrain/.planning/stages/3-gui/CONTEXT.md       # 4 locked decisions + locked contract
@plugins/O-simpleGrain/.planning/parameter-spec.md             # the 18 params (15 sliders + 2 combos + 1 toggle)
@plugins/O-simpleGrain/.planning/ROADMAP.md                    # § Stage 3 Phases — authoritative 3.1/3.2/3.3 + test criteria

# Sibling sources to copy/adapt (cite file:line per task):
@plugins/O-simpleFM/Source/PluginEditor.cpp                    # resource provider, native-fn chain, Windows opts, scope/spectrum timer
@plugins/O-simpleAdditive/Source/PluginEditor.cpp              # WebComboBoxRelay wiring + timer push skeleton
@plugins/O-simpleAdditive/Source/PluginProcessor.cpp           # applyFactoryPreset snapshot pattern (:359)
@plugins/O-simpleFM/Source/ui/public/js/app.js                 # bindKnob/bindToggle, makeCanvas, drawScope/drawSpectrum, setupVizEvents, fetchSampleRate
@plugins/O-simpleAdditive/Source/ui/public/js/app.js           # bindCombo (:251), tour buttons (:399-426)
@plugins/O-simpleAdditive/Source/ui/public/index.html          # combo + tour markup base
@plugins/O-simpleFM/Source/ui/public/css/styles.css            # Naturalist CSS base (D1)
@modules/core/webview-drop-streaming/js/webview-drop-streaming.js  # readFileEntryAsBase64/arrayBufferToBase64 exports
@plugins/O-simpleGrain/Source/PluginProcessor.h                # tap + native-fn surface (already shipped)
</context>

---

## MUST-HOLD invariants (DO NOT re-derive — RESEARCH § "MUST-HOLD Invariants" 1–12 is authoritative)

Read RESEARCH lines 339–352 before touching any task. Every task below is constrained by them;
the load-bearing ones per phase are flagged inline. The 12, in one line each:

1. Member order **relays → WebView → attachments** (wrong = reload crash).
2. **3-arg attach + `nullptr` undoManager**, `jassert(param != nullptr)` per ID.
3. Resource provider compares **BARE PATHS** by direct equality; `charset=utf-8` on text.
4. **`Juce` namespace vs `window.__JUCE__`** (RECURRING) — param state + native fns via `Juce`; viz events via `window.__JUCE__.backend.addEventListener`.
5. Canvas DPR backing store + `calc()` sizing — never `right/bottom`.
6. base64 decode = `juce::Base64::convertFromBase64` (C++ side already correct).
7. **No audio-thread FFT/alloc** — FFT on the message-thread Timer only.
8. **Scope copied before FFT** (in-place transform clobbers).
9. Windows WebView2 checklist — `NEEDS_WEBVIEW2` + static-link def + `withUserDataFolder` (all 3 already in CMake; add the editor `#if JUCE_WINDOWS` block).
10. **Drop native-fn names are FIXED** — `dropSessionStart/AddFile/CommitFile/CommitFolder`, `loadSourceFromFileChooser`. Never rename.
11. Unique WebView temp-dir prefix `OsimpleGrain_WebView`.
12. **Viz event names are the contract** — `scopeUpdate`/`spectrumUpdate`/`grainCloudUpdate`/`grainMeterUpdate`/`windowInsetUpdate` must match exactly C++↔JS.

**Verified facts (resolved during planning — do not re-investigate):**
- Drop handlers are **hand-rolled in `PluginProcessor.cpp`** (decode at :494 via `convertFromBase64`), NOT pulled from the module. CMake does NOT need the C++ side of the drop module; `ouaricon_add_module(O-simpleGrain webview-drop-streaming)` is used **only to copy the JS** into `ui/public/modules/` and serve it (sampler pattern, CMakeLists :68).
- `applyFactoryPreset` and `getSourceThumbnail` **do not exist yet** → authored fresh in 3.3 / 3.2 respectively.
- WebView CMake flags (`IS_SYNTH`/`NEEDS_MIDI_INPUT`/`NEEDS_WEB_BROWSER`/`NEEDS_WEBVIEW2` + the 3 defs) are **already present** from Foundation (CMakeLists :18-23, :103-105). Only the `juce_add_binary_data(O-simpleGrain_UIResources …)` block + module-add are new.
- Existing `juce_add_binary_data` target is `O-simpleGrain_Samples` (the 4 source .wavs) — do NOT confuse with the new `_UIResources` target.
- Editor is a 45-line Stage-1 placeholder → **full rewrite** (no salvage).
- **Param split is 15 sliders + 2 combos + 1 toggle** (RESEARCH count-correction, lines 118-123): `freeze`=toggle, `sourceSample`/`windowShape`=combos. Use 15, NOT 16, in the slider array.

---

# Phase 3.1 — Layout + controls + cross-platform wiring + load-your-own UI

**Goal (ROADMAP 3.1):** single-page projector-readable WebView with all 18 params two-way bound
and cross-platform wiring correct; drag-drop + picker load a source. First visual review is the
live build (D2). Layout = balanced 2×2 viz-grid **placeholders** + side control rail (D3) — the
grid cells are empty canvases this phase; 3.2 fills them.

**Dependencies:** none beyond shipped Stage 2. **Must build clean + auval SUCCEEDED before 3.2.**
**Load-bearing invariants:** 1, 2, 3, 4, 9, 10, 11. **Watch:** the 15/2/1 param split (Pitfall 1).

### Task 3.1-1 — Copy the JUCE ES module + check-interop + img verbatim; scaffold the UI tree
- **Files (create):** `Source/ui/public/js/juce/index.js`, `Source/ui/public/js/juce/check_native_interop.js`, `Source/ui/public/img/insects.png`
- **Action:** Copy all three **verbatim** from `O-simpleFM/Source/ui/public/js/juce/{index.js,check_native_interop.js}` and `.../img/insects.png` (RESEARCH Reuse map lines 65-67 — "copy verbatim"). These are the JUCE-generated ES module (`getSliderState`/`getComboBoxState`/`getToggleState`/`getNativeFunction`) + the interop check + the botanical overlay. No edits. Create the `Source/ui/public/{css,js/juce,img}` directory tree.
- **Verify:** `ls Source/ui/public/js/juce/index.js Source/ui/public/js/juce/check_native_interop.js Source/ui/public/img/insects.png` all exist; `index.js` exports `getSliderState`.
- **Done:** the three verbatim assets are in place; tree scaffolded. → one commit.

### Task 3.1-2 — Adapt CSS (Naturalist base) for the 2×2 grid + side rail + combos + drop zone
- **Files (create):** `Source/ui/public/css/styles.css`
- **Action:** Copy `O-simpleFM/Source/ui/public/css/styles.css` **verbatim** as the base (D1 names FM as the CSS base — CONTEXT line 21; RESEARCH line 64). Keep knob/toggle/header/preset-bar/keyboard components untouched. **Add** these rules in the same sepia/brown palette: `.viz-grid` (CSS grid, 2×2 equal-weight), `.canvas-wrap` (each cell), `.window-inset` (tucked into the cloud or waveform corner — Claude's discretion), `.grain-readout`, `.source-drop-zone`, `.side-rail` (6 groups), and a `<select>.combo` rule **lifted from `O-simpleAdditive`'s CSS** (for the two combos). Canvas sizing MUST use `width: calc(100% - Npx); height: calc(100% - Npx)` — **never** `right/bottom` (invariant 5, replaced-element gotcha). Subtitle styling: "Granular Synthesizer · A Field Guide".
- **Verify:** `grep -E "viz-grid|side-rail|source-drop-zone|window-inset|grain-readout|select.combo|\.combo" Source/ui/public/css/styles.css` returns all rules; `grep -c "right:\|bottom:" ` shows no canvas uses right/bottom for stretch.
- **Done:** CSS renders the field-guide palette with the new granular rules. → one commit.

### Task 3.1-3 — Author production `index.html` (2×2 viz grid placeholders + side rail + preset bar + drop zone)
- **Files (create):** `Source/ui/public/index.html`
- **Action:** Adapt from `O-simpleAdditive/Source/ui/public/index.html` (it already has the two `<select class="combo">` + `applyFactoryPreset` tour-button markup — RESEARCH line 68). Title → "O–simpleGrain", subtitle → "Granular Synthesizer · A Field Guide". Replace the drawbar bay with the **side control rail** (6 groups, D3): **Source** (`<select class="combo" id="combo-sourceSample">` + `Load…` button + `#source-drop-zone`) · **Grain** (`grainSize`, `density`, `position`, `scan`, `freeze` toggle) · **Window** (`<select class="combo" id="combo-windowShape">`) · **Spray & Scatter** (`pitchSpray`, `positionSpray`, `scatter`, `grainPitch`, `panSpray`, `velToDensity`) · **Amp** (`ampAttack`/`Decay`/`Sustain`/`Release`) · **Output** (`outputLevel`). Replace the 2-up viz panel with the **2×2 `.viz-grid`** (4 empty `.canvas-wrap` cells: `cloudCanvas`, `sourceWaveCanvas`, `scopeCanvas`, `spectrumCanvas`) + a `.window-inset` canvas + a `.grain-readout` div — **placeholders only this phase**. Header preset bar: 8 `.tour-btn data-preset` buttons renamed to the concept presets (Single Grain · Pitched Buzz · Fragments · Smooth Cloud · Frozen Pad · Asynchronous Cloud · Granular Fire · Rect Click — D4). Keep the on-screen keyboard for sibling parity (RESEARCH Discretion / A3). Every knob gets `data-tip="…"` attrs (copy-filled in 3.3). All `<script>` are `type="module"`.
- **Verify:** `grep -Eo "data-param=\"[a-zA-Z]+\"" index.html | sort -u | wc -l` accounts for all 15 sliders; `grep -c "combo-sourceSample\|combo-windowShape" ` = 2; `grep -c "tour-btn" ` = 8; `grep -c 'type="module"' ` ≥ 1.
- **Done:** the field-guide page structure exists with all 18 control elements + 8 preset buttons + drop zone + empty viz grid. → one commit.

### Task 3.1-4 — Rewrite PluginEditor.h: relays → WebView → attachments (15/2/1 split) + analyzer + fileChooser
- **Files (modify):** `Source/PluginEditor.h`
- **Action:** Replace the placeholder editor declaration entirely (RESEARCH C++ Blueprint lines 84-105). Class is `public juce::AudioProcessorEditor, private juce::Timer`. Member order **MUST be exactly** (invariant 1): (1) `std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;` (15), `std::vector<…WebComboBoxRelay>> comboRelays;` (2), `std::vector<…WebToggleButtonRelay>> toggleRelays;` (1); (2) `std::unique_ptr<juce::WebBrowserComponent> webView;`; (3) `std::vector<…WebSliderParameterAttachment>> sliderAttachments;`, `…WebComboBoxParameterAttachment>> comboAttachments;`, `…WebToggleButtonParameterAttachment>> toggleAttachments;`. Plus `GrainVizAnalyzer vizAnalyzer;` (already in-tree, `VizAnalyzer.h`) and `std::unique_ptr<juce::FileChooser> fileChooser;` (held across the async picker). Declare `void timerCallback() override;` and a resource-provider helper. Base on `O-simpleAdditive/Source/PluginEditor.h` (has both slider+combo vectors) + add the toggle vectors from `O-simpleFM`.
- **Verify:** `grep -n "WebSliderRelay\|WebComboBoxRelay\|WebToggleButtonRelay\|GrainVizAnalyzer\|FileChooser\|Timer" Source/PluginEditor.h` shows all three relay vectors declared **before** `webView`, all three attachment vectors **after**.
- **Done:** header compiles against the new member set; order is relays→WebView→attachments. → one commit (paired with 3.1-5 build).

### Task 3.1-5 — Rewrite PluginEditor.cpp: relays, resource provider, 18 attachments, Windows opts (bind all 18 params)
- **Files (modify):** `Source/PluginEditor.cpp`
- **Action:** Full rewrite from the placeholder (RESEARCH Construction sequence lines 125-133; mirror `O-simpleAdditive:63-156` + `O-simpleFM:64-229`). Build three ID arrays using `OSimpleGrain::ParamIDs` (RESEARCH lines 110-123): `sliderIds` = the **15** floats (`grainSize, density, position, scan, pitchSpray, positionSpray, scatter, grainPitch, panSpray, velToDensity, ampAttack, ampDecay, ampSustain, ampRelease, outputLevel`), `comboIds` = `{ sourceSample, windowShape }`, `toggleIds` = `{ freeze }`. (1) make all relays **before** the WebView. (2) `Options{}.withNativeIntegrationEnabled().withKeepPageLoadedWhenBrowserIsHidden().withResourceProvider(...)` then `.withOptionsFrom(*relay)` for every relay. (3) **Resource provider — BARE PATHS, direct equality** (invariant 3; RESEARCH lines 135-149): map `"/"`/`"/index.html"`→`index_html`, `"/css/styles.css"`, `"/js/app.js"`, `"/js/juce/index.js"`, `"/js/juce/check_native_interop.js"`, `"/js/modules/webview-drop-streaming.js"`, `"/img/insects.png"` — `charset=utf-8` on every text resource; never strip scheme/host. (4) `#if JUCE_WINDOWS` → `withWinWebView2Options(...withUserDataFolder(tempDir.getChildFile("OsimpleGrain_WebView")).withStatusBarDisabled().withBuiltInErrorPageDisabled())` (invariants 9, 11). (5) construct `webView`. (6) build attachments **after** the WebView, one per ID, **3-arg ctor + `nullptr` undoManager**, `jassert(param != nullptr)` each (invariant 2). (7) `addAndMakeVisible(*webView); webView->goToURL(getResourceProviderRoot());`. (8) `setSize(900, 760); startTimerHz(30);` (RESEARCH line 133 — final size is Claude's discretion at first live review). Empty `timerCallback()` stub this phase (3.2 fills it). **Native-fn chain is added in 3.1-6/3.1-7** — leave a clearly-marked insertion point in the options chain.
- **Verify:** `ninja O-simpleGrain_VST3 O-simpleGrain_AU` compiles clean; `grep -c "jassert" Source/PluginEditor.cpp` ≥ 18 (one per attachment); `grep -c "convertTo0to1\|fromFirstOccurrenceOf" ` shows no scheme-strip on the resource path.
- **Done:** WebView constructs, page loads, all 18 relays+attachments wired (drag a knob → DSP responds; host automation → UI reflects). → one commit (with 3.1-4).

### Task 3.1-6 — Register the 5 drop/picker native fns in the editor options chain
- **Files (modify):** `Source/PluginEditor.cpp`
- **Action:** In the options chain (insertion point from 3.1-5), `.withNativeFunction(...)` for each of the **5 FIXED-name** fns (invariant 10; RESEARCH lines 151-152): `dropSessionStart`, `dropSessionAddFile`, `dropSessionCommitFile`, `dropSessionCommitFolder`, `loadSourceFromFileChooser`. Each body forwards args to the matching `processorRef.dropSession*` / `processorRef.loadSourceFromFileChooser()` method (signatures verified `PluginProcessor.h:141-150` — note `dropSessionCommitFile(sessionId, filename, base64)` takes base64 directly, NO midi/vel) and `complete(juce::var(ok))` with the returned bool. Pattern shape = `O-simpleFM:100-178` native-fn chain. Also register `getSampleRate` returning `processorRef.getCurrentSampleRate()` (for the freq axis, used in 3.2). **Do NOT rename any drop fn** (C++ side + module both depend on them).
- **Verify:** `grep -c "withNativeFunction" Source/PluginEditor.cpp` ≥ 6 (5 drop/picker + getSampleRate); `ninja O-simpleGrain_VST3` compiles.
- **Done:** all 5 drop/picker fns + getSampleRate callable from JS. → one commit.

### Task 3.1-7 — Author `app.js` core: 18-param bind + single-source drag-drop bind + Load… picker + CMake UI-resources block
- **Files (create):** `Source/ui/public/js/app.js`; **(modify):** `CMakeLists.txt`
- **Action:** Build `app.js` on the **Additive structure** (has `bindCombo`) + **FM** (`bindKnob`/`bindToggle`) — RESEARCH lines 69, 266-269. (a) **Param inventory** = the 18 grain IDs split into KNOB_IDS (15), COMBO_IDS (`sourceSample`,`windowShape`), TOGGLE_IDS (`freeze`). `import * as Juce from "./juce/index.js"`. Bind knobs via FM `bindKnob:103-162` (`Juce.getSliderState`, relative ±135° drag + wheel + arrow keys — invariant 4/critical-pattern #16); toggle via FM `bindToggle:165-180` (`Juce.getToggleState`); combos via Additive `bindCombo:251-280` (`Juce.getComboBoxState`, build `<option>`s from `st.properties.choices`). (b) **`bindSourceDrop()`** (~40 lines, RESEARCH lines 271-276, Pitfall 3): document-level `drop` over `#source-drop-zone`; on a single audio-file drop, `webkitGetAsEntry()`, then call the **grain** native fns directly — `dropSessionStart(id)` → `dropSessionAddFile(id, name, base64)` → `dropSessionCommitFile(id, name, base64)` — reusing only `readFileEntryAsBase64`/`arrayBufferToBase64` imported from `./modules/webview-drop-streaming.js`. **`opts.juce` / all `getNativeFunction` calls use the `Juce` namespace** (invariant 4 — `window.__JUCE__` lacks `getNativeFunction`). `await` each fn; `showToast(...)` on `false`/throw (toast-on-false contract). (c) **`Load…` button** → `Juce.getNativeFunction("loadSourceFromFileChooser")()`. (d) **Truncation notice** (Open Q4 — default): after a load, poll a tiny status read and surface `wasLastLoadTruncated()` as a "truncated to 10 s" notice (fold into the load wiring; minor). (e) **CMake:** add `ouaricon_add_module(O-simpleGrain webview-drop-streaming)` (copies the JS into `ui/public/modules/`, sampler pattern CMakeLists:68) **before** a new `juce_add_binary_data(O-simpleGrain_UIResources NAMESPACE BinaryData HEADER_NAME UIBinaryData.h SOURCES …)` listing the 8 UI files (index.html, css/styles.css, js/app.js, js/juce/index.js, js/juce/check_native_interop.js, modules/webview-drop-streaming.js, img/insects.png) — distinct from the existing `O-simpleGrain_Samples` target; then `target_link_libraries(O-simpleGrain PRIVATE O-simpleGrain_UIResources)`. Use a **separate HEADER_NAME** (e.g. `UIBinaryData.h`/namespace) or merge into one binary-data target so the two `juce_add_binary_data` calls don't collide on `BinaryData.h`.
- **Verify:** `ninja O-simpleGrain_VST3 O-simpleGrain_AU` builds; in the live build all 15 knobs + 2 combos + freeze are two-way bound (host automation moves the UI); dropping a `.wav` on the zone OR clicking `Load…` loads + granulates a source. `grep -c "ouaricon_add_module\|O-simpleGrain_UIResources" CMakeLists.txt` ≥ 2.
- **Done:** UI fully interactive — 18 params bound, drag-drop + picker working. → one commit.

### Task 3.1-8 — CHECKPOINT (human-verify): live-build visual review (D2 first review)
- **Type:** `checkpoint:human-verify`
- **What built:** the full 3.1 WebView — Naturalist field guide, 2×2 viz-grid placeholders, side control rail, preset bar, drop zone.
- **How to verify:** `./scripts/build-and-install.sh O-simpleGrain`, then open `/show-standalone` (or Logic). Confirm: (1) the page renders (NOT blank — invariant 9); (2) every knob drags and the value moves DSP; automate a param in the host and watch the UI reflect; (3) the two combos populate + switch; (4) `freeze` toggles; (5) drag a `.wav` onto the drop zone AND use `Load…` — both granulate; (6) layout is projector-readable (UI-06). Tune `setSize(900, 760)` if the grid+rail need more room (D2 — your discretion).
- **Resume signal:** type "approved" or list layout/binding issues to fix.

### Task 3.1-9 — Build + install + auval (Phase 3.1 gate)
- **Files:** none (build/verify only).
- **Action:** `ninja O-simpleGrain_VST3 O-simpleGrain_AU`; CLAUDE.md cache-clear + dual-variant sweep + install (`./scripts/build-and-install.sh O-simpleGrain`); `auval -a | grep -i simplegrain`.
- **Verify:** AU appears + auval SUCCEEDED; VST3 + AU both built; macOS UI renders. (Windows VST3 verified by **config-parity** with the proven siblings — no local Windows build this stage, CONTEXT line 122; the WebView2 checklist invariant 9 is the guarantee.)
- **Done:** ✅ **Phase 3.1 gate met.** → commit `stage: O-simpleGrain Stage 3.1 (GUI layout + 18-param bind + load) complete`.

**Phase 3.1 success criteria (= ROADMAP 3.1 test criteria — acceptance gate):**
- [ ] WebView opens, single-page layout renders, classroom/projector-readable (UI-06).
- [ ] All knobs/menus/toggle two-way bound (drag → DSP; host automation → UI); relative-drag knobs (#16); `freeze` via `getToggleState` (#19).
- [ ] Drag-drop + picker load a source from the UI (FUNC-05 wiring).
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI) — Windows by config-parity (COMPAT-02).

---

# Phase 3.2 — The four live visualizations + overlap/CPU readout

**Goal (ROADMAP 3.2):** the headline teaching visuals — grain-cloud scatter, source waveform +
playheads/freeze-pin/spray-range, window-envelope inset, output scope/spectrum, and the
grain-count/overlap/CPU readout — animating at 30 Hz off the **already-built** taps. No
audio-thread FFT/alloc; scope copied before FFT.

**Dependencies:** Phase 3.1 must build clean (canvases + event plumbing exist).
**Load-bearing invariants:** 4, 5, 7, 8, 12. **Watch:** Pitfall 2 (drop FM's `drawSidebandMarkers`/`carrierUpdate`), Pitfall 4 (inset on-change only).

### Task 3.2-1 — Add `getSourceThumbnail` native fn (Open Q1 — recommended default)
- **Files (modify):** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`
- **Action:** UI-02's waveform needs the current source's data; no fn returns it (RESEARCH Open Q1, lines 436). Add `getSourceThumbnail()` to the processor returning **~512 min/max pairs** of the currently-published `currentSource` buffer (read-only snapshot off the message thread — low risk). Register it as a `withNativeFunction("getSourceThumbnail", …)` in the editor options chain, returning a flat `var` array (min,max,min,max…). Also emit/return it **on load** (after a successful drop/picker commit) and **at boot** so the waveform repaints when the source changes. (Decode/access is message-thread; the audio thread is untouched — invariant 7.)
- **Verify:** `grep -n "getSourceThumbnail" Source/PluginProcessor.h Source/PluginEditor.cpp` present; `ninja O-simpleGrain_VST3` compiles; the thumbnail fn returns ~512 pairs for the default `fire` source.
- **Done:** JS can fetch a min/max envelope of the live source. → one commit.

### Task 3.2-2 — Timer body: push scope + spectrum + cloud + grain-meter events (the C++↔JS contract)
- **Files (modify):** `Source/PluginEditor.cpp`
- **Action:** Fill `timerCallback()` (RESEARCH Timer body lines 170-192; mirror `O-simpleFM:237-259` + `O-simpleAdditive:164-193`): (1) `vizAnalyzer.process(processorRef.getVizRing(), processorRef.getCurrentSampleRate())` — FFT+scope on the **message thread** (invariant 7); scope is copied **before** the in-place FFT inside the analyzer (invariant 8 — already handled, do not reorder). (2) `pushFloatArray("spectrumUpdate", vizAnalyzer.getSpectrum())` (256 dB bins) + `pushFloatArray("scopeUpdate", vizAnalyzer.getScope())` (128 pts). (3) `const GrainCloudFrame& f = processorRef.getGrainCloudBuffer().read();` → build `makeCloudVar(f)` (a `DynamicObject` with `count`, a nested `var` array of per-grain `[readPosNorm,sizeMs,pitchSemis,pan,spawnSample]`, plus `playheadNorm`/`positionNorm`/`positionSprayNorm`/`frozen`) → `webView->emitEventIfBrowserIsVisible("grainCloudUpdate", …)` (push shape = Additive:182-191). (4) `emitEventIfBrowserIsVisible("grainMeterUpdate", juce::var(processorRef.getActiveGrainCount()))`. **Event names MUST match JS exactly** (invariant 12). `windowInsetUpdate` is NOT pushed here (Pitfall 4 — on `windowShape` change only; handled JS-side in 3.2-5).
- **Verify:** `grep -Eo '"(scopeUpdate|spectrumUpdate|grainCloudUpdate|grainMeterUpdate)"' Source/PluginEditor.cpp | sort -u | wc -l` = 4; `ninja O-simpleGrain_VST3 O-simpleGrain_AU` compiles; no FFT/alloc added to any audio-thread method (invariant 7).
- **Done:** four viz events emit at 30 Hz from the message thread. → one commit.

### Task 3.2-3 — JS: `drawCloud` + `drawSourceWaveform` renderers + DPR canvas plumbing (UI-01/02)
- **Files (modify):** `Source/ui/public/js/app.js`
- **Action:** New renderers (RESEARCH lines 296-317, viz contracts). **`makeCanvas`** DPR pattern copied from FM `app.js:414-426` (invariant 5 — `canvas.width = clientWidth*dpr; ctx.setTransform(dpr,…)`; single `window 'resize'` re-fits all canvases). **`drawCloud(f)`** (UI-01): scatter `f.count` dots — X = `readPosNorm` (or `spawnSample` for a time-cloud), Y = `pitchSemis` mapped vertically, radius ∝ `sizeMs`, lateral nudge from `pan`, **sepia fill on aged paper** (D1). **`drawSourceWaveform(f)`** (UI-02): draw the static source thumbnail (from `getSourceThumbnail`, fetched on load + boot) as the background, then a **brown vertical playhead** at `f.playheadNorm`, a **translucent shaded band** `[positionNorm ± positionSprayNorm]`, and a **freeze-pin glyph** when `f.frozen` (FUNC-03). Subscribe both in `setupVizEvents` via `be.addEventListener("grainCloudUpdate", (f) => { drawCloud(f); drawSourceWaveform(f); })` using `window.__JUCE__.backend` (invariant 4 — viz events use the low-level backend, NOT `Juce`).
- **Verify:** in the live build, playing MIDI accumulates sepia dots in the cloud (density thickens, position/pitch spray widens); the waveform shows a moving playhead + shaded spray band + freeze pin when frozen.
- **Done:** UI-01 + UI-02 live. → one commit.

### Task 3.2-4 — JS: reuse `drawScope` + `drawSpectrum` verbatim; DROP sideband markers (UI-04)
- **Files (modify):** `Source/ui/public/js/app.js`
- **Action:** Copy FM `drawScope:535-558` and `drawSpectrum:441-482` **verbatim** (same 128/256 sizes, same log-freq axis — RESEARCH lines 319-324). Wire `fetchSampleRate` (FM:688-694) via the `getSampleRate` native fn (registered in 3.1-6) for the freq-axis labels. **DROP** FM's `drawSidebandMarkers` + the `carrierUpdate` subscription (Pitfall 2 — there's no carrier in granular; copying them wholesale leaves dead overlay code). Subscribe `scopeUpdate`→`drawScope`, `spectrumUpdate`→`drawSpectrum` in `setupVizEvents` (invariant 12 — exact names).
- **Verify:** in the live build, scatter at 0% shows **discrete sidebands** on the spectrum (pitched/comb) and high scatter **smears toward noise** (UI-04, DSP-05 visual); the scope shows the post-gain waveform; no `drawSidebandMarkers`/`carrierUpdate` references remain (`grep -c "drawSidebandMarkers\|carrierUpdate" app.js` = 0).
- **Done:** UI-04 scope + spectrum live, sync↔async lesson visible. → one commit.

### Task 3.2-5 — JS: `drawWindowInset` (recompute in JS, Open Q2 default) + `drawGrainReadout` (UI-03 + UI-05)
- **Files (modify):** `Source/ui/public/js/app.js`
- **Action:** **`drawWindowInset`** (UI-03, RESEARCH Open Q2 lines 437 — **recommended default = JS recompute**): on the `windowShape` combo's `valueChanged` (+ once at boot), compute ~128 points of the selected closed-form window (rect/tri/Welch/Gauss/Hann) and draw the curve in the inset corner (Pitfall 4 — on-change only, NOT every frame; no C++ change). If exactness vs the DSP 2048-pt LUTs ever matters for the teaching point, the fallback is to expose the real LUT via a native fn (Assumption A1) — default is JS recompute. **`drawGrainReadout(n)`** (UI-05, RESEARCH lines 326-329): on `grainMeterUpdate`, render `Grains: N/192` (over `kGlobalGrainCap=192`), `Overlap: ×Y` where `Y = grainSizeSec × density` read from the `Juce` slider states (`grainSize`/1000 × `density` — display-only, no extra tap), and a **coarse CPU bar** mapped from `N/192` (CONTEXT "coarse CPU bar"; Assumption A2 — no real CPU tap). Subscribe `grainMeterUpdate`→`drawGrainReadout`; hook the inset redraw into the `windowShape` combo's change handler.
- **Verify:** in the live build, the inset matches the selected window and **redraws on change**; the readout shows live `Grains N/192`, `Overlap ×Y`, and a CPU bar that grows with density×size.
- **Done:** UI-03 + UI-05 live. → one commit.

### Task 3.2-6 — CHECKPOINT (human-verify) + build/install/auval (Phase 3.2 gate)
- **Type:** `checkpoint:human-verify`
- **What built:** all four live visualizations + window inset + grain/overlap/CPU readout.
- **How to verify:** `./scripts/build-and-install.sh O-simpleGrain`; in standalone/DAW play a held chord and sweep params. Confirm the ROADMAP 3.2 criteria below. Then `auval -a | grep -i simplegrain` SUCCEEDED.
- **Resume signal:** "approved" or list viz issues.
- **Done after approval:** ✅ **Phase 3.2 gate met.** → commit `stage: O-simpleGrain Stage 3.2 (GUI four visualizations + readout) complete`.

**Phase 3.2 success criteria (= ROADMAP 3.2 test criteria — acceptance gate):**
- [ ] Grain cloud accumulates dots (read-position × time); raising density thickens it, spray widens it (UI-01).
- [ ] Source waveform shows live playheads + freeze point + shaded spray range; position/scan/freeze visibly tracked (UI-02).
- [ ] Window-envelope inset matches the selected shape and redraws on change (UI-03).
- [ ] Output scope/spectrum shows discrete sidebands at scatter 0 and smears toward noise at high scatter (UI-04, DSP-05 visual).
- [ ] Grain-count/overlap/CPU readout updates live and ties density×size×poly to cost (UI-05).
- [ ] No audio-thread FFT/alloc; UI smooth at 30 Hz; scope not corrupted by in-place FFT (PERF-01, invariants 7/8).

---

# Phase 3.3 — Pedagogical layer

**Goal (ROADMAP 3.3):** the teaching scaffolding — per-control plain-language hover tooltips
(FUNC-07), the 8 concept presets via the additive `applyFactoryPreset` snapshot pattern
(FUNC-06, RESEARCH-recommended over the preset-manager module), and optional cloud/waveform
annotations. "Oh, THAT's how granular works" in five minutes.

**Dependencies:** Phase 3.2 must build clean (all controls + viz exist to annotate/snapshot).
**Load-bearing invariants:** 4 (`Juce` namespace for `applyFactoryPreset`), 2 (relays sync the snapshot back). **Watch:** the 8 preset value tables are **new content** the planner/executor authors.

### Task 3.3-1 — Tooltip map: plain-language hover copy on EVERY control (FUNC-07)
- **Files (modify):** `Source/ui/public/js/app.js`, `Source/ui/public/index.html`
- **Action:** Author a JS `const TOOLTIPS = { … }` map keyed by param ID + concept (RESEARCH FUNC-07; ROADMAP 3.3) covering **all 18 controls + the Load… action + the 8 presets**, with concrete class-grounded examples: what overlap-add is (grainSize/density), why rectangular clicks (windowShape), what freeze/scan do, sync↔async (scatter), why density×size×poly is the CPU cost (readout). Wire each control's `data-tip` (placed in 3.1-3) to surface its copy on hover (reuse the sibling tooltip mechanism — FM/Additive both ship one). Tone = field-guide plain language, not jargon.
- **Verify:** in the live build, hovering **every** knob/combo/toggle/preset shows a tooltip; `grep -Eo "data-tip=\"[a-zA-Z]+\"" index.html | sort -u | wc -l` covers all 18 controls + presets; no control is missing a `TOOLTIPS` entry.
- **Done:** every parameter has a working, concrete hover tooltip. → one commit.

### Task 3.3-2 — Author `applyFactoryPreset` (8 concept snapshots) in PluginProcessor (FUNC-06)
- **Files (modify):** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`
- **Action:** Add `void applyFactoryPreset(const juce::String& name)` to the processor using the **additive snapshot pattern** (RESEARCH lines 154-168; `O-simpleAdditive/PluginProcessor.cpp:359`) — NOT the preset-manager module (D4 / RESEARCH decision line 76). Reset every param to default first (`setValueNotifyingHost(getDefaultValue())`), then apply the named snapshot via `setReal`/`setChoice`/`setBool` lambdas (`convertTo0to1`). **Author the 8 value tables** (the new content — each isolates exactly one concept per FUNC-06; derive from parameter-spec.md ranges):
  - **Single Grain** — grainSize≈30, density=1 (separated grains).
  - **Pitched Buzz** — grainSize≈5, density≈200, scatter=0 (fast synchronous → pitched).
  - **Fragments** — grainSize≈80, density≈8 (recognizable source chunks).
  - **Smooth Cloud** — grainSize≈40, density≈120, windowShape=Hann (fused continuous).
  - **Frozen Pad** — `freeze`=on, density≈80, pitchSpray>0 (sustained shimmering pad).
  - **Asynchronous Cloud** — scatter≈100, density≈120 (noisy/smeared spectrum).
  - **Granular Fire** — sourceSample=fire (idx 0) + a lively grain/spray set (the worked example).
  - **Rect Click** — windowShape=rect (idx 0), density low (the intentional click teaching artifact).
  Register `withNativeFunction("applyFactoryPreset", …)` in the editor options chain (Additive:112-116) — forwards `a[0].toString()` to the processor; **the relays sync every knob/combo/toggle back automatically** (invariant 2 — no extra UI wiring).
- **Verify:** `grep -n "applyFactoryPreset" Source/PluginProcessor.h Source/PluginProcessor.cpp Source/PluginEditor.cpp` present; `ninja O-simpleGrain_VST3 O-simpleGrain_AU` compiles; all 8 names handled.
- **Done:** the 8 snapshot tables exist and the native fn is callable. → one commit.

### Task 3.3-3 — JS: wire the 8 tour buttons + captions + active state (FUNC-06)
- **Files (modify):** `Source/ui/public/js/app.js`
- **Action:** Wire the preset bar (RESEARCH lines 279-290; Additive `app.js:399-426`): `applyPresetFn = Juce.getNativeFunction("applyFactoryPreset")` (**`Juce` namespace** — invariant 4), each `.tour-btn` `onclick` → `await applyPresetFn(btn.dataset.preset)` then set the caption + `active` class (`querySelectorAll(".tour-btn").forEach(b => b.classList.toggle("active", …))`). No `preset-manager` module, no JSON, no file dialogs. After a preset applies, the relays propagate to every knob/combo/toggle and the viz reacts on the next timer tick.
- **Verify:** in the live build, clicking each of the 8 presets loads it — knobs/combos/toggle visibly snap, the cloud/scope/spectrum change, the active button highlights; each audibly/visually isolates its concept.
- **Done:** the 8-preset concept tour works end-to-end. → one commit.

### Task 3.3-4 — (Optional) cloud/waveform annotations + CHECKPOINT + build/install/auval (Phase 3.3 gate)
- **Files (modify):** `Source/ui/public/js/app.js` (annotations — optional, low risk).
- **Action:** (Optional, RESEARCH/ROADMAP 3.3) add small labels on the waveform/cloud (playhead, freeze pin, spray range) — skip if it crowds the projector view (keep single-page, UI-06). Then build + install + auval.
- **Type:** `checkpoint:human-verify` — **What built:** tooltips + 8-preset tour + optional annotations. **How to verify:** `./scripts/build-and-install.sh O-simpleGrain`; hover every control (tooltip present), run all 8 presets (each isolates its concept), confirm the page stays single-page/projector-readable; `auval -a | grep -i simplegrain` SUCCEEDED. **Resume:** "approved" or list issues.
- **Done after approval:** ✅ **Phase 3.3 gate met → Stage 3 complete.** → commit `stage: O-simpleGrain Stage 3 (GUI) complete`.

**Phase 3.3 success criteria (= ROADMAP 3.3 test criteria — acceptance gate):**
- [ ] Every parameter has a working hover tooltip with a concrete, class-grounded example (FUNC-07).
- [ ] Each preset loads and audibly/visually isolates its concept (single grain, buzz, fragments, cloud, frozen pad, async, fire, rect-click) (FUNC-06).
- [ ] Layout stays single-page and projector-readable (UI-06).

---

<verification>
## Stage-3 goal-backward verification (tasks → every ROADMAP Stage-3 criterion + all 18 params + Load…)

**Parameter-spec coverage (18 params + Load… action):**
- All 15 sliders + 2 combos + 1 toggle bound in **T3.1-4/3.1-5/3.1-7** (relays+attachments+JS bind). The 15/2/1 split is enforced (Pitfall 1).
- `Load…` action (non-APVTS) wired in **T3.1-6** (native fns) + **T3.1-7** (drag-drop bind + picker).

**ROADMAP test-criterion → task map:**
| ROADMAP criterion | Task(s) |
|---|---|
| 3.1 WebView opens / projector-readable (UI-06) | 3.1-2, 3.1-3, 3.1-8 |
| 3.1 all 18 two-way bound, relative-drag, freeze toggle | 3.1-4, 3.1-5, 3.1-7 |
| 3.1 drag-drop + picker load (FUNC-05) | 3.1-6, 3.1-7 |
| 3.1 macOS VST3+AU + Windows VST3 (COMPAT-02) | 3.1-5 (Win opts), 3.1-9 (auval), config-parity |
| 3.2 cloud accumulates / density / spray (UI-01) | 3.2-2, 3.2-3 |
| 3.2 waveform playheads + freeze + spray range (UI-02) | 3.2-1, 3.2-2, 3.2-3 |
| 3.2 window inset matches + redraws (UI-03) | 3.2-5 |
| 3.2 scope/spectrum sidebands→noise (UI-04) | 3.2-2, 3.2-4 |
| 3.2 grain/overlap/CPU readout (UI-05) | 3.2-5 |
| 3.2 no audio-thread FFT/alloc (PERF-01) | invariants 7/8, 3.2-2 |
| 3.3 every control tooltip (FUNC-07) | 3.3-1 |
| 3.3 8 presets isolate concept (FUNC-06) | 3.3-2, 3.3-3 |
| 3.3 single-page projector-readable (UI-06) | 3.3-1, 3.3-4 |

**No gaps.** Every ROADMAP Stage-3 checkbox, all 18 params, and the Load… action are covered.

## Open-question resolutions folded in (RESEARCH § Open Questions)
- **Q1 source thumbnail** → new `getSourceThumbnail` native fn (T3.2-1).
- **Q2 window inset** → JS recompute (T3.2-5, default; native-LUT fallback noted).
- **Q3 drop module linkage** → hand-rolled C++ (verified); JS pulled via `ouaricon_add_module` for the base64 primitives only (T3.1-7).
- **Q4 truncation notice** → surfaced after load via `wasLastLoadTruncated()` (T3.1-7).
</verification>

<success_criteria>
Stage 3 is complete when all three phase gates pass:
- [ ] **3.1:** WebView renders (macOS VST3+AU, no blank), all 18 params two-way bound, drag-drop+picker load a source, builds clean + auval SUCCEEDED.
- [ ] **3.2:** four live visualizations + window inset + grain/overlap/CPU readout animate at 30 Hz off the taps; no audio-thread FFT/alloc; auval SUCCEEDED.
- [ ] **3.3:** every control has a hover tooltip; the 8 concept presets load and isolate one concept each; single-page/projector-readable; auval SUCCEEDED.
- [ ] All 12 MUST-HOLD invariants held (member order, bare-path provider, `Juce` vs `window.__JUCE__`, DPR canvas, fixed drop names, exact viz-event names, no audio-thread FFT).

Out of scope (→ Stage 4, CONTEXT lines 111-114): pluginval VST3+AU sweep, preset audit, artifact/aliasing/freeze listen audit, Windows drag-drop smoke test, changelog.
</success_criteria>

<output>
Update this PLAN.md checkboxes as phases complete. At Stage 3→4 boundary, present the
two-step `/clear` handoff (CLAUDE.md handoff protocol) → next command
`/plugin-verify O-simpleGrain` (or the manual-mode next phase). Do NOT auto-invoke.
</output>
