# Stage 3 (GUI) — PLAN

**Plugin:** O-simpleSubtractive
**Stage:** 3 of 4 — GUI (WebView UI + parameter binding + headline visuals)
**Mode:** single execute pass (gui-agent checklist) — tasks are numbered + dependency-ordered, grouped under the 3 ROADMAP sub-phases.
**Date:** 2026-06-25
**Sources:** `.planning/stages/3-gui/{CONTEXT.md,RESEARCH.md}`, `.planning/ROADMAP.md`, `Source/{PluginProcessor.h,SubVizAnalyzer.h}`, `CMakeLists.txt`. Sibling refs verified this session: `O-simpleFM`, `O-simpleGrain`, `O-simpleAdditive`.

---

## Goal (from CONTEXT.md)

Turn the validated Stage-2 polyphonic synth into a single-page, projector-readable WebView teaching instrument: all **20** parameters two-way bound, with the headline **filter-curve-over-spectrum** visual plus oscilloscope, dual-ADSR display, live signal-path diagram, and per-control pedagogical tooltips. Consume the frozen Stage-2 DSP→UI contract only — **do NOT touch DSP**.

---

## Pre-flight facts (load-bearing — verify before coding)

- **20 params = 16 sliders + 4 combos** (RESEARCH §"Parameter → control-type inventory"):
  - `sliderIds` (16): `subLevel, noiseLevel, cutoff, resonance, filterEnvAmount, keyTrack, filterAttack, filterDecay, filterSustain, filterRelease, ampAttack, ampDecay, ampSustain, ampRelease, glide, outputLevel`
  - `comboIds` (4): `oscWave, filterType, filterSlope, voiceMode`
  - 0 toggles (FM had toggles; **drop the toggle relay/attachment arrays entirely**).
  - Source of truth: `OSimpleSubtractive::ParamIDs` in `PluginProcessor.h:28-61`. Any drift trips `jassert(param != nullptr)`.
- **3 native fns** (JS `getNativeFunction` ↔ C++ `withNativeFunction`): `uiMidi`, `getSampleRate`, `applyFactoryPreset`.
- **4 emitted events** (C++ `emitEventIfBrowserIsVisible` ↔ JS `window.__JUCE__.backend.addEventListener`): `filterCurveUpdate`, `spectrumUpdate`, `scopeUpdate`, `envUpdate`.
- **`applyFactoryPreset` is NOT declared on the processor** (verified: `PluginProcessor.h` declares only `handleUiMidi`, line 166; `.cpp:222`). Stage 3 adds a **wiring-only stub** (PluginProcessor `.h`/`.cpp`) — permitted, this is bridge wiring, **not a DSP change**. Model: `O-simpleGrain/Source/PluginProcessor.h:172` + `.cpp:813`. Stage 4 (FUNC-06) fills the body with the 8 concept snapshots.
- **DSP→UI contract already exposes everything** (read-only on message thread): `getVizRing()`, `getCurrentSampleRate()`, `getDisplayCutoffHz/K/Type/Slope()`, `getFilterEnvValue()`, `getAmpEnvValue()`, `handleUiMidi()`, and `SubVizAnalyzer` (`process()`, `updateCurve()`, `getSpectrum()` 256, `getScope()` 128, `getCurve()` 256). See `SubVizAnalyzer.h`.
- **Curve/spectrum axis identity (UI-01 alignment):** both `getCurve()` and `getSpectrum()` are 256 bins, both `hz = 20·pow((sr/2)/20, b/255)` (`SubVizAnalyzer.h:153-167` & `173-181`). Bin `b` is the SAME frequency in both → JS plots both at `x=(b/255)*w` with **no remapping**. Curve dB floor −120 and peaks can exceed 0 dB at resonance; spectrum dB floor −100.

---

## Phase 3.1 — Layout + Controls + Cross-Platform Wiring (UI-05/06, COMPAT-02)

> Goal: single-page left→right signal-path layout, all 20 controls two-way bound, cross-platform correct. End state: tree builds; WebView opens; every knob/combo moves the DSP and reflects host automation; on-screen keyboard plays.

### T1 — Copy the JUCE JS bridge verbatim (no edits)
- **Files (create):** `Source/ui/public/js/juce/index.js`, `Source/ui/public/js/juce/check_native_interop.js`
- **Change:** Copy **byte-for-byte** from `O-simpleFM/Source/ui/public/js/juce/{index.js,check_native_interop.js}` (17959 + 4376 bytes). Do NOT edit — version-matched to JUCE 8.0.9.
- **Depends on:** none (do first).

### T2 — `index.html` — 5-column signal-path layout + 4 combos + canvases + diagram + keyboard
- **Files (create):** `Source/ui/public/index.html`
- **Change:** Adapt `O-simpleFM/Source/ui/public/index.html` (212 lines). Keep the frame/header/`section`/`group`/`knob-cell` idiom. Reorganize control groups into the **5 Subtractive panels, left→right**: **OSC** (oscWave combo / subLevel / noiseLevel) | **FILTER** (filterType combo / filterSlope combo / cutoff / resonance / filterEnvAmount / keyTrack) | **FILTER ADSR** (filterAttack/Decay/Sustain/Release) | **AMP ADSR** (ampAttack/Decay/Sustain/Release) | **VOICE/OUT** (voiceMode combo / glide / outputLevel). Every control cell gets `data-tip="<paramId>"`.
  - Combo markup model: `O-simpleGrain/Source/ui/public/index.html:86,134` → `<select class="combo" id="combo-oscWave" aria-label="Oscillator wave"></select>` (one per combo: `combo-oscWave`, `combo-filterType`, `combo-filterSlope`, `combo-voiceMode`; options populated by JS).
  - Knob-cell model: FM `index.html:78-106`.
  - Headline canvas + scope canvas: model FM viz-panel `index.html:36-45` (`<div class="canvas-wrap"><canvas id="headlineCanvas"></canvas></div>` for the curve-over-spectrum; `<canvas id="scopeCanvas">`). Add `<canvas id="filterAdsrCanvas">` + `<canvas id="ampAdsrCanvas">` (or one `dualAdsrCanvas`) — wired in 3.2.
  - Signal-path SVG placeholder `<svg id="routingSvg">` (model FM `index.html:48-71`) — populated/skinned in 3.3.
  - Preset-tour section + on-screen keyboard placeholder (model FM `index.html:186-196` + `198-202`) — wired in 3.3 / this phase respectively.
- **Depends on:** none (parallel with T1, but combo/canvas IDs here are the contract for T5/T8/T9/T11).

### T3 — `css/styles.css` — Ouaricon-Naturalist base, re-flowed to 5 columns
- **Files (create):** `Source/ui/public/css/styles.css`
- **Change:** Copy `O-simpleFM/Source/ui/public/css/styles.css` (618 lines) verbatim as the base skin (Claude's discretion on color/skin per RESEARCH §"Claude's Discretion"). Adjust panel/group widths so 5 signal-path columns fit projector-readable; add `.combo` styling if not present; ensure `.canvas-wrap`/canvas containers size for the headline + scope + dual-ADSR.
- **Depends on:** T2 (class names must match HTML).

### T4 — Rewrite `PluginEditor.h` — member order + Timer + analyzer member
- **Files (modify):** `Source/PluginEditor.h`
- **Change:** Replace the `GenericAudioProcessorEditor` placeholder body. Class derives `public juce::AudioProcessorEditor, private juce::Timer`. Members in **exact order** (C++ destroys in reverse — relays MUST outlive attachments; model FM `PluginEditor.h:39-56`):
  1. `OSimpleSubtractiveAudioProcessor& processorRef;`
  2. `SubVizAnalyzer vizAnalyzer;`
  3. **RELAYS:** `std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;` + `std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;` (NEW vs FM; NO toggle relays)
  4. **WEBVIEW:** `std::unique_ptr<juce::WebBrowserComponent> webView;`
  5. **ATTACHMENTS:** `std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;` + `std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;`
  - Declare `void timerCallback() override;` and `std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);`.
  - **Drop** the `FileChooser` member (FM-only; no preset-manager in scope).
- **Depends on:** none (header contract for T5/T6/T7).

### T5 — Rewrite `PluginEditor.cpp` — resource provider + relays + native fns + Windows block + attachments
- **Files (modify):** `Source/PluginEditor.cpp`
- **Change:** Replace the placeholder. Adapt `O-simpleFM/Source/PluginEditor.cpp`:
  - `#include "BinaryData.h"`.
  - `makeBinaryResource` helper + `getResource` with **bare-path direct equality** (model FM `:19-61`). Map: `/` & `/index.html` → `index_html`; `/css/styles.css` → `styles_css`; `/js/app.js` → `app_js`; `/js/juce/index.js` → `index_js`; `/js/juce/check_native_interop.js` → `check_native_interop_js`. MIME with `; charset=utf-8` on text. **Do NOT** add `/modules/...` or `/img/...` (FM-only). **Never strip scheme/host** (Pitfall 2).
  - Build `sliderIds` (16) + `comboIds` (4) StringArrays from `using namespace OSimpleSubtractive::ParamIDs` (NOT FM's 15+2).
  - **1. Relays before WebView** (model FM `:79-94`, combos from Grain `:95-96`): loop `sliderIds`→`WebSliderRelay`, loop `comboIds`→`WebComboBoxRelay`.
  - **2. Options chain** (FM `:86-89`): `withNativeIntegrationEnabled().withKeepPageLoadedWhenBrowserIsHidden().withResourceProvider(...)`, then `withOptionsFrom(*relay)` for every slider relay AND every combo relay (Grain `:108`).
  - **Native fns — exactly 3** (Grain `:169-177` + FM `:180-189`):
    - `uiMidi` → `if (args.size() >= 2) processorRef.handleUiMidi((int)args[0], (bool)args[1], args.size()>=3 ? (float)args[2] : 0.8f); complete(juce::var());`
    - `getSampleRate` → `complete(processorRef.getCurrentSampleRate());`
    - `applyFactoryPreset` → `if (args.size() > 0) processorRef.applyFactoryPreset(args[0].toString()); complete(true);`
  - **Windows block** (FM `:191-199`): `#if JUCE_WINDOWS … withWinWebView2Options(WinWebView2{}.withUserDataFolder(File::getSpecialLocation(File::tempDirectory).getChildFile("OsimpleSubtractive_WebView")).withStatusBarDisabled().withBuiltInErrorPageDisabled());` (Pitfall 6 — silent IE fallback → blank UI).
  - Construct `webView`. **3. Attachments after WebView** (FM `:203-222`, combos Grain `:210-217`): for each `sliderIds[i]` → `WebSliderParameterAttachment(*param, *sliderRelays[i], nullptr)` (3-arg, `nullptr` undoManager); for each `comboIds[i]` → `WebComboBoxParameterAttachment(*param, *comboRelays[i], nullptr)`. Keep the `jassert(param != nullptr)` guard.
  - `addAndMakeVisible(*webView); webView->goToURL(getResourceProviderRoot()); setSize(<wide enough for 5 columns + keyboard>); startTimerHz(30);`
  - Destructor `stopTimer();`. `resized()` → `webView->setBounds(getLocalBounds());`. `timerCallback()` body added in T8 (stub it empty for now so it compiles).
- **Depends on:** T4 (members), T1+T2 (resource paths), T6 (`applyFactoryPreset` must be declared on the processor or this won't compile).

### T6 — `applyFactoryPreset` wiring-only stub on the processor
- **Files (modify):** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Change:** Add `void applyFactoryPreset (const juce::String& name);` to the public section of `PluginProcessor.h` (near `handleUiMidi`, line ~166). In `.cpp`, add a minimal body: no-op (or a single trivial demo snapshot via `setValueNotifyingHost`) with a comment that **Stage 4 (FUNC-06)** fills the 8 concept snapshots. Model: `O-simpleGrain/Source/PluginProcessor.cpp:806-813`. **This is the ONLY processor change permitted in Stage 3 — it is bridge wiring, not DSP. Flag it clearly in the commit.**
- **Depends on:** none (but T5 depends on this).

### T7 — `app.js` — boot, knob binding, combo binding, keyboard (controls live)
- **Files (create):** `Source/ui/public/js/app.js`
- **Change:** Build from `O-simpleFM/Source/ui/public/js/app.js` skeleton. Carry over verbatim where noted; **the canvas/diagram/tooltip render bodies are filled in 3.2/3.3** — this task makes all controls live.
  - Header note + `import * as Juce from './js/juce/index.js'`. **`getSliderState`/`getComboBoxState`/`getNativeFunction` are on the `Juce` ES-module namespace; `window.__JUCE__.backend.addEventListener` is ONLY for emitted events** (Pitfall 1, project memory).
  - `SLIDER_IDS` (16) + `COMBO_IDS` (4) consts matching `sliderIds`/`comboIds`. Remove FM's `TOGGLE_IDS`.
  - Knob binding verbatim from FM `app.js:68-162` (`KNOB_MIN_DEG=-135`, `KNOB_MAX_DEG=135`, `DRAG_TRAVEL_PX=220`; `bindKnob`: relative vertical drag via `sliderDragStarted/setNormalisedValue/sliderDragEnded`, wheel, arrow-key `nudge`). `FORMAT` map adapted for Subtractive params (Hz/dB/%/s).
  - Combo binding verbatim from Grain `app.js:236-265` (`bindCombo(id)`: `Juce.getComboBoxState(id)`, build `<option>`s from `st.properties.choices`, set `selectedIndex` from `st.getChoiceIndex()`, `change`→`st.setChoiceIndex`, and listen to **both** `propertiesChangedEvent` AND `valueChangedEvent` because choices may arrive late — Pitfall §3.1). `COMBO_IDS.forEach(bindCombo)` in boot (Grain `:985`).
  - On-screen keyboard verbatim from FM `app.js:591-685` (`KB_LOW/HIGH`, `QWERTY` map, `noteOn/noteOff`, `setupKeyboard` → `uiMidiFn = Juce.getNativeFunction("uiMidi")`).
  - `boot()` (model FM `:700-727`): `SLIDER_IDS.forEach(bindKnob); COMBO_IDS.forEach(bindCombo); setupKeyboard();` + calls to `setupTooltips/updateDiagram/setupPresets/setupVizEvents/rewireResize` (defined in 3.2/3.3 — declare them now so boot resolves; **every referenced symbol must exist or module-eval ReferenceError kills the whole UI** — Pitfall 7). DOMContentLoaded guard.
- **Depends on:** T1 (Juce module), T2 (element IDs).

### T8 — CMake delta — single `juce_add_binary_data` target + link
- **Files (modify):** `CMakeLists.txt`
- **Change:** WebView flags are **already present** (`NEEDS_WEB_BROWSER/WEBVIEW2 TRUE`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0` — confirm, do NOT duplicate). Add ONLY:
  1. Before `target_link_libraries` (current line 46), add ONE target (model FM `CMakeLists.txt:49-59`, **drop** the preset-manager + img SOURCES lines):
     ```
     juce_add_binary_data(O-simpleSubtractive_UIResources
         SOURCES
             Source/ui/public/index.html
             Source/ui/public/css/styles.css
             Source/ui/public/js/app.js
             Source/ui/public/js/juce/index.js
             Source/ui/public/js/juce/check_native_interop.js
     )
     ```
  2. Add `O-simpleSubtractive_UIResources` as the first entry in the `PRIVATE` block of `target_link_libraries` (model FM `:62`).
  3. Do **NOT** call `ouaricon_add_module(... preset-manager)` (FM-only; out of scope).
  4. Single binary-data target → **default `BinaryData` namespace is fine**. Only a 2nd target would need a distinct `NAMESPACE` (O-simpleGrain Stage 3.1 collision lesson).
- **Depends on:** T1, T2, T3, T7 (all SOURCES files must exist before configure).

**3.1 exit:** tree configures + builds; WebView opens; 20 controls two-way bound; keyboard plays. (Canvases/diagram/tooltips may be blank/stub — filled next.)

---

## Phase 3.2 — Headline Filter-Curve-Over-Spectrum + Oscilloscope + Dual-ADSR (UI-01/02/04, PERF-01, QUAL-02)

> Goal: the teaching visuals, driven by the editor Timer @ 30 Hz. Audio thread stays copy-only (PERF-01) — all FFT/curve/scope is on the message-thread Timer.

### T9 — `timerCallback()` — analyzer + curve + emit 4 events
- **Files (modify):** `Source/PluginEditor.cpp`
- **Change:** Fill the `timerCallback()` body (model FM `:237-259` + Additive `DynamicObject` emit `PluginEditor.cpp:183-186`):
  ```
  vizAnalyzer.process(processorRef.getVizRing(), processorRef.getCurrentSampleRate());   // scope copied BEFORE FFT inside process()
  vizAnalyzer.updateCurve(processorRef.getDisplayCutoffHz(), processorRef.getDisplayK(),
                          processorRef.getDisplayType(), processorRef.getDisplaySlope(),
                          processorRef.getCurrentSampleRate());                            // closed-form curve == running filter (QUAL-02)
  if (webView == nullptr) return;
  ```
  Pack `getCurve()` (256), `getSpectrum()` (256), `getScope()` (128) into `juce::Array<juce::var>` (FM `:248-252` idiom). Build env `DynamicObject{ filterEnv: getFilterEnvValue(), ampEnv: getAmpEnvValue() }` (Additive idiom). Emit **curve before spectrum** (the curve annotates the bars): `filterCurveUpdate`, then `spectrumUpdate`, `scopeUpdate`, `envUpdate` via `emitEventIfBrowserIsVisible`.
  - **Do NOT add audio-thread work; do NOT reorder scope-vs-FFT** (already correct inside `SubVizAnalyzer::process`, `SubVizAnalyzer.h:135-151`).
- **Depends on:** T5 (editor + webView), T4 (`vizAnalyzer` member).

### T10 — `app.js` — `makeCanvas` (DPR) + viz event listeners + `rewireResize`
- **Files (modify):** `Source/ui/public/js/app.js`
- **Change:**
  - `makeCanvas(id)` verbatim from FM `app.js:414-426`: DPR backing store (`canvas.width = clientWidth*dpr`, `ctx.setTransform(dpr,0,0,dpr,0,0)`). **Explicit width/height — never `position:absolute` left+right** (canvas is a CSS replaced element; project memory + Pitfall 5).
  - `setupVizEvents()` (model FM `:560-574`): `window.__JUCE__.backend.addEventListener` for `"spectrumUpdate"`→store `lastSpectrum`+`drawHeadline()`, `"filterCurveUpdate"`→store `lastCurve`+`drawHeadline()`, `"scopeUpdate"`→`drawScope`, `"envUpdate"`→store `lastEnv`+`drawDualAdsr()`. Decode `envUpdate` as a JS object `{filterEnv, ampEnv}` (Additive shape).
  - `rewireResize()` verbatim from FM `:578-585`: re-fit canvases + redraw last frame on editor resize.
  - `fetchSampleRate()` via `Juce.getNativeFunction("getSampleRate")` → `nyquistHz` (FM idiom) for the freq-axis labels.
- **Depends on:** T7 (boot/app skeleton), T2 (canvas IDs).

### T11 — `app.js` — `drawHeadline()` (UI-01 curve over spectrum) + `drawScope()` (UI-04)
- **Files (modify):** `Source/ui/public/js/app.js`
- **Change:**
  - `drawHeadline()` adapts FM `drawSpectrum` (`app.js:441-482`) **minus the FM sideband markers**. ONE canvas, two layers: (1) spectrum bars from `lastSpectrum` (256), then (2) the filter curve line from `lastCurve` (256) stroked 2px on top. Both use the SAME x-map `x=(b/255)*w` (axis identity proven, `SubVizAnalyzer.h`) and the SAME dB→y window. **Use a shared dB window ~`[-90, +18]`** so the resonance/self-osc peak does not flat-top (curve peaks exceed 0 dB; A3). `y = h - ((db-DB_MIN)/(DB_MAX-DB_MIN))*h`, clamp 0..h. Reuse FM log-freq tick labels (`:468-479`, ticks `[100,1000,10000]`, label via `nyquistHz`).
  - `drawScope(arr)` verbatim from FM `app.js:535-558` (128 pts, center line, 2px). `getScope()` is post-filter so it morphs with cutoff/res/env automatically.
- **Depends on:** T10 (`makeCanvas`, listeners, `lastSpectrum`/`lastCurve`).

### T12 — `app.js` — `drawDualAdsr()` (UI-02, NEW renderer — no sibling precedent)
- **Files (modify):** `Source/ui/public/js/app.js`
- **Change:** Design here (MEDIUM confidence — flagged risk). Draw two ADSR shapes (filter env top, amp env below) on the dual-ADSR canvas(es from T2). Each shape computed in JS from its 4 `sliderState` scaled values (A,D,R seconds + S level 0..1): map A+D+R to an x-budget with a fixed sustain plateau, y = level 0..1. Redraw on any of the 8 ADSR `valueChangedEvent`s. **Playheads:** from `lastEnv.{filterEnv,ampEnv}` (live 0..1) — draw a vertical/dot marker whose **y = the live env value** on each shape (vertical-only marker per Open Question 1 — no time index is exposed; do NOT add a DSP atomic). Label filter-env axis "→ cutoff" and amp-env axis "→ level" so the **independent scales** read clearly (CONTEXT UI-02).
- **Depends on:** T10 (canvas + `envUpdate` listener), T7 (`sliderState` populated by `bindKnob`).

**3.2 exit:** sweeping cutoff/res/slope/type/filter-env moves the curve over the live spectrum, harmonics above cutoff visibly attenuated, curve matches what's heard (QUAL-02), self-osc shows a peak; scope morphs; dual-ADSR shows both envelopes moving independently. No audio-thread FFT/alloc; smooth at 30 Hz.

---

## Phase 3.3 — Signal-Path Diagram + Tooltips + Preset Tour Hook (UI-03/07, FUNC-06)

> Goal: pedagogical scaffolding. Preset *content* lands in Stage 4 — Stage 3 ships only the selectable UI hook + live bridge.

### T13 — Signal-path SVG diagram + `updateDiagram()` (UI-03)
- **Files (modify):** `Source/ui/public/index.html`, `Source/ui/public/js/app.js`, (CSS as needed in `css/styles.css`)
- **Change:** **SVG, not canvas** (RESEARCH recommendation: crisp at projector res, no DPR mgmt, declarative highlight). In `index.html`, populate `<svg id="routingSvg">` (model FM `index.html:48-71`) re-skinned: `OSC → FILTER → VCA → ♪` with two envelope-route arrows (FILTER ADSR → filter cutoff; AMP ADSR → VCA). Use FM's `<line>`/`<polygon>` arrow + `<circle>`/`<text>` node idiom; give each dynamic element a stable `id`.
  - In `app.js`, `updateDiagram()` (model FM `updateRouting` `app.js:188-236`): OSC node label reflects `oscWave` choice; FILTER node text reflects `filterType` + `filterSlope` (e.g. "24 dB LP"); filter-env arrow opacity/width ∝ `filterEnvAmount` magnitude (color the bipolar sign); VCA node pulses with live `lastEnv.ampEnv`. **Guard every `getElementById` with `if (el)`** (typo → silent dead highlight; Pitfall §3.3). Hook the relevant `sliderState`/`comboState` `valueChangedEvent`s to `updateDiagram()` in boot (FM boot idiom `:704-709`).
- **Depends on:** T2 (SVG placeholder), T7 (states bound), T10 (`lastEnv` for the VCA pulse).

### T14 — Pedagogical tooltips on every control (UI-05)
- **Files (modify):** `Source/ui/public/js/app.js`
- **Change:** `TIPS` const-map + `setupTooltips()` verbatim from FM `app.js:39-65, 239-285` (pointerenter/move/leave + focusin/out for keyboard a11y + Escape-to-hide). Write **plain-language pedagogy** copy (~20+ entries — one per param, plus the diagram/readout) per BRIEF: what cutoff does, why resonance whistles (self-oscillation), what "poles"/slope mean, filter-env vs amp-env routing. **Every `data-tip` key in `index.html` (T2) must match a `TIPS` key exactly** (missing key = silent no-tip; Pitfall §3.3).
- **Depends on:** T2 (`data-tip` attributes), T7 (boot calls `setupTooltips`).

### T15 — Preset-tour hook (UI-07) — buttons + wired `applyFactoryPreset`
- **Files (modify):** `Source/ui/public/index.html`, `Source/ui/public/js/app.js`
- **Change:** Copy O-simpleGrain's tour pattern (NOT FM's `OuariconPresetManager` — out of scope). In `index.html`, a `.preset-tour` section with `<button class="tour-btn" data-preset="…">` per concept + a `#tourCaption` (model FM `index.html:186-196`, Grain captions). In `app.js`, `setupPresets()` verbatim-shaped from Grain `app.js:805-835`: `applyPresetFn = Juce.getNativeFunction("applyFactoryPreset")`; each button `click` → `applyLesson(label)` → `await applyPresetFn(label)` + set caption + active highlight. Loading is C++-side `setValueNotifyingHost` → relays/attachments sync every knob/combo back to the page automatically (no DOM poking). **Stage 3 ships buttons + the live bridge; the C++ body (8 snapshots) is Stage 4 (FUNC-06)** — the T6 stub is sufficient for the bridge to be live.
- **Depends on:** T6 (processor stub), T5 (native fn registered), T2 (tour section markup), T7 (boot calls `setupPresets`).

**3.3 exit:** diagram reflects osc/filter/envelope state; every control has a pedagogical tooltip; preset tour selectable + bridge live.

---

## Pre-execute verification checklist (gui-agent runs BEFORE declaring done)

Run these in-tree before handoff to the verify phase. Each maps to a known silent-failure class.

- [ ] **(a) Native-fn bridge parity** — grep-diff must match EXACTLY (Pitfall 4 / `pattern_webview_native_fn_bridge_gap`):
      `grep -o 'getNativeFunction("[^"]*")' Source/ui/public/js/app.js | sort -u` vs
      `grep -o 'withNativeFunction ("[^"]*"' Source/PluginEditor.cpp | sort -u`
      → both resolve to exactly `{uiMidi, getSampleRate, applyFactoryPreset}`.
- [ ] **(b) No module-load ReferenceError** — every module-level helper referenced in `boot()` is defined (`setupTooltips`, `updateDiagram`, `setupPresets`, `setupVizEvents`, `rewireResize`, `bindKnob`, `bindCombo`, `setupKeyboard`, `makeCanvas`, `drawHeadline`, `drawScope`, `drawDualAdsr`). A single undefined symbol aborts the whole script → blank UI that still passes build/auval (Pitfall 7).
- [ ] **(c) All 20 params have a relay + attachment** — `sliderIds` has 16, `comboIds` has 4; each appears in both the relay loop and the attachment loop in `PluginEditor.cpp`; all 20 IDs exist in `OSimpleSubtractive::ParamIDs` (`PluginProcessor.h:28-61`). `jassert(param != nullptr)` present on both loops.
- [ ] **(d) Canvas sizing** — `makeCanvas` uses explicit `clientWidth*dpr`/`clientHeight*dpr` backing store + `ctx.setTransform(dpr,…)`; no canvas relies on `position:absolute` left+right to stretch (Pitfall 5).
- [ ] **(e) `Juce` namespace, not `window.__JUCE__`** — `getSliderState`/`getComboBoxState`/`getNativeFunction` are all called on `Juce.*`; `window.__JUCE__.backend.addEventListener` is used ONLY for the 4 emitted events (Pitfall 1).
- [ ] **(f) Resource provider parity** — every path in `getResource` has a real BinaryData symbol and a real file in the `juce_add_binary_data` SOURCES; bare-path direct equality, no scheme stripping (Pitfall 2).
- [ ] **(g) Event parity** — the 4 C++ `emitEventIfBrowserIsVisible` names match the 4 JS `addEventListener` names exactly (`filterCurveUpdate, spectrumUpdate, scopeUpdate, envUpdate`).
- [ ] **(h) DSP untouched** — `git diff` on `Source/{OscillatorBank.h,SvfZDF.h,SubVoice.h,SubVizAnalyzer.h}` and the `processBlock`/voice code in `PluginProcessor.cpp` is empty; the ONLY processor change is the `applyFactoryPreset` stub (T6).
- [ ] **(i) Tree builds** — `ninja O-simpleSubtractive_VST3 O-simpleSubtractive_AU` configures + compiles clean (the formal pluginval/auval/visual gate is the verify phase, but execute must leave the tree building).

---

## Success criteria (from CONTEXT.md — goal-backward check)

1. WebView opens; single-page left→right signal-path layout renders, projector-readable (UI-06).
2. All 20 controls two-way bound (drag → DSP; host automation → UI).
3. Sweeping cutoff/res/slope/type/filter-env moves the filter curve over the live spectrum; harmonics above cutoff visibly attenuated; **curve matches what's heard** (QUAL-02); self-osc shows a peak at cutoff.
4. Oscilloscope shows the filtered waveform morphing; dual-ADSR shows the two envelopes moving independently.
5. Live signal-path diagram reflects osc/filter/envelope state; every control has a pedagogical tooltip; preset tour selectable from UI.
6. Builds VST3 + AU on macOS; CMake configured for Windows VST3 (no blank-UI flags missing). pluginval + auval pass.
7. No audio-thread FFT/alloc; UI smooth at 30 Hz.

---

## Validation gate (NOT in execute — verify phase)

Execute must leave the tree building (checklist item (i)). The formal gate runs in `/plugin-verify`:
- `ninja O-simpleSubtractive_VST3 O-simpleSubtractive_AU` clean.
- `pluginval` (VST3 + AU) passes.
- `auval -a | grep -i subtractive` passes.
- `show-standalone` visual check: layout renders, all controls move, headline curve-over-spectrum + scope + dual-ADSR animate, diagram + tooltips + preset tour work.
- Standard CLAUDE.md cache-clear + dual-variant install sweep on install.

---

## Risks

- **R1 — Dual-ADSR renderer (T12):** the only component with no sibling precedent (RESEARCH confidence MEDIUM). Vertical-only playhead (live env value on y) is the agreed Stage-3 scope; a time-advancing playhead would need a new DSP atomic (out of scope — do NOT touch DSP). Mitigation: ship the vertical marker; it satisfies the "independent scales" pedagogy.
- **R2 — dB window clipping (T11):** resonance/self-osc curve peaks exceed 0 dB; if `DB_MAX` too low the peak flat-tops. Mitigation: `DB_MAX ≥ +18` (A3, cosmetic — tune in verify).
- **R3 — Silent failure classes:** native-fn gap, module-load ReferenceError, `window.__JUCE__` vs `Juce`, bare-path resource provider, canvas replaced-element sizing, Windows IE fallback — all pass build/auval but break the live UI. Mitigation: the pre-execute checklist (a)–(i) gates each one before handoff.
- **R4 — `applyFactoryPreset` processor change (T6):** the only sanctioned processor edit; keep it a wiring-only stub and flag in the commit so it's not mistaken for a DSP change.
- **R5 — Combo choices late on first load:** `bindCombo` must listen to `propertiesChangedEvent` AND `valueChangedEvent` or the 4 selectors render empty (Grain pattern; covered in T7).
