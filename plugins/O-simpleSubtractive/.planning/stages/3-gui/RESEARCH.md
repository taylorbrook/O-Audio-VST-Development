# Stage 3 (GUI) — RESEARCH

**Plugin:** O-simpleSubtractive
**Stage:** 3 of 4 — GUI (WebView UI + parameter binding + headline visuals)
**Researched:** 2026-06-25
**Domain:** JUCE 8.0.9 WebView UI (relay/attachment binding, message-thread Timer viz, canvas rendering)
**Confidence:** HIGH — every pattern is extracted verbatim from shipped sibling plugins (O-simpleFM primary, O-simpleGrain/Additive secondary). No invented APIs.

---

## Summary

Stage 3 swaps the `GenericAudioProcessorEditor` placeholder for a single-page Ouaricon-Naturalist WebView UI. The binding/Timer/resource-provider machinery is a **direct copy** of `O-simpleFM/Source/PluginEditor.{h,cpp}`, with one addition the FM template lacks: O-simpleSubtractive has **4 choice params** (`oscWave`, `filterType`, `filterSlope`, `voiceMode`) that require the `WebComboBoxRelay`/`WebComboBoxParameterAttachment` pattern — copy that from `O-simpleGrain/Source/PluginEditor.cpp:89-218` and `O-simpleGrain/Source/ui/public/js/app.js:235-271`.

The genuinely new rendering work is three Stage-2-contract-driven visuals: (1) the headline **filter-curve-over-spectrum** — both `getCurve()` and `getSpectrum()` are already 256 log-f bins spanning 20Hz→Nyquist with identical axis math (`SubVizAnalyzer.h:153-181`), so they overlay on one shared-axis canvas with zero remapping; (2) **dual-ADSR** shapes drawn from the 8 ADSR params with playheads driven by `getFilterEnvValue()`/`getAmpEnvValue()`; (3) a **signal-path SVG diagram** (copy the FM `routingSvg` idiom, re-skin osc→filter→VCA).

**Primary recommendation:** Clone the O-simpleFM editor + JS + CSS wholesale, add the combo pattern from O-simpleGrain, then build the three new canvas/SVG renderers against the already-frozen Stage-2 DSP→UI contract. Do NOT touch DSP. Do NOT use a preset-manager module (FM/Grain use `OuariconPresetManager`; Subtractive's preset *content* is Stage 4 — ship only the in-UI tour-button hook calling a `applyFactoryPreset` native fn, exactly like O-simpleGrain).

---

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Follow the O-simpleFM sibling editor pattern exactly:** member order relays → WebBrowserComponent → attachments (relays outlive attachments); 3-arg attachments with `nullptr` undoManager.
- **Resource provider receives bare paths** — direct equality, never strip scheme/host.
- **`type="module"` scripts**; `import * as Juce from './js/juce/index.js'`; pass `Juce` (not `window.__JUCE__`) to anything needing `getNativeFunction`.
- **Editor Timer @ 30 Hz**; copy scope window BEFORE FFT (already handled inside `SubVizAnalyzer::process`).
- **Verify every JS `getNativeFunction(name)` has a matching C++ `withNativeFunction(name,...)`** (silent-bridge-gap pattern).
- **CMake:** `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`; defines `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0` (all **already present** in current CMakeLists.txt). Windows `withUserDataFolder(File::tempDirectory child)`.
- **BinaryData:** ship ONE `juce_add_binary_data` target (UI resources). A 2nd target (only if embedded presets are ever added) needs a distinct `NAMESPACE`.
- **Do NOT touch Stage-2 DSP.** Audio thread stays copy-only (PERF-01).

### Claude's Discretion
- Exact CSS skin / color choices (Ouaricon-Naturalist family; reuse FM's `styles.css` as the base).
- Signal-path diagram: SVG vs canvas (this doc recommends **SVG**, see 3.3).
- Tooltip copy wording (must be plain-language pedagogy per BRIEF).
- Canvas layout proportions for the headline visual.

### Deferred Ideas (OUT OF SCOPE)
- Preset **content** (the 8 concept presets, FUNC-06) — Stage 4. Stage 3 ships only the selectable UI hook.
- Persistent save/load preset manager (FM uses `OuariconPresetManager`; Subtractive does NOT need it for Stage 3).

---

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| UI-01 | Live filter-response-over-spectrum (headline) | `getCurve()`+`getSpectrum()` both 256 log-f bins, same axis math → overlay on one canvas (§3.2) |
| UI-02 | Animated dual-ADSR display (filter→cutoff, amp→level, independent scales) | 8 ADSR params + `getFilterEnvValue()`/`getAmpEnvValue()` playheads via `envUpdate` event (§3.2) |
| UI-03 | Live signal-path diagram, active stage highlighted | SVG idiom from FM `routingSvg` (`index.html:48-71`), re-skinned osc→filter→VCA (§3.3) |
| UI-04 | Oscilloscope morphing with cutoff/res/env | `getScope()` 128 pts → `drawScope` copied verbatim from FM `app.js:535-558` (§3.2) |
| UI-05 | On-hover pedagogical tooltips on every control | `TIPS` const-map + `setupTooltips()` from FM `app.js:38-65,238-285` (§3.3) |
| UI-06 | Single-page projector-readable left→right layout | FM `index.html` frame/panel/group structure (§3.1) |
| UI-07 | Preset tour selectable from UI (hook now) | tour-button + `applyFactoryPreset` native fn from O-simpleGrain `app.js`/`PluginEditor.cpp:169-173` (§3.3) |
| COMPAT-02 | Windows WebView2 configured, renders on Windows | CMake flags present; `withWinWebView2Options`+`withUserDataFolder` from FM `PluginEditor.cpp:191-199` (§3.1) |
| QUAL-02 | Filter curve + spectrum accurately reflect what's heard | `SubFilterCurve::magnitudeDb` = closed-form of running SVF (QUAL-02 by construction, `SubVizAnalyzer.h:71-110`) |

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Parameter two-way binding | WebView relay/attachment (C++) | JS slider/combo/toggle state | APVTS is source of truth; relays bridge to JS DOM |
| FFT / scope / filter-curve compute | Message-thread Timer (C++) | — | PERF-01: audio thread copy-only; all DSP-display math off audio thread |
| Spectrum/scope/curve/env rendering | JS canvas (browser) | — | C++ emits float arrays; JS draws |
| On-screen keyboard → notes | JS → `uiMidi` native fn → `MidiMessageCollector` (C++) | — | identical path to host MIDI |
| Signal-path diagram | JS + SVG (browser) | reads JS param state | static structure, dynamic highlight from param values |
| Tooltips / preset tour | JS (browser) | `applyFactoryPreset` native fn (C++, Stage 4 content) | pedagogy layer; preset content deferred |

---

## Standard Stack

No new external packages. Everything ships with JUCE 8.0.9 (local `/Users/taylorbrook/JUCE`) and the existing module set already linked in `CMakeLists.txt`.

| Component | Source | Purpose |
|-----------|--------|---------|
| `juce::WebBrowserComponent` + `Options` | `juce_gui_extra` (already linked) | hosts the page, registers relays + native fns |
| `WebSliderRelay` / `WebSliderParameterAttachment` | `juce_gui_extra` | 13 continuous knobs |
| `WebComboBoxRelay` / `WebComboBoxParameterAttachment` | `juce_gui_extra` | 4 choice params — **NEW vs FM** |
| `juce::dsp::FFT` / `WindowingFunction` | `juce_dsp` (already linked) | inside `SubVizAnalyzer` (already built) |
| `juce/index.js` + `check_native_interop.js` | copy verbatim from FM | JS-side `Juce.*` ES module |

**No `juce_add_binary_data` target exists yet** — Stage 3 adds the first one. No package legitimacy audit needed (zero external deps).

---

## Parameter → control-type inventory (drives relay arrays)

13 sliders, 4 combos, 0 toggles. (FM had toggles; Subtractive has none — drop the toggle arrays.)

```cpp
const juce::StringArray sliderIds {
    subLevel, noiseLevel,                                  // OSC/MIX
    cutoff, resonance, filterEnvAmount, keyTrack,          // FILTER
    filterAttack, filterDecay, filterSustain, filterRelease,  // FILTER ADSR
    ampAttack, ampDecay, ampSustain, ampRelease,          // AMP ADSR  (14? see note)
    glide, outputLevel                                     // VOICE/OUT
};
const juce::StringArray comboIds { oscWave, filterType, filterSlope, voiceMode };
```

> **COUNT NOTE for the gui-agent:** the 20 params = 4 combos + 16 sliders. The slider list above is the 16: `subLevel, noiseLevel, cutoff, resonance, filterEnvAmount, keyTrack, filterAttack, filterDecay, filterSustain, filterRelease, ampAttack, ampDecay, ampSustain, ampRelease, glide, outputLevel`. Verify with `OSimpleSubtractive::ParamIDs` in `PluginProcessor.h:28-61` — any drift trips the `jassert(param != nullptr)`.

---

## Native function registry (JS `getNativeFunction` ↔ C++ `withNativeFunction`)

Every row MUST exist on both sides or the control silently dies (project memory: `pattern_webview_native_fn_bridge_gap`). Grep-diff after wiring: `grep getNativeFunction app.js` vs `grep withNativeFunction PluginEditor.cpp`.

| JS name | C++ handler | Purpose | Source pattern |
|---------|-------------|---------|----------------|
| `uiMidi` | `processorRef.handleUiMidi((int)a[0],(bool)a[1], a.size()>=3?(float)a[2]:0.8f)` | on-screen keyboard note on/off → `MidiMessageCollector` | FM `PluginEditor.cpp:180-185` |
| `getSampleRate` | `complete(processorRef.getCurrentSampleRate())` | spectrum/curve x-axis Nyquist label | FM `PluginEditor.cpp:187-189` |
| `applyFactoryPreset` | `if(args.size()>0) processorRef.applyFactoryPreset(args[0].toString()); complete(true)` | preset-tour hook (UI-07); **content is Stage 4** | Grain `PluginEditor.cpp:169-173` |

> **`applyFactoryPreset` for Stage 3:** the C++ method may be a stub that does nothing yet (or applies a minimal demo), but the native fn MUST be registered so the JS tour buttons are live and the bridge is wired. Stage 4 fills the body. Confirm whether `OSimpleSubtractiveAudioProcessor` already declares `applyFactoryPreset` — if not, add a no-op/minimal declaration in the processor (this is wiring, permitted; it does not alter DSP). `[ASSUMED]` the processor does not yet declare it (placeholder editor confirms Stage 1 state).

No preset-manager module, no file-chooser native fns (those are FM/Grain-only; out of scope).

---

## Emitted events (C++ `emitEventIfBrowserIsVisible` ↔ JS `backend.addEventListener`)

Events arrive on `window.__JUCE__.backend.addEventListener`, NOT `Juce.*`. Order matters: emit the lead-voice/scalar context BEFORE the array it annotates (FM emits `carrierUpdate` before `spectrumUpdate`, `app.js:256-258`).

| Event name | Payload shape | JS canvas consumer | Notes |
|------------|---------------|--------------------|-------|
| `spectrumUpdate` | `var` array, 256 floats dB ~[-100,0] | spectrum bars on headline canvas | `vizAnalyzer.getSpectrum()` |
| `filterCurveUpdate` | `var` array, 256 floats dB (curve bins) | filter curve line **over** spectrum, same canvas | `vizAnalyzer.getCurve()`; call `updateCurve(getDisplayCutoffHz, getDisplayK, getDisplayType, getDisplaySlope, sr)` first |
| `scopeUpdate` | `var` array, 128 floats [-1,1] | oscilloscope canvas | `getScope()`; verbatim FM `drawScope` |
| `envUpdate` | `DynamicObject { filterEnv: float, ampEnv: float }` (both 0..1) | dual-ADSR playheads | `getFilterEnvValue()`/`getAmpEnvValue()` — use the `DynamicObject` packaging idiom from Additive `PluginEditor.cpp:183-186` |

> **Single-canvas overlay (UI-01):** `spectrumUpdate` and `filterCurveUpdate` both feed the SAME canvas. Store both arrays in JS module-scope (`lastSpectrum`, `lastCurve`) and have ONE `drawHeadline()` that draws spectrum bars then strokes the curve line on top. Both arrays are 256 bins, both log-f 20Hz→Nyquist, both dB — identical x and y mapping (see §3.2 axis proof). Optionally fold all four into one combined `vizUpdate` `DynamicObject` to guarantee same-frame coherence; either is acceptable — the separate-events approach matches FM most closely.

---

## RESEARCH by ROADMAP sub-phase

### 3.1 — Layout + Basic Controls + Cross-Platform Wiring (UI-05/06, COMPAT-02)

**Goal:** single-page left→right signal-path layout, all 20 controls two-way bound, cross-platform correct.

**Files to create (copy + adapt from O-simpleFM):**
- `Source/ui/public/index.html` ← FM `index.html` (212 lines). Reuse frame/header/panel/group/knob-cell idiom. Replace control groups with the 5 Subtractive panels (OSC | FILTER | FILTER ADSR | AMP ADSR | VOICE/OUT). Add `<select class="combo" id="combo-oscWave">` etc. for the 4 choice params (Grain's combo markup is the model).
- `Source/ui/public/css/styles.css` ← FM `styles.css` (618 lines) verbatim as the base skin; tweak panel widths for 5 columns.
- `Source/ui/public/js/app.js` ← FM `app.js` skeleton (boot/knob/tooltip/keyboard/canvas) + Grain combo binding.
- `Source/ui/public/js/juce/index.js` and `check_native_interop.js` ← **copy verbatim** from `O-simpleFM/Source/ui/public/js/juce/` (17959 + 4376 bytes). Do NOT edit.

**Editor C++ (`PluginEditor.{h,cpp}`):** Replace the `GenericAudioProcessorEditor` body.

Member declaration order (CRITICAL — C++ destroys in reverse; FM `PluginEditor.h:39-56`):
```cpp
// 1. RELAYS (declared first → destroyed last)
std::vector<std::unique_ptr<juce::WebSliderRelay>>   sliderRelays;
std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;   // NEW vs FM
// 2. WEBVIEW
std::unique_ptr<juce::WebBrowserComponent> webView;
// 3. ATTACHMENTS (declared last → destroyed first, while WebView alive)
std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>   sliderAttachments;
std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;
```
Also: `private juce::Timer` base, `SubVizAnalyzer vizAnalyzer;` member, `getResource()` method, `timerCallback()`. (FM `PluginEditor.h:18-37`.)

Relay construction + 3-arg attachments (verbatim shape, FM `PluginEditor.cpp:79-94, 203-222`; combos from Grain `PluginEditor.cpp:95-96, 210-218`):
```cpp
for (const auto& id : sliderIds) sliderRelays.push_back(std::make_unique<juce::WebSliderRelay>(id));
for (const auto& id : comboIds)  comboRelays .push_back(std::make_unique<juce::WebComboBoxRelay>(id));
// ... options.withOptionsFrom(*relay) for each ...
// attachments AFTER webView constructed:
sliderAttachments.push_back(std::make_unique<juce::WebSliderParameterAttachment>(*param, *sliderRelays[i], nullptr));
comboAttachments .push_back(std::make_unique<juce::WebComboBoxParameterAttachment>(*param, *comboRelays[i], nullptr));
```

`WebBrowserComponent::Options` chain (FM `PluginEditor.cpp:86-89`):
```cpp
auto options = juce::WebBrowserComponent::Options{}
    .withNativeIntegrationEnabled()
    .withKeepPageLoadedWhenBrowserIsHidden()
    .withResourceProvider([this](const auto& url){ return getResource(url); });
// then .withOptionsFrom(*relay) for all relays, then .withNativeFunction(...) chain,
// then #if JUCE_WINDOWS withWinWebView2Options(...).withUserDataFolder(tempDir child)
webView = std::make_unique<juce::WebBrowserComponent>(options);
addAndMakeVisible(*webView);
webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
startTimerHz(30);
```

Resource provider (FM `PluginEditor.cpp:19-61`): bare-path direct equality; `makeBinaryResource` helper; MIME `text/html; charset=utf-8`, `text/css; charset=utf-8`, `application/javascript; charset=utf-8`. Map paths: `/`, `/index.html`, `/css/styles.css`, `/js/app.js`, `/js/juce/index.js`, `/js/juce/check_native_interop.js`. (No `/modules/...`, no `/img/...` unless a botanical overlay PNG is added.)

Windows block (FM `PluginEditor.cpp:191-199`): `withUserDataFolder(File::tempDirectory.getChildFile("OsimpleSubtractive_WebView"))` + `.withStatusBarDisabled().withBuiltInErrorPageDisabled()`.

JS combo binding (Grain `app.js:235-271`) — handles choices-not-ready-on-first-load via listening to both `propertiesChangedEvent` and `valueChangedEvent`:
```js
function bindCombo(id){
  const st = Juce.getComboBoxState(id); comboState[id]=st;
  const sel = document.getElementById(`combo-${id}`);
  const buildOptions = () => { const choices=(st.properties&&st.properties.choices)||[]; /* build <option>s */ };
  const refresh = () => { buildOptions(); const idx=st.getChoiceIndex(); if(idx>=0) sel.selectedIndex=idx; };
  st.propertiesChangedEvent.addListener(refresh); st.valueChangedEvent.addListener(refresh); refresh();
  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
}
```
JS knob binding (relative vertical drag) verbatim from FM `app.js:67-162` (`KNOB_MIN_DEG=-135`, `KNOB_MAX_DEG=135`, `DRAG_TRAVEL_PX=220`; `sliderDragStarted/setNormalisedValue/sliderDragEnded`; wheel + arrow-key nudge).

On-screen keyboard verbatim from FM `app.js:587-685` + `uiMidi` native fn. (UI-06 layout puts the keyboard at the bottom.)

**CMake deltas** (current `CMakeLists.txt` already has all WebView flags + module links). Add ONLY:
1. A `juce_add_binary_data` target before `target_link_libraries` (model FM `CMakeLists.txt:50-59`, drop preset-manager/img lines):
```cmake
juce_add_binary_data(O-simpleSubtractive_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)
```
2. Add `O-simpleSubtractive_UIResources` to the `PRIVATE` block of `target_link_libraries` (FM `CMakeLists.txt:64`).
3. Do NOT call `ouaricon_add_module(... preset-manager)` (FM-only; out of scope).
4. Single binary-data target → default `BinaryData` namespace is fine. Only a 2nd target would need a distinct `NAMESPACE` (O-simpleGrain Stage 3.1 collision lesson).

**Pitfalls applying to 3.1:**
- Member-order crash (relays must outlive attachments).
- Bare-path resource provider (no scheme stripping).
- `Juce` namespace vs `window.__JUCE__` (getNativeFunction is on the ES module).
- Combo choices may be empty on first load — must listen to `propertiesChangedEvent`.
- Windows: missing `withUserDataFolder` → silent IE fallback → blank UI.
- Module-load `ReferenceError` in app.js fails silently to build/auval but kills the whole WebView — verify all helper refs after editing (project memory: `feedback_module_extraction_regression_check`).

---

### 3.2 — Headline Filter-Curve-Over-Spectrum + Oscilloscope + Dual-ADSR (UI-01/02/04, PERF-01, QUAL-02)

**Goal:** the teaching visuals, driven by the editor Timer @ 30 Hz.

**Timer callback (`timerCallback()`)** — model FM `PluginEditor.cpp:237-259` + Additive `DynamicObject` packaging `PluginEditor.cpp:183-186`:
```cpp
void timerCallback() {
    vizAnalyzer.process(processorRef.getVizRing(), processorRef.getCurrentSampleRate());   // scope+FFT (scope copied before FFT inside process())
    vizAnalyzer.updateCurve(processorRef.getDisplayCutoffHz(), processorRef.getDisplayK(),
                            processorRef.getDisplayType(), processorRef.getDisplaySlope(),
                            processorRef.getCurrentSampleRate());                            // closed-form curve = running filter (QUAL-02)
    if (webView == nullptr) return;
    // pack getSpectrum() (256), getCurve() (256), getScope() (128) into var arrays
    // pack env DynamicObject { filterEnv, ampEnv }
    webView->emitEventIfBrowserIsVisible("filterCurveUpdate", juce::var(std::move(curveArr)));  // curve before/with spectrum
    webView->emitEventIfBrowserIsVisible("spectrumUpdate",    juce::var(std::move(specArr)));
    webView->emitEventIfBrowserIsVisible("scopeUpdate",       juce::var(std::move(scopeArr)));
    webView->emitEventIfBrowserIsVisible("envUpdate",         juce::var(envObj));
}
```

**UI-01 axis proof (the headline alignment):** In `SubVizAnalyzer.h`, spectrum bins (`:153-167`) and curve bins (`:173-182`) use the **identical** mapping: `frac = b/(N-1)`, `hz = 20 * pow((sr*0.5)/20, frac)`, with `N=256` for both. **Therefore curve bin `b` and spectrum bin `b` are the same frequency** — JS plots both at `x = (b/255) * width`, no per-array remapping. Both are dB:
- spectrum dB floor −100 (`Decibels::gainToDecibels(mag+1e-9f, -100.0f)`).
- curve dB floor −120 (`magnitudeDb` returns `gainToDecibels(mag+1e-9, -120.0)`), peaks can exceed 0 dB at resonance/self-osc.

JS y-mapping: use a shared dB window, e.g. `[-90, +18]` so the resonance peak stays on-canvas. `y = h - ((db - DB_MIN)/(DB_MAX - DB_MIN)) * h`, clamp 0..h. Draw spectrum as filled bars (FM `drawSpectrum` `app.js:441-482`, minus the FM sideband markers), then stroke the curve as a 2px line on top. Reuse FM's log-freq tick labels (`app.js:468-479`, ticks `[100,1000,10000]`, `getSampleRate` → `nyquistHz`).

> **Teaching payoff (BRIEF):** lower cutoff → spectrum bars above the curve's knee visibly drop under the curve; raise resonance → curve peak grows at cutoff then self-oscillates (curve peak + matching spectrum spike); steepen slope (combo 6/12/24) → curve knee steepens (24dB squares the magnitude, `SubVizAnalyzer.h:105`); route filter env → curve's cutoff knee slides independently of the amp envelope.

**UI-04 oscilloscope:** copy FM `drawScope` verbatim (`app.js:535-558`) — 128 pts, center line, 2px stroke. `getScope()` is the post-filter output, so it morphs with cutoff/res/env automatically.

**UI-02 dual-ADSR:** NEW renderer (no sibling has it; design here).
- Draw two stacked ADSR shapes (filter env on top, amp env below) on one canvas, OR two small canvases.
- Each shape is computed in JS from the 4 relevant `sliderState` scaled values (A,D,S,R seconds + sustain 0..1). Map A+D+R to an x-budget with a fixed sustain plateau width; y = level 0..1. Redraw on any of the 8 ADSR `valueChangedEvent`s.
- **Playheads:** the `envUpdate` event pushes `{filterEnv, ampEnv}` (current envelope output 0..1, from `getFilterEnvValue()`/`getAmpEnvValue()`). Draw a moving dot/vertical marker whose **y = the live env value** on each shape (the x can advance with note phase if tracked, but y-on-curve is the minimum viable "independent scales" cue: filter env moves brightness, amp env moves level).
- Independent scales (CONTEXT): label the filter-env axis "→ cutoff" and amp-env axis "→ level" so students see they're separate routings.

**Canvas DPR gotcha (project memory + FM `makeCanvas` `app.js:413-426`):** `<canvas>` is a CSS replaced element — `position:absolute` with left+right does NOT stretch it. Use explicit `width/height` via `clientWidth*dpr` backing store + `ctx.setTransform(dpr,0,0,dpr,0,0)`. Copy `makeCanvas` verbatim and the single `rewireResize()` handler (`app.js:578-585`) that re-fits + redraws last frame on editor resize.

**Viz event listeners (JS):** copy FM `setupVizEvents` (`app.js:560-574`) — `window.__JUCE__.backend.addEventListener("spectrumUpdate"/...)`. Add `filterCurveUpdate` and `envUpdate` listeners storing into `lastCurve`/`lastEnv` and calling the headline/ADSR redraws.

**Pitfalls applying to 3.2:**
- Audio-thread purity (PERF-01): all FFT/curve/scope is in `SubVizAnalyzer` on the Timer; never call it from `processBlock`. Already enforced by Stage 2 design — just don't add audio-thread work.
- Scope-before-FFT ordering: already handled inside `SubVizAnalyzer::process` (`:135-151`); do not reorder.
- dB window clipping: resonance peak exceeds 0 dB — pick `DB_MAX ≥ +18` or the peak flat-tops.
- Canvas replaced-element sizing + DPR (above).
- `envUpdate` `DynamicObject` decoding in JS: it arrives as a JS object `{filterEnv, ampEnv}` (Additive uses this shape for `{sounding, levels}`).

---

### 3.3 — Signal-Path Diagram + Tooltips + Preset Tour Hook (UI-03/07, FUNC-06)

**Goal:** pedagogical scaffolding; preset *content* lands in Stage 4.

**UI-03 signal-path diagram — recommend SVG** (matches FM `routingSvg`, `index.html:48-71`; styled in CSS; dynamic via `element.style` from JS, FM `updateRouting` `app.js:187-236`). SVG over canvas because: crisp at any projector resolution, no DPR backing-store management, declarative nodes, trivial active-stage highlight via class toggles.
- Structure: `OSC → FILTER → VCA → ♪`, with two envelope-route arrows (FILTER ADSR → filter cutoff; AMP ADSR → VCA). Copy FM's `<line>`/`<polygon>` arrow + `<circle>`/`<text>` node idiom.
- Dynamic highlight (model FM `updateRouting`): 
  - OSC node label/shape reflects `oscWave` combo choice (Saw/Square/Triangle/Sine).
  - FILTER node reflects `filterType` (LP/HP/BP/Notch) + `filterSlope` (pole count) — e.g. text "24 dB LP".
  - filter-env arrow opacity/width ∝ `filterEnvAmount` magnitude (bipolar — color the sign).
  - VCA node pulses with live `ampEnv` (from `envUpdate`).
- Hook the relevant `sliderState`/`comboState` `valueChangedEvent`s to an `updateDiagram()` (pattern: FM `boot()` `app.js:704-709`).

**UI-05 tooltips:** `TIPS` const-map + `setupTooltips()` verbatim from FM `app.js:38-65, 238-285`. Every `[data-tip]` element gets pointerenter/move/leave + focusin/out (keyboard accessibility) + Escape-to-hide. Write plain-language pedagogy copy for all 20 params plus the diagram/readout — per BRIEF, explain what cutoff does, why resonance whistles (self-oscillation), what "poles"/slope mean, filter-env vs amp-env. (Subtractive needs ~24 tip entries; FM's 20-odd entries are the tone/length model.)

**UI-07 preset tour hook:** copy O-simpleGrain's tour pattern (NOT FM's `OuariconPresetManager` — that's persistent JSON presets, out of scope).
- HTML: a `.preset-tour` section with `<button class="tour-btn" data-preset="...">` per concept (FM `index.html:185-196`).
- JS: `setupPresets()` wiring each button to call the `applyFactoryPreset` native fn with the button's label (Grain `app.js` `applyFactoryPreset` fn ref; Grain `PluginEditor.cpp:169-173`). Loading a preset calls `setValueNotifyingHost` on every param C++-side, and the relays/attachments sync all knobs/combos back to the page automatically — no DOM poking.
- Stage 3 ships the buttons + the wired native fn; the C++ `applyFactoryPreset` body (the 8 concept snapshots) is **Stage 4 (FUNC-06)**. A Stage-3 stub that applies nothing (or one demo preset) is acceptable as long as the bridge is live.

**Pitfalls applying to 3.3:**
- Silent native-fn bridge gap: `applyFactoryPreset` + `uiMidi` + `getSampleRate` must each have matching C++ `withNativeFunction`. Grep-diff before declaring done.
- Tooltip `data-tip` keys must match `TIPS` map keys exactly (a missing key = silent no-tip).
- SVG element IDs referenced by `updateDiagram()` must exist in `index.html` (a typo'd `getElementById` returns null → silent dead highlight; FM guards with `if (el)`).
- `applyFactoryPreset` calling `setValueNotifyingHost` is the ONLY sanctioned way to move params from C++; do not write APVTS values directly without host notification (breaks UI sync + automation).

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Param ↔ UI sync | Manual postMessage + value marshalling | `Web*Relay` + `Web*ParameterAttachment` | handles host automation, undo, normalisation, thread hops |
| Filter magnitude curve | New JS biquad math | `SubVizAnalyzer::updateCurve` → `getCurve()` | already the closed-form of the running SVF (QUAL-02 by construction) |
| Log-freq axis mapping | New JS frequency loop | spectrum & curve are pre-mapped 256 log-f bins | bin index == frequency for both arrays |
| FFT / windowing | JS FFT lib | `SubVizAnalyzer` (juce::dsp::FFT, message thread) | already built, RT-safe, off audio thread |
| On-screen MIDI | Custom event queue | `uiMidi` native fn → `MidiMessageCollector` | identical path to host MIDI, thread-safe |
| Canvas crisp sizing | left/right absolute positioning | explicit `clientWidth*dpr` + `setTransform` | canvas is a CSS replaced element (won't stretch) |
| JS↔C++ ES module | hand-rolled bridge | copy `js/juce/index.js` + `check_native_interop.js` verbatim | JUCE-shipped, version-matched to 8.0.9 |

**Key insight:** Stage 2 deliberately exposed a complete, RT-safe display contract (`getCurve/getSpectrum/getScope/getDisplay*/getFilterEnvValue/getAmpEnvValue`). The entire GUI is a *consumer* of that contract — no DSP, no new audio-thread code, no curve math.

---

## Common Pitfalls

### Pitfall 1: `Juce` namespace vs `window.__JUCE__`
**What goes wrong:** calling `window.__JUCE__.getNativeFunction(...)` throws `TypeError` (no such method); the control renders but is silently dead.
**Avoid:** `getSliderState/getComboBoxState/getToggleState/getNativeFunction` are on the `Juce` ES-module namespace; `window.__JUCE__.backend.addEventListener` is for emitted events only. (Project memory: critical_juce_webview_namespace_vs_postmessage.)

### Pitfall 2: Resource provider scheme stripping
**What goes wrong:** `url.fromFirstOccurrenceOf("://")` on a bare path returns "" → all lookups fail → "Frame load interrupted" / blank page.
**Avoid:** compare bare paths by direct equality (`url == "/js/app.js"`). FM `PluginEditor.cpp:32-61`.

### Pitfall 3: Member-order destruction crash
**What goes wrong:** attachments declared before relays → relay destroyed first → attachment dangles → release-build crash on plugin reload.
**Avoid:** relays → webView → attachments declaration order. FM `PluginEditor.h:39-56`.

### Pitfall 4: Silent native-fn bridge gap
**What goes wrong:** JS calls `getNativeFunction("foo")` with no matching C++ `withNativeFunction("foo")` → passes build/auval/render-harness but the control is dead.
**Avoid:** grep-diff the two lists; gate `handleUiMidi` with an empty host MIDI buffer (already done in DSP). Project memory: pattern_webview_native_fn_bridge_gap.

### Pitfall 5: Canvas replaced-element sizing + DPR
**What goes wrong:** canvas stuck at 300×150 default / blurry on Retina.
**Avoid:** `makeCanvas` DPR backing store (FM `app.js:413-426`); explicit width/height calc, not left+right absolute.

### Pitfall 6: Windows WebView2 silent IE fallback
**What goes wrong:** WebView2 denied default user-data dir in DAW host → silent fallback to IE → no resource provider → blank page.
**Avoid:** `withUserDataFolder(File::tempDirectory child)` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (already in CMake). FM `PluginEditor.cpp:191-199`.

### Pitfall 7: Module-load ReferenceError kills the whole UI
**What goes wrong:** any undefined symbol at app.js module-eval time aborts the entire script → blank UI, but build/auval/render-harness pass.
**Avoid:** after editing app.js, verify every helper reference resolves; test in a real DAW. Project memory: feedback_module_extraction_regression_check.

### Pitfall 8: BinaryData namespace collision (only if 2nd target added)
**What goes wrong:** a 2nd `juce_add_binary_data` with default namespace → duplicate-symbol link fail.
**Avoid:** v1.0 ships ONE target. If embedded presets ever add a 2nd, give it a distinct `NAMESPACE`. O-simpleGrain Stage 3.1 lesson.

---

## Code Examples (verified from siblings)

### Combo binding C++ (Grain `PluginEditor.cpp:95-96, 210-218`)
```cpp
for (const auto& id : comboIds)
    comboRelays.push_back(std::make_unique<juce::WebComboBoxRelay>(id));
// ... after webView:
comboAttachments.push_back(std::make_unique<juce::WebComboBoxParameterAttachment>(
    *param, *comboRelays[(size_t)i], nullptr));
```

### Env DynamicObject emit C++ (model: Additive `PluginEditor.cpp:183-186`)
```cpp
auto* envObj = new juce::DynamicObject();
envObj->setProperty("filterEnv", processorRef.getFilterEnvValue());
envObj->setProperty("ampEnv",    processorRef.getAmpEnvValue());
webView->emitEventIfBrowserIsVisible("envUpdate", juce::var(envObj));
```

### Combo binding JS (Grain `app.js:235-271`)
```js
const st = Juce.getComboBoxState(id);
const choices = (st.properties && st.properties.choices) || [];
// build <option> per choice; st.getChoiceIndex() to set selectedIndex
sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
st.propertiesChangedEvent.addListener(refresh);  // choices may arrive late
```

### Headline overlay JS (adapt FM `drawSpectrum` `app.js:441-482`)
```js
function drawHeadline() {
  // spectrum bars: x=(i/255)*w, y from dB window [-90,+18]
  // then stroke curve line on top using the SAME x map (256 bins) and same dB window
}
```

---

## Runtime State Inventory

Greenfield GUI stage (new files only). Not a rename/refactor. No stored data, live service config, OS-registered state, secrets, or stale build artifacts to migrate.
- **Stored data:** None — UI reads APVTS live; no new persisted state (preset content is Stage 4).
- **Live service config:** None.
- **OS-registered state:** None (plugin cache clear on install is the standard CLAUDE.md build step, not a state migration).
- **Secrets/env vars:** None.
- **Build artifacts:** First `juce_add_binary_data` target — a fresh CMake configure regenerates `BinaryData.h`. No stale artifacts (no prior binary-data target existed).

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| JUCE | entire build | ✓ | 8.0.9 (`/Users/taylorbrook/JUCE`) | — |
| CMake + Ninja | build | ✓ | per project | — |
| `juce_gui_extra` (WebBrowserComponent) | WebView | ✓ | linked in CMakeLists.txt | — |
| WebView2 runtime (Windows) | COMPAT-02 | n/a on macOS dev | — | static-linking define + tempdir userdata |
| `js/juce/*.js` helpers | JS binding | ✓ (copy from FM) | matches 8.0.9 | — |

No blocking gaps on macOS. Windows is config-only for Stage 3 (cannot test on this machine; correctness ensured by CMake flags + sibling parity).

---

## Validation Architecture

Stage 3 is GUI; the Stage-2 offline render-harness (`tests/render-harness`) remains the DSP correctness gate and is untouched. GUI validation is manual-in-DAW (project pattern) plus `pluginval`/`auval`.

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Command / Method |
|--------|----------|-----------|------------------|
| UI-06 | layout renders, all 20 bound | manual-DAW | open in Logic/Reaper, drag each control |
| UI-01/QUAL-02 | curve over spectrum, matches audio | manual-DAW + harness curve probe | sweep cutoff; harness `magnitudeDb` vs measured (Stage-2 gate already validates curve==filter) |
| UI-02/04 | scope + dual-ADSR animate | manual-DAW | play a note, observe |
| COMPAT-02 | builds + auval/pluginval pass | automated | `auval -a \| grep -i subtractive`; `pluginval` |
| all | no audio-thread alloc/FFT | manual + design | viz code is Timer-only by construction |

### Phase gate
`ninja O-simpleSubtractive_VST3 O-simpleSubtractive_AU` clean; `auval` passes; `pluginval` passes; visual UAT in DAW. (No new unit-test framework needed for Stage 3.)

---

## Security Domain

`security_enforcement` not applicable in the web-threat sense — this is a locally-embedded WebView serving only BinaryData resources (no network, `JUCE_USE_CURL=0`), no user-supplied URLs, no remote content. The only "input" is APVTS values (host-validated) and on-screen-keyboard note ints (bounded `0..127` in `handleUiMidi`, already validated in DSP). No ASVS categories apply beyond V5 input validation, which the DSP layer already enforces.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `OSimpleSubtractiveAudioProcessor` does not yet declare `applyFactoryPreset` | Native fn registry / 3.3 | Low — gui-agent adds a wiring-only stub; if it exists, just register the native fn. Verify in `PluginProcessor.h`. |
| A2 | The 16 slider IDs + 4 combo IDs partition the 20 params exactly | Param inventory | Low — `jassert(param != nullptr)` catches drift at debug runtime; cross-check `ParamIDs` |
| A3 | A `+18 dB` upper dB window keeps the self-oscillation peak on-canvas | 3.2 | Low — cosmetic; adjust window if peak flat-tops |
| A4 | Stage 3 needs no botanical-overlay PNG (FM ships `insects.png`) | 3.1 | None — purely aesthetic discretion |

**All other claims are VERIFIED against sibling source (file:line cited) or CITED from the Stage-2 contract files.**

## Open Questions

1. **Dual-ADSR playhead x-advance** — should the playhead advance along the ADSR time axis (requires note-phase tracking) or only move vertically to the live env value?
   - What we know: `getFilterEnvValue()`/`getAmpEnvValue()` give the live 0..1 output; no phase/time index is exposed.
   - Recommendation: vertical-only marker for Stage 3 (sufficient for "independent scales" pedagogy); a time-advancing playhead would need a new atomic from DSP (out of scope — don't touch DSP).

2. **Combined `vizUpdate` vs four separate events** — single `DynamicObject` guarantees same-frame coherence between spectrum and curve.
   - Recommendation: separate events (matches FM most closely); fold into one only if visible spectrum/curve tearing appears at 30 Hz.

---

## Sources

### Primary (HIGH confidence — shipped sibling source, read this session)
- `O-simpleFM/Source/PluginEditor.{h,cpp}` — relay/attachment/Timer/resource-provider/native-fn/Windows pattern (cited by line throughout).
- `O-simpleFM/Source/ui/public/{index.html,js/app.js}` — layout, knob/tooltip/keyboard/canvas/viz-event JS.
- `O-simpleFM/CMakeLists.txt:50-96` — binary-data target + flags.
- `O-simpleGrain/Source/PluginEditor.cpp:89-218` + `ui/public/js/app.js:235-271` — ComboBox relay/attachment + JS combo binding + `applyFactoryPreset` tour hook.
- `O-simpleAdditive/Source/PluginEditor.cpp:165-194` — `DynamicObject` emit packaging (model for `envUpdate`).
- `O-simpleSubtractive/Source/PluginProcessor.h` + `SubVizAnalyzer.h` — the frozen Stage-2 DSP→UI contract.

### Secondary (project memory / CLAUDE.md — HIGH)
- MEMORY.md: namespace-vs-postmessage, bare-path resource provider, canvas DPR, Windows WebView2 userdata/static-linking/IE-fallback, BinaryData namespace collision, native-fn bridge gap, module-load ReferenceError.

## Metadata

**Confidence breakdown:**
- Standard stack / binding: HIGH — verbatim from shipped FM/Grain, no new deps.
- Headline overlay (UI-01): HIGH — axis math proven identical in `SubVizAnalyzer.h`.
- Dual-ADSR (UI-02): MEDIUM — no sibling precedent; renderer designed here (playhead detail is an open question).
- Diagram/tooltips/preset hook (3.3): HIGH — direct FM/Grain idioms.

**Research date:** 2026-06-25
**Valid until:** stable (sibling patterns + JUCE 8.0.9 pinned); revisit only if JUCE version bumps.
</content>
</invoke>
