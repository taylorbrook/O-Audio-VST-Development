# Stage 3 (GUI) — Research: Implementation-Ready Reference

**Researched:** 2026-06-25
**Domain:** JUCE 8 WebView plugin editor (Ouaricon "simple" series) — 18-param two-way binding, 4 live visualizations, drag-drop source loading, 8 concept presets
**Confidence:** HIGH (every pattern grounded in two shipped siblings + the already-built Stage-2 taps; no novel C++/JS infrastructure required)

> **The strategy is reuse.** O-simpleFM and O-simpleAdditive are two shipped, proven Ouaricon-Naturalist WebView editors. Stage 3 is ~90% copy-and-adapt: the relay/attachment/resource-provider/timer skeleton, the knob/toggle/combo JS, the canvas DPR pattern, the preset snapshot pattern, and the CSS palette are all already written and validated. The only genuinely-new code is **four canvas renderers** (cloud / source-waveform / scope / spectrum) consuming the **already-built** `GrainCloudFrame`/`VizRing`/`GrainVizAnalyzer` taps, and the **JS drag-drop bind** to the **already-built** C++ `dropSession*` native fns.

---

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D1 — Aesthetic:** continue the Ouaricon Audio Naturalist / "Field Guide" theme. Aged-paper bg, Garamond serif, seed-cross-section knobs, green botanical toggles, brown frame. Subtitle **"Granular Synthesizer · A Field Guide."** Reuse `O-simpleFM/Source/ui/public/css/styles.css` as the CSS base. New granular canvases (cloud scatter, source-waveform-with-playheads, window inset, scope/spectrum) styled to the sepia/brown palette (sepia dots on aged paper, brown playhead lines).
- **D2 — Workflow:** **No separate ui-mockup HTML loop.** Build the production `index.html` + `css/` + `js/` directly on the sibling structure; first visual review is the live plugin build at end of 3.1.
- **D3 — Layout:** balanced **2×2 visualization grid** (cloud / source-waveform / scope / spectrum) + **side control rail** (Source · Grain · Window · Spray&Scatter · Amp · Output). Window-envelope inset tucked into the cloud or waveform corner.
- **D4 — Presets:** ship **all 8 concept presets** in Phase 3.3 — Single Grain · Pitched Buzz · Fragments · Smooth Cloud · Frozen Pad · Asynchronous Cloud · Granular Fire · Rect Click. Each isolates exactly one concept (FUNC-06). Via APVTS snapshots, selectable from the header preset bar.

### Claude's Discretion
- Exact preset-bar UX (the additive `applyFactoryPreset` snapshot tour vs the FM `preset-manager` file-persistence bar) — **RESEARCH recommends the additive snapshot pattern** (see Reuse map + §Preset bar). The 8 concept presets are read-only teaching snapshots, not user-saved JSON; the additive pattern is the exact fit and avoids the `preset-manager` module dependency.
- Window-inset corner placement (cloud corner vs waveform corner).
- On-screen keyboard inclusion (both siblings ship one; recommended to keep for parity — lets the teacher play without external MIDI).

### Deferred Ideas (OUT OF SCOPE — Stage 4)
pluginval (VST3+AU) sweep, preset audit, artifact/aliasing/freeze listen audit, Windows drag-drop smoke test, changelog. Stage 3 verify only checks the GUI goal + clean build + auval.

---

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| UI-01 | Grain-cloud scatter visualization | `GrainCloudFrame.events[]` → cloud canvas. Shape fully documented §Viz contracts. No sibling has this exact canvas — **new renderer**, but data tap is built. |
| UI-02 | Source-waveform with live playheads / freeze pin / spray range shade | `GrainCloudFrame.playheadNorm/positionNorm/positionSprayNorm/frozen` + a static source-waveform thumbnail. **New renderer**; needs a source-thumbnail native fn (§Open Q1). |
| UI-03 | Window-envelope inset | Draw window LUT for one grain; redraw on `windowShape` change. **New small renderer**; needs LUT exposure (§Open Q2) or JS recompute. |
| UI-04 | Scope + spectrum | `GrainVizAnalyzer` (4096/Blackman-Harris) over `VizRing` → 128-pt scope + 256-bin spectrum. **Verbatim FM/Additive pattern** (the analyzer is already copied into O-simpleGrain). |
| UI-05 | Grain-count / overlap / CPU readout | `getActiveGrainCount()` + `grainSizeSec×density` math. **New small readout**; trivial JS. |
| FUNC-03 | Freeze | `freeze` toggle relay + `GrainCloudFrame.frozen` pin marker on the waveform. |
| FUNC-06 | 8 concept presets | `applyFactoryPreset` snapshot native fn (additive pattern). |
| Drag-drop / picker | Load-your-own source | `bindWebViewFileDrop` JS → C++ `dropSession*` (already built). |
| All 18 params | Two-way bind | 16 `WebSliderRelay` + 2 `WebComboBoxRelay` + 1 `WebToggleButtonRelay`. |

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| 18 param two-way binding | C++ editor (relays/attachments) | JS (`getSliderState`/`getComboBoxState`/`getToggleState`) | JUCE relay bridge is the only sanctioned APVTS↔WebView path |
| FFT / scope analysis | C++ message-thread Timer (`GrainVizAnalyzer`) | — | Already built; audio thread is copy-only into `VizRing` (PERF-01) |
| Grain-cloud / playhead state | C++ audio thread (fills `GrainCloudFrame`) → message-thread read | JS canvas render | Lock-free `TripleBuffer` SPSC handoff already built |
| Canvas rendering (4 viz + inset) | JS (`<canvas>` 2D, DPR-aware) | — | All drawing on the page; C++ only pushes data via `emitEventIfBrowserIsVisible` |
| Source decode/resample/publish | C++ message thread (drag-drop / picker) | JS streams base64 | Already built (`dropSession*`, `loadSourceFromFileChooser`); atomic `shared_ptr` swap to audio thread |
| Preset snapshots (8) | C++ (`applyFactoryPreset` → `setValueNotifyingHost`) | JS tour buttons | Relays propagate the snapshot back to every knob automatically |

---

## Reuse Map

> For every new O-simpleGrain GUI file: the exact sibling file to copy/adapt and what changes. **Closest sibling differs per file** — combo plumbing comes from Additive, the preset-bar/keyboard/tooltip scaffolding from either, the spectrum/scope analyzer is already in-tree.

| New O-simpleGrain file | Copy from | What changes |
|------------------------|-----------|--------------|
| `Source/ui/public/css/styles.css` | **`O-simpleFM/.../css/styles.css`** (D1 names FM as the CSS base) | Add `.viz-grid` (2×2), `.window-inset`, `.grain-readout`, `.source-drop-zone`, `.side-rail` rules in the same sepia palette. Keep knob/toggle/header/preset-bar/keyboard components verbatim. Add a `<select>.combo` rule (lift from Additive's CSS for the two combos). |
| `Source/ui/public/js/juce/index.js` | **copy verbatim** from either sibling | None — this is the JUCE-generated ES module. |
| `Source/ui/public/js/juce/check_native_interop.js` | **copy verbatim** from either sibling | None. |
| `Source/ui/public/img/insects.png` | copy from either sibling (or swap art) | Optional: a granular-appropriate botanical (seed/spore) overlay; reuse `insects.png` if no new art. |
| `Source/ui/public/index.html` | **`O-simpleAdditive/.../index.html`** (it already has the two `<select class="combo">` + the `applyFactoryPreset` tour-button markup we need) | Replace the drawbar bay with the **side control rail** (6 groups, 16 knobs + 2 selects + 1 toggle). Replace the 2-up viz panel with the **2×2 viz grid** + window inset. Add `source-drop-zone` + `Load…` button. Change 8 `.tour-btn data-preset` names to the concept presets. Title → "O-simpleGrain", subtitle → "Granular Synthesizer · A Field Guide". |
| `Source/ui/public/js/app.js` | **Additive `app.js`** as the structural base (it has `bindCombo` + the `applyFactoryPreset` tour) **+ FM `app.js`** for `drawSpectrum`/`drawScope`/`setupVizEvents`/log-freq-axis | Swap param inventory to the 18 grain IDs. Add 4 new canvas renderers (`drawCloud`, `drawSourceWaveform`, `drawWindowInset`, plus reuse `drawScope`/`drawSpectrum`). Add the drag-drop bind + `Load…` picker call + toast. Rewrite tooltip copy for the granular params. |
| `Source/PluginEditor.h` | **Additive `PluginEditor.h`** (it has both `WebSliderRelay` + `WebComboBoxRelay` vectors) **+ add** `WebToggleButtonRelay` vector (from FM, for `freeze`) | Add `GrainVizAnalyzer vizAnalyzer;`, `std::unique_ptr<juce::FileChooser> fileChooser;`, the toggle relay/attachment vectors. |
| `Source/PluginEditor.cpp` | **Additive `PluginEditor.cpp`** (relay/combo/timer skeleton) **+** FM's spectrum/scope timer body **+** sampler's `dropSession*` native-fn registration | Wire 16 sliders + 2 combos + 1 toggle. Register `dropSessionStart/AddFile/CommitFile/CommitFolder`, `loadSourceFromFileChooser`, `applyFactoryPreset`, `getSampleRate`, optional `uiMidi`, optional source-thumbnail fn. Timer pushes 4 viz events. |
| `CMakeLists.txt` (edit) | Pattern from **FM `CMakeLists.txt`** (the `juce_add_binary_data` + `ouaricon_add_module` block) | Add `juce_add_binary_data(O-simpleGrain_UIResources ...)` listing the 7 UI files (+ `webview-drop-streaming.js` if used as a module — see §Drag-drop). Add the drop-streaming module via `ouaricon_add_module(O-simpleGrain webview-drop-streaming)` (the C++ side is already in-tree per PluginProcessor.h, so confirm whether the module is already linked — §Open Q3). WebView2 flags **already present** (verified). |

**Files to author fresh (no verbatim sibling):** the 4 viz canvas renderers inside `app.js` (`drawCloud`, `drawSourceWaveform`, `drawWindowInset`, `drawGrainReadout`) and their `setupVizEvents` subscriptions. Everything else is copy-and-edit.

**Decision — preset bar:** use the **Additive `applyFactoryPreset` snapshot pattern**, NOT the FM `preset-manager` module. The 8 concept presets are read-only teaching snapshots; the additive pattern (one C++ native fn iterating `setValueNotifyingHost`, JS tour buttons) is the exact fit, needs no JSON persistence, and avoids adding the `preset-manager` module + its 10 native fns. `[VERIFIED: O-simpleAdditive/Source/PluginProcessor.cpp:359 + app.js:399-426]`

---

## C++ Editor Blueprint

Grounded in `O-simpleAdditive/Source/PluginEditor.{h,cpp}` (combo + timer skeleton) and `O-simpleFM/Source/PluginEditor.cpp` (spectrum/scope timer + Windows options).

### Member declaration order (MUST be exactly this — C++ destroys in reverse)
`[VERIFIED: O-simpleFM/PluginEditor.h:39-59, O-simpleAdditive/PluginEditor.h:43-60]`

```
// 1. RELAYS (declared first → destroyed last)
std::vector<std::unique_ptr<juce::WebSliderRelay>>        sliderRelays;   // 16 float knobs
std::vector<std::unique_ptr<juce::WebComboBoxRelay>>      comboRelays;    // sourceSample, windowShape
std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>  toggleRelays;   // freeze

// 2. WEBVIEW
std::unique_ptr<juce::WebBrowserComponent> webView;

// 3. ATTACHMENTS (declared last → destroyed first, while WebView still alive)
std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>     comboAttachments;
std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

// message-thread analyzer + held file chooser
GrainVizAnalyzer vizAnalyzer;                       // already in-tree (VizAnalyzer.h)
std::unique_ptr<juce::FileChooser> fileChooser;     // held across async picker
```
Editor is `public juce::AudioProcessorEditor, private juce::Timer`.

### Param ID inventory (split by relay type)
`[VERIFIED: PluginProcessor.h:37-68 OSimpleGrain::ParamIDs]`

```cpp
using namespace OSimpleGrain::ParamIDs;
const juce::StringArray sliderIds {                 // 16 WebSliderRelay
    grainSize, density, position, scan,
    pitchSpray, positionSpray, scatter, grainPitch, panSpray, velToDensity,
    ampAttack, ampDecay, ampSustain, ampRelease, outputLevel
    // NB: that is 15 — see note below; the 16th is none. Count: 16 floats MINUS
    // the two choices and one bool. Re-derive: 18 total − sourceSample(combo)
    // − windowShape(combo) − freeze(toggle) = 15 sliders.
};
const juce::StringArray comboIds  { sourceSample, windowShape };   // 2 WebComboBoxRelay
const juce::StringArray toggleIds { freeze };                       // 1 WebToggleButtonRelay
```
> **Count correction (load-bearing):** 18 APVTS params = **15 sliders + 2 combos + 1 toggle**. (Source group's `sourceSample` is a combo; `windowShape` is a combo; `freeze` is the toggle; the remaining 15 floats are sliders.) The planner must use 15, not 16, in the slider array.

### Construction sequence (mirror Additive:63-156 / FM:64-229)
1. Build relays from the three ID arrays (`std::make_unique<juce::WebSliderRelay>(id)` etc.) **before** the WebView. `[VERIFIED: Additive:82-86]`
2. `juce::WebBrowserComponent::Options{}.withNativeIntegrationEnabled().withKeepPageLoadedWhenBrowserIsHidden().withResourceProvider(...)`, then `.withOptionsFrom(*relay)` for every relay. `[VERIFIED: Additive:88-97]`
3. Chain `.withNativeFunction(...)` for: the 5 drop fns, `applyFactoryPreset`, `getSampleRate`, optional `uiMidi`, optional `getSourceThumbnail` (§Open Q1). `[VERIFIED: FM:96-189 native-fn chain shape; Additive:99-116]`
4. `#if JUCE_WINDOWS` → `withWinWebView2Options(...withUserDataFolder(tempDir.getChildFile("OsimpleGrain_WebView")).withStatusBarDisabled().withBuiltInErrorPageDisabled())`. `[VERIFIED: FM:191-199, Additive:118-126]`
5. `webView = std::make_unique<juce::WebBrowserComponent>(options);`
6. Build attachments **after** the WebView, one per ID, with `jassert(param != nullptr)` and **3-arg ctor + `nullptr` undoManager**. `[VERIFIED: Additive:131-149, FM:204-222]`
7. `addAndMakeVisible(*webView); webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());`
8. `setSize(W, H); startTimerHz(30);` — recommend `setSize(900, 760)` (2×2 grid is wider than the FM stack; the additive frame is 860×980). Final size is Claude's discretion at first live review (D2). `[VERIFIED: FM:227, Additive:154]`

### Resource provider — BARE PATHS, direct equality
`[VERIFIED: FM:32-61, Additive:35-60]` `[project memory: "Resource Provider Receives PATHS, Not Full URLs"]`

```cpp
if (url == "/" || url == "/index.html")  return makeBinaryResource(BinaryData::index_html, ..., "text/html; charset=utf-8");
if (url == "/css/styles.css")            ... "text/css; charset=utf-8";
if (url == "/js/app.js")                 ... "application/javascript; charset=utf-8";
if (url == "/js/juce/index.js")          ...;
if (url == "/js/juce/check_native_interop.js") ...;
if (url == "/modules/webview-drop-streaming.js") ...;   // if drop module used as a file
if (url == "/img/insects.png")           ... "image/png";
return std::nullopt;
```
- **`charset=utf-8` mandatory** on all text resources (the page uses ♪/fleuron/en-dash entities; missing charset mojibakes them on some hosts). `[VERIFIED: FM:35-37]`
- Never strip scheme/host — a bare path has none, so `fromFirstOccurrenceOf("://")` collapses every lookup to "". `[project memory]`

### Drop / picker native-fn registration (C++ side already built)
The processor exposes `dropSessionStart`, `dropSessionAddFile`, `dropSessionCommitFile`, `dropSessionCommitFolder` (bool), and `loadSourceFromFileChooser()` (void, async). `[VERIFIED: PluginProcessor.h:141-150]` Register each as a `withNativeFunction` that forwards args to the processor method and `complete(...)`s the bool. The names are **FIXED by the shared module — do NOT rename** (CONTEXT + module.yaml warning). Pattern shape: same as FM's preset native fns (FM:100-178), but the bodies call `processorRef.dropSession*` / `processorRef.loadSourceFromFileChooser()`.

### `applyFactoryPreset` native fn (8 concept presets) — author in PluginProcessor.cpp
`[VERIFIED: O-simpleAdditive/PluginProcessor.cpp:359-381]` Pattern:
```cpp
void OSimpleGrainAudioProcessor::applyFactoryPreset (const juce::String& name) {
    for (auto* p : getParameters()) p->setValueNotifyingHost (p->getDefaultValue());  // reset first
    auto setReal   = [this](const char* id, float real){ if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(real)); };
    auto setChoice = [this](const char* id, int idx){ if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1((float)idx)); };
    auto setBool   = [this](const char* id, bool on){ if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(on ? 1.0f : 0.0f); };
    if (name == "Single Grain") { setReal(grainSize, 30); setReal(density, 1); /* … */ }
    else if (name == "Pitched Buzz") { setReal(grainSize, 5); setReal(density, 200); setReal(scatter, 0); /* … */ }
    // … Fragments, Smooth Cloud, Frozen Pad (setBool(freeze,true)), Asynchronous Cloud (setReal(scatter,100)),
    //    Granular Fire (setChoice(sourceSample, 0)), Rect Click (setChoice(windowShape, 0)/*rect*/)
}
```
Register `withNativeFunction("applyFactoryPreset", [this](const juce::Array<juce::var>& a, auto complete){ if(a.size()>0) processorRef.applyFactoryPreset(a[0].toString()); complete(juce::var()); })`. `[VERIFIED: Additive:112-116]` Relays sync every knob/combo/toggle automatically — no extra UI wiring. **The 8 preset value tables are the new content the planner must author** (concept→param mapping); each isolates one concept per FUNC-06.

### Timer body (30 Hz) — push 4 viz events + sample rate
`[VERIFIED: FM:237-259 scope+spectrum, Additive:164-193 push shape]`

```cpp
void timerCallback() override {
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());  // FFT+scope on MSG thread
    if (webView == nullptr) return;

    // 1. scope + spectrum (UI-04) — verbatim FM pattern
    pushFloatArray("spectrumUpdate", vizAnalyzer.getSpectrum());  // 256 dB bins
    pushFloatArray("scopeUpdate",    vizAnalyzer.getScope());     // 128 pts [-1,1]

    // 2. grain cloud + playheads (UI-01/02) — read the published frame
    const GrainCloudFrame& f = processorRef.getGrainCloudBuffer().read();
    webView->emitEventIfBrowserIsVisible("grainCloudUpdate", makeCloudVar(f));   // events[] + playhead/position/spray/frozen

    // 3. grain meter (UI-05)
    webView->emitEventIfBrowserIsVisible("grainMeterUpdate", juce::var(processorRef.getActiveGrainCount()));
}
```
- `makeCloudVar(f)` builds a `DynamicObject` with `count`, a `var` array of per-grain `[readPosNorm,sizeMs,pitchSemis,pan,spawnSample]` (or a flat Float32-style array), plus `playheadNorm`, `positionNorm`, `positionSprayNorm`, `frozen`. (Additive's `drawbarSpectrumUpdate` shows the `DynamicObject` + nested array push shape. `[VERIFIED: Additive:182-191]`)
- **`windowInsetUpdate` (UI-03)** is NOT pushed every frame — it changes only on `windowShape`. Push it once at boot and on the combo's `valueChanged`, OR compute the window LUT in JS (§Open Q2). Recommended: emit on change from C++ if the LUT is exposed; else recompute in JS (5 closed-form windows, trivial).
- **Order matters** for any "context-before-frame" pairing (FM emits `carrierUpdate` before `spectrumUpdate` so the same frame's markers are correct — FM:256-258). For grain we have no equivalent ordering dependency.

---

## Web Layer Blueprint

### File list (copy-as-is vs author-new)

| File | Disposition |
|------|-------------|
| `js/juce/index.js` | **copy verbatim** (JUCE ES module — `getSliderState`/`getComboBoxState`/`getToggleState`/`getNativeFunction`) |
| `js/juce/check_native_interop.js` | **copy verbatim** |
| `img/insects.png` | copy verbatim (or swap for a seed/spore overlay) |
| `css/styles.css` | **copy FM verbatim, then add** granular-specific rules (viz grid, inset, readout, drop-zone, side-rail, `.combo` from Additive) |
| `modules/webview-drop-streaming.js` | **copy verbatim** from `modules/core/webview-drop-streaming/js/` (or pull via `ouaricon_add_module` which copies it into `ui/public/modules/`) |
| `index.html` | **adapt from Additive** (combo + tour markup present) |
| `js/app.js` | **adapt** (Additive structure + FM viz renderers + new cloud/waveform/inset renderers + drop bind) |

### Layout — 2×2 viz grid + side rail (D3)
```
┌──────────────────────────────── frame ───────────────────────────────┐
│ header: title "O–simpleGrain" + subtitle + preset bar (8 concept tour) │
├───────────────────────────────────┬───────────────────────────────────┤
│  VIZ GRID (2×2)                    │  SIDE CONTROL RAIL                │
│  ┌─────────────┬─────────────┐     │  ▸ Source   (sourceSample select  │
│  │ grain cloud │ source wave │     │             + Load… + drop-zone)  │
│  │  (UI-01)    │ +playheads  │     │  ▸ Grain    (grainSize,density,    │
│  │ [window     │  (UI-02)    │     │             position,scan,freeze)  │
│  │  inset      ├─────────────┤     │  ▸ Window   (windowShape select)  │
│  │  corner]    │   scope     │     │  ▸ Spray&Scatter (pitchSpray,      │
│  ├─────────────┤  (UI-04)    │     │             positionSpray,scatter, │
│  │  spectrum   │             │     │             grainPitch,panSpray,   │
│  │  (UI-04)    │             │     │             velToDensity)          │
│  └─────────────┴─────────────┘     │  ▸ Amp ADSR (4 knobs)             │
│  grain readout: Grains N/192 ·     │  ▸ Output   (outputLevel)         │
│  Overlap ×Y · CPU ▮▮▯               │                                   │
├───────────────────────────────────┴───────────────────────────────────┤
│ on-screen keyboard (optional, parity with siblings)                    │
└────────────────────────────────────────────────────────────────────────┘
```
Window inset (UI-03) tucked in the cloud or waveform corner (Claude's discretion). Use CSS grid for the 2×2; each cell is a `.canvas-wrap` with a `<canvas>`.

### Canvas pattern — DPR-aware backing store (MUST hold)
`[VERIFIED: FM app.js:413-426 makeCanvas]` `[project memory: Canvas Replaced Element]`
```js
function makeCanvas(id) {
  const canvas = document.getElementById(id), ctx = canvas.getContext("2d");
  const resize = () => {
    const dpr = window.devicePixelRatio || 1;
    canvas.width  = Math.max(1, Math.round(canvas.clientWidth  * dpr));
    canvas.height = Math.max(1, Math.round(canvas.clientHeight * dpr));
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  };
  resize(); return { canvas, ctx, resize };
}
```
- CSS sizing of each canvas MUST use `width: calc(100% - Npx); height: calc(100% - Npx);` — **never** `right/bottom` (a `<canvas>` is a CSS replaced element; `position:absolute; right/bottom` does NOT stretch it, leaving it at 300×150). `[project memory]`
- Single `window 'resize'` handler re-fits **all** canvases then redraws the last frame (FM `rewireResize` :578-585).

### JS viz-event subscription (low-level backend, NOT Juce.*)
`[VERIFIED: FM app.js:560-574]` `[CONTEXT line 92-93]`
```js
function setupVizEvents() {
  const be = window.__JUCE__ && window.__JUCE__.backend;
  if (!be) { console.error("backend unavailable — viz dead"); return; }
  be.addEventListener("scopeUpdate",       (a) => drawScope(a));        // UI-04
  be.addEventListener("spectrumUpdate",    (a) => drawSpectrum(a));     // UI-04
  be.addEventListener("grainCloudUpdate",  (f) => { drawCloud(f); drawSourceWaveform(f); });  // UI-01/02
  be.addEventListener("grainMeterUpdate",  (n) => drawGrainReadout(n)); // UI-05
  be.addEventListener("windowInsetUpdate", (lut) => drawWindowInset(lut)); // UI-03 (on-change only)
}
```
> **Event names are the C++/JS contract** — they must match `emitEventIfBrowserIsVisible` exactly: `scopeUpdate`, `spectrumUpdate`, `grainCloudUpdate`, `grainMeterUpdate`, `windowInsetUpdate` (CONTEXT line 93). Viz subscriptions use `window.__JUCE__.backend.addEventListener` (the low-level backend); param state uses the `Juce` namespace.

### Knob / toggle / combo binding (copy verbatim, swap IDs)
- **Knob** (relative vertical drag, ±135°, wheel + arrow-key, `data-tip` tooltip): FM `bindKnob` :103-162. `Juce.getSliderState(id)`; `sliderDragStarted`/`setNormalisedValue`/`sliderDragEnded`. `[VERIFIED]`
- **Toggle** (`freeze`): FM `bindToggle` :165-180. `Juce.getToggleState(id)`. `[VERIFIED]`
- **Combo** (`sourceSample`, `windowShape`): Additive `bindCombo` :251-280. `Juce.getComboBoxState(id)`; build `<option>`s from `st.properties.choices`; `st.setChoiceIndex(sel.selectedIndex)` on change; `st.getChoiceIndex()` to reflect. HTML: `<select class="combo" id="combo-<id>">`. `[VERIFIED]`

### Drag-drop bind (single source)
The shared module's `bindWebViewFileDrop(opts)` is folder/cell-oriented (built for the sampler). For O-simpleGrain's **single source**, the relevant primitives are `streamSingleFileEntryToCpp(fileEntry, midi, vel, opts)` and `readFileEntryAsBase64`. `[VERIFIED: webview-drop-streaming.js:326-369]` Recommended approach:
- Bind a document-level `drop` handler over the `#source-drop-zone`. On a single-file audio drop, get `webkitGetAsEntry()`, then call the C++ flow: `dropSessionStart(sessionId)` → `dropSessionAddFile(sessionId, name, base64)` → `dropSessionCommitFile(sessionId, name, ...)`. The grain C++ `dropSessionCommitFile(sessionId, filename, base64)` signature `[VERIFIED: PluginProcessor.h:144]` takes `(sessionId, filename, base64)` — **note this differs from the sampler's `(sessionId, name, midi, vel)`** (the grain commit takes the base64 directly, no midi/vel). The planner must either (a) call the grain native fns directly with a thin custom drop handler, or (b) pass a `commitSingleFile` adapter into a generalized bind. **Recommend (a):** a ~40-line `bindSourceDrop()` in app.js calling the 3 grain native fns directly, reusing only `readFileEntryAsBase64` / `arrayBufferToBase64` from the shared module (which are exported). `[VERIFIED: module exports readFileEntryAsBase64:375, arrayBufferToBase64:390]`
- **`opts.juce` MUST be the `Juce` ES-module namespace**, not `window.__JUCE__` (the latter lacks `getNativeFunction`; calls silently throw and are eaten). `[VERIFIED: module.yaml warning + memory]`
- **Toast-on-false contract:** every native fn returns bool "ok"; `await` it and `showToast(...)` on `false` (or on a thrown error). The fast-path probe + per-iteration try/catch in the module are the reference. `[VERIFIED: module:255,337,360]`
- **`Load…` picker** (always-works fallback): a button → `Juce.getNativeFunction("loadSourceFromFileChooser")()` → C++ opens an async `FileChooser`. No JS streaming needed. `[VERIFIED: PluginProcessor.h:150]`
- **Truncation notice:** after a load, surface `wasLastLoadTruncated()` (10 s cap) as a UI notice. `[VERIFIED: PluginProcessor.h:154]` (Expose via a native fn or piggyback on the commit return — §Open Q4.)

### Preset bar — 8 concept tour buttons (additive pattern)
`[VERIFIED: Additive index.html:162-167 + app.js:399-426]`
```html
<button class="tour-btn" data-preset="Single Grain"       data-tip="lessonSingleGrain">Single Grain</button>
<button class="tour-btn" data-preset="Pitched Buzz"       data-tip="lessonPitchedBuzz">Pitched Buzz</button>
<!-- … Fragments, Smooth Cloud, Frozen Pad, Asynchronous Cloud, Granular Fire, Rect Click -->
```
```js
applyPresetFn = Juce.getNativeFunction("applyFactoryPreset");   // Juce namespace
btn.onclick = () => applyLesson(btn.getAttribute("data-preset"));   // await applyPresetFn(name) + set caption + active class
```
No `preset-manager` module, no JSON, no file dialogs — the snapshot is applied entirely in C++ and the relays sync the page.

---

## Viz Data Contracts

### `GrainCloudFrame` (UI-01 cloud + UI-02 waveform) — `[VERIFIED: dsp/GrainCloudFrame.h]`
Published once per block via `TripleBuffer<GrainCloudFrame>::publish()`; editor `read()`s on the Timer.

**`GrainEvent` (per grain), `kMaxEvents = 256`:**
| Field | Type | Range | Drives |
|-------|------|-------|--------|
| `readPosNorm` | float | [0,1] (grain.readPos / sourceLen) | cloud X **and** waveform playhead X |
| `sizeMs` | float | grain length ms | cloud dot **size** |
| `pitchSemis` | float | (grainPitch+spray) st, relative | cloud **Y** / colour |
| `pan` | float | [0,1] (0.5 = centre) | cloud **lateral** offset |
| `spawnSample` | int | 0..blockSize | cloud **time axis** (intra-block) |

**Frame-level fields:**
| Field | Type | Drives |
|-------|------|--------|
| `count` | int (≤256) | number of valid `events[]` this frame |
| `playheadNorm` | float [0,1] | the **live playhead line** on the waveform |
| `positionNorm` | float [0,1] | resting point — centre of the **shaded spray range** |
| `positionSprayNorm` | float | ± shade **half-width** (fraction of source length) |
| `frozen` | bool | draw the **freeze pin** marker (FUNC-03) |

> Cloud render: scatter `count` dots; X = `readPosNorm` (or `spawnSample` for a time-cloud), Y = `pitchSemis` mapped to vertical, radius ∝ `sizeMs`, lateral nudge from `pan`, sepia fill. Waveform render: static source thumbnail (§Open Q1) + a vertical line at `playheadNorm`, a translucent band `[positionNorm ± positionSprayNorm]`, and a pin glyph when `frozen`.

### `VizRing` → `GrainVizAnalyzer` → scope + spectrum (UI-04) — `[VERIFIED: VizAnalyzer.h]`
- Audio thread: `vizRing.write(post-gain mono, n)` (copy-only, lock-free, `kSize=8192`).
- Message thread (Timer): `vizAnalyzer.process(ring, sampleRate)` →
  - **scope:** 1024 raw samples → 128 max-abs-downsampled points, `[-1,1]` (`getScope()`). Copied **before** the FFT (the FFT clobbers its buffer in place). `[VERIFIED: VizAnalyzer.h:88-100]`
  - **spectrum:** 4096-pt Blackman-Harris FFT → 256 **log-frequency** bins (20 Hz→Nyquist), dB `[-100,0]`, rise-fast/fall-slow smoothing (`getSpectrum()`). `kFftOrder=12` (4096) is chosen to **separate discrete sidebands** — the sync↔async lesson (UI-04, DSP-05). `[VERIFIED: VizAnalyzer.h:70, 103-121]`
- JS renderers: **reuse FM's `drawScope` (:535-558) and `drawSpectrum` (:441-482) verbatim** (same 128/256 sizes, same log-freq axis). Fetch `getSampleRate` for the freq-axis labels (FM `fetchSampleRate` :688-694). Drop FM's FM-specific `drawSidebandMarkers` (carrier/sideband overlay is FM-only); a granular spectrum needs no predicted-marker overlay.

### Grain-count / overlap / CPU readout (UI-05) — `[VERIFIED: PluginProcessor.h:126 getActiveGrainCount; CONTEXT line 67-68]`
- `Grains: N/192` — `getActiveGrainCount()` over `kGlobalGrainCap = 192` `[VERIFIED: PluginProcessor.h:160]`.
- `Overlap: ×Y` — `Y = grainSizeSec × density` where `grainSizeSec = sliderState.grainSize.getScaledValue()/1000`, `density = sliderState.density.getScaledValue()`. Display-only (CONTEXT line 68). Read both from the `Juce` slider states in JS — no extra C++ tap needed.
- **CPU bar** — coarse. Either map `N/192` to a bar (cheap, no new tap) or expose a real CPU estimate via a native fn. **Recommend** the `N/192`-derived coarse bar (CONTEXT says "coarse CPU bar") to avoid a new tap; note as Claude's discretion.

### Window inset (UI-03) — `[VERIFIED: CONTEXT line 70; PluginProcessor.h:163 kWindowLutSize=2048; dsp/WindowLuts.h exists]`
Draw the selected window LUT for one grain; redraw on `windowShape` change. Two options:
1. **Expose the LUT from C++** (a `getWindowLut(shapeIndex)` native fn or a `windowInsetUpdate` emit on combo change) and draw the returned 2048-pt curve (downsample to ~128 for the inset).
2. **Recompute in JS** — the 5 windows (rect/tri/Welch/Gauss/Hann) are closed-form; compute ~128 points on the `windowShape` combo's `valueChanged`. No C++ change.
**Recommend (2)** for the inset (trivial, decouples from C++) unless the DSP LUTs differ subtly from the closed forms — if exactness matters for the teaching point, use (1). Flag for the planner (§Open Q2).

---

## MUST-HOLD Invariants (pitfall checklist)

1. **Member order = relays → WebView → attachments.** Wrong order = release-build crash on plugin reload (C++ destroys in reverse; attachments must die while the WebView is still alive). `[VERIFIED: FM/Additive PluginEditor.h]`
2. **3-arg attachment + `nullptr` undoManager**, with `jassert(param != nullptr)` per ID (ID drift = silently dead control). `[VERIFIED: Additive:131-149]`
3. **Resource provider compares BARE PATHS by direct equality** (`url == "/" || url == "/index.html"` …). Never strip scheme/host. `charset=utf-8` on every text resource. `[VERIFIED + memory]`
4. **`Juce` namespace vs `window.__JUCE__` (RECURRING regression).** Param state + native fns via the `Juce` ES-module namespace (`getSliderState`/`getComboBoxState`/`getToggleState`/`getNativeFunction`). Viz push events via `window.__JUCE__.backend.addEventListener`. **Any shared panel / drop module gets `Juce`, NOT `window.__JUCE__`** — the latter has no `getNativeFunction`; calls silently throw and are eaten by try/catch. `[VERIFIED: module.yaml + MEMORY.md]`
5. **Canvas DPR backing store + `calc()` sizing.** `canvas.width = clientWidth*dpr; ctx.setTransform(dpr,…)`. CSS uses `width/height: calc(100% - Npx)`, **never** `right/bottom` (replaced-element gotcha → stuck at 300×150). `[VERIFIED + memory]`
6. **base64 decode = `juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`.** The latter is JUCE's proprietary `<size>.<altAlphabet>` format and silently rejects standard `btoa()` output. (C++ side already correct per PluginProcessor.h; the invariant is for any new decode path.) `[VERIFIED: module.yaml + memory]`
7. **No audio-thread FFT/alloc.** The audio thread only `vizRing.write(...)` (copy) and fills `GrainCloudFrame` (POD, fixed array, extras dropped — never grown). FFT + scope downsample run on the message-thread Timer. `[VERIFIED: VizAnalyzer.h header + PERF-01]`
8. **Scope copied before FFT.** `performFrequencyOnlyForwardTransform` clobbers its buffer in place; the analyzer already reads the scope window first (VizAnalyzer.h:88). Don't reorder.
9. **Windows WebView2 blank-UI checklist (config-parity, no local Windows build this stage).** `NEEDS_WEBVIEW2 TRUE` **and** `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` **and** `withUserDataFolder(tempDir)`. All three already present in CMakeLists + must be added to the editor's `#if JUCE_WINDOWS` options block. Missing any → silent IE fallback → blank page (no error). `[VERIFIED: CMakeLists.txt confirmed; FM/Additive editor options]` `[MEMORY.md: WebView2 static-linking]`
10. **Drop native-fn names are FIXED** (`dropSessionStart/AddFile/CommitFile/CommitFolder`, `loadSourceFromFileChooser`). Do NOT rename — the C++ side and the shared module both depend on them. `[VERIFIED: PluginProcessor.h:141-150 + CONTEXT line 74-75]`
11. **Unique WebView temp-dir prefix** (`OsimpleGrain_WebView` for the user-data folder; if the drop module's reaper is used, a unique `*-drop-` prefix) so the stale-session reaper can't delete another plugin's in-flight sessions. `[VERIFIED: module.yaml warning]`
12. **Viz event names are the contract** — `scopeUpdate`/`spectrumUpdate`/`grainCloudUpdate`/`grainMeterUpdate`/`windowInsetUpdate` must match exactly between `emitEventIfBrowserIsVisible` (C++) and `backend.addEventListener` (JS). A typo = silently dead viz, no error. `[CONTEXT line 93]`

---

## Common Pitfalls

### Pitfall 1: Slider count off-by-one (16 vs 15)
**What goes wrong:** Treating 18 params as "16 sliders + 2 combos" (the obvious 18−2) and forgetting `freeze` is a toggle → 16-element slider array attaches a non-float param or skips the toggle.
**Avoid:** 18 = **15 sliders + 2 combos + 1 toggle**. Build three separate ID arrays. `jassert` on every attachment.

### Pitfall 2: Reusing FM's `drawSidebandMarkers` for the grain spectrum
**What goes wrong:** FM's spectrum draws predicted `f_c ± k·f_m` markers driven by `carrierUpdate` — there's no carrier in granular; copying it wholesale leaves dead overlay code expecting a `carrierHz`.
**Avoid:** Copy `drawSpectrum`'s bars + freq axis; **drop** `drawSidebandMarkers` and the `carrierUpdate` subscription.

### Pitfall 3: Grain commit signature mismatch
**What goes wrong:** Assuming the sampler's `dropSessionCommitFile(sessionId, name, midi, vel)` — the grain processor's is `dropSessionCommitFile(sessionId, filename, base64)` (no midi/vel; takes the bytes directly).
**Avoid:** Wire a thin custom `bindSourceDrop()` calling the **grain** native fns; reuse only `readFileEntryAsBase64`/`arrayBufferToBase64` from the shared module. `[VERIFIED: PluginProcessor.h:144 vs module:363]`

### Pitfall 4: Window inset drawn every frame
**What goes wrong:** Pushing `windowInsetUpdate` at 30 Hz wastes the bridge and flickers; the window only changes on the `windowShape` combo.
**Avoid:** Emit/recompute the inset **only** on `windowShape` `valueChanged` (+ once at boot).

---

## State of the Art

| Old approach | Current approach | Impact |
|--------------|------------------|--------|
| `juce::WebSliderRelay` low-level wiring per knob | Same — JUCE 8 relay/attachment bridge is the stable, current path | No change; siblings shipped on it |
| `preset-manager` module for all presets | **Read-only concept presets via `applyFactoryPreset` snapshot** | Lighter; no JSON/file dialogs for teaching presets |
| Folder/cell `bindWebViewFileDrop` (sampler) | **Single-source custom bind** reusing the module's base64 primitives | Avoids the folder/modal machinery the grain doesn't need |

**Deprecated/outdated:** none relevant — JUCE 8.0.9 WebView API is current and matches both siblings.

---

## Environment Availability

| Dependency | Required by | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| JUCE WebView (juce_gui_extra) | All UI | ✓ | 8.0.9 | — |
| `webview-drop-streaming` shared module | Drag-drop | ✓ | 1.0.0 (`modules/core/`) | C++ side already in-tree; JS exports available |
| `preset-manager` shared module | (NOT used — additive snapshot pattern chosen) | ✓ | 1.0.0 | n/a |
| WebView2 (Windows) | Windows UI | (config-parity only; no local Win build this stage) | static-linked | — |
| 4 built-in sample binaries | Source default | ✓ | embedded via `juce_add_binary_data` | — |

**No missing dependencies.** All siblings, modules, and Stage-2 taps are present and verified.

---

## Validation Architecture

> Stage 3 verify checks only the GUI goal + a clean build + auval (full battery is Stage 4 — CONTEXT line 111-114). No automated unit-test framework for the WebView UI; verification is the live build + `auval`.

| Property | Value |
|----------|-------|
| Framework | none for UI (manual live-build review per D2) + `auval` static check |
| Quick run | `ninja O-simpleGrain_VST3 O-simpleGrain_AU` then cache-clear/install per CLAUDE.md |
| Phase gate | clean build + `auval -a \| grep -i simplegrain` passes + visual review in standalone/DAW |

**Wave 0 gaps:** None — no test infra required for the GUI stage. The Stage-2 render-harness already gates DSP correctness.

---

## Security Domain

`security_enforcement` is not relevant to this offline audio-plugin GUI stage (no auth, network, or persistence of untrusted input beyond the already-hardened drop module, which ships path-traversal / symlink / size-cap defences in `DropSessionGuard.h`). `[VERIFIED: module.yaml patterns]` No new attack surface introduced by Stage 3. V5 input validation is satisfied by the existing drop-session guards on the C++ side (already in-tree).

---

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|-------|---------|---------------|
| A1 | The 5 window LUTs match their closed-form definitions closely enough that JS-recomputed inset curves are visually faithful | Window inset (UI-03) | Inset slightly mismatches the actual per-grain envelope; mitigate by exposing the real LUT from C++ (Open Q2) |
| A2 | Coarse CPU bar derived from `N/192` is acceptable (no real CPU tap needed) | UI-05 readout | If a true CPU meter is wanted, a new native fn is needed |
| A3 | The on-screen keyboard should be kept for sibling parity | Layout | Minor — can be dropped without consequence |

*All other claims are `[VERIFIED]` against in-tree source or `[CITED]` from module manifests.*

---

## Open Questions for the Planner

1. **Source-waveform thumbnail (UI-02):** The waveform background needs the current source's sample data (or a precomputed min/max envelope). No existing native fn returns it. **Recommend:** add a `getSourceThumbnail()` native fn returning ~512 min/max pairs of the current `currentSource` buffer, emitted on load (and at boot). Low risk — read-only snapshot off the message thread. *Planner: add this task to 3.2.*
2. **Window inset source (UI-03):** Recompute the 5 windows in JS (zero C++ change, A1 risk) **or** expose the real 2048-pt LUT via a native fn / `windowInsetUpdate` emit on `windowShape` change. **Recommend JS recompute** unless the DSP LUTs deviate from closed forms. *Planner: pick one; default = JS.*
3. **Drop-streaming module linkage:** PluginProcessor.h says the C++ drop side is "already implemented," but `CMakeLists.txt` does not yet show `ouaricon_add_module(O-simpleGrain webview-drop-streaming)` or the `WebViewDropStreaming.h` include. Confirm whether the C++ handlers are hand-rolled in PluginProcessor.cpp or pulled from the module; if the latter, the module must be added to CMake in 3.1. **Recommend:** verify in PluginProcessor.cpp during planning; add the module + the JS file to `juce_add_binary_data` if needed. *Planner: confirm at 3.1.*
4. **Truncation notice plumbing:** `wasLastLoadTruncated()` exists but no native fn surfaces it. **Recommend:** either return a richer object from `dropSessionCommitFile` (success + truncated) or add a tiny `getLoadStatus()` native fn polled after load. *Planner: minor; fold into 3.1 load wiring.*

---

## Sources

### Primary (HIGH confidence — in-tree, this session)
- `plugins/O-simpleFM/Source/PluginEditor.{h,cpp}` — relay/attachment/resource-provider/native-fn/timer skeleton; spectrum+scope push
- `plugins/O-simpleAdditive/Source/PluginEditor.{h,cpp}` + `PluginProcessor.cpp:359` — `WebComboBoxRelay` wiring + `applyFactoryPreset` snapshot pattern
- `plugins/O-simpleFM/Source/ui/public/{index.html, js/app.js, modules/preset-manager.js}` + Additive `app.js`/`index.html` — knob/toggle/combo JS, canvas DPR, viz subscriptions, tour buttons
- `plugins/O-simpleGrain/Source/{PluginProcessor.h, VizAnalyzer.h, dsp/GrainCloudFrame.h, dsp/TripleBuffer.h}` — the Stage-2 taps + native-fn surface to consume
- `modules/core/webview-drop-streaming/js/webview-drop-streaming.js` + `module.yaml` — drag-drop bind + base64 primitives + invariants
- `plugins/O-simpleFM/CMakeLists.txt` + `O-simpleGrain/CMakeLists.txt` — binary-data + module + WebView2-flag pattern (grain flags verified present)

### Secondary (MEDIUM)
- `modules/persistence/preset-manager/module.yaml` — confirmed available but NOT used (snapshot pattern chosen)
- `MEMORY.md` / project critical-patterns — Windows WebView2, `Juce` namespace, canvas replaced-element, base64 gotchas

---

## Metadata

**Confidence breakdown:**
- Reuse map / C++ blueprint / web blueprint: **HIGH** — every pattern is a verbatim or near-verbatim adaptation of two shipped siblings + the already-built taps.
- Viz data contracts: **HIGH** — read directly from in-tree headers.
- Window inset + source thumbnail: **MEDIUM** — two viable paths each; recommended defaults flagged as open questions (no blocker).

**Research date:** 2026-06-25
**Valid until:** stable (sibling code + Stage-2 taps are committed; re-verify only if siblings change).
